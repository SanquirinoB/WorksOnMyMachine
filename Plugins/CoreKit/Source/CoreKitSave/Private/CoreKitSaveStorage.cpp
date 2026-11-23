#include "CoreKitSaveStorage.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Crc.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

namespace CoreKitSave
{
namespace
{
constexpr uint32 FileMagic = 0x56534B43; // CKSV
constexpr int32 CurrentVersion = 1;
constexpr int32 HeaderSize = sizeof(uint32) + 2 * sizeof(int32) + sizeof(uint64) + sizeof(int64);
constexpr int32 ChecksumSize = sizeof(uint32);
FCriticalSection SlotFiles;

FString GetSlotDirectory()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CoreKit"), TEXT("Slots"));
}

struct FRecord
{
	FDiskResult Data;
	uint64 Generation = 0;
};

FRecord ReadRecord(IPlatformFile& File, const FString& Path, const FString& SlotName, bool bVerify)
{
	FRecord Record;
	if (!File.FileExists(*Path))
	{
		return Record;
	}

	Record.Data.Result = ECoreKitSaveResult::IOError;
	const TUniquePtr<IFileHandle> Handle(File.OpenRead(*Path));
	if (!Handle)
	{
		return Record;
	}

	const int64 Size = Handle->Size();
	if (Size < HeaderSize + ChecksumSize || Size > HeaderSize + MaxPayloadBytes + ChecksumSize)
	{
		Record.Data.Result = ECoreKitSaveResult::Corrupt;
		return Record;
	}

	TArray<uint8> Bytes;
	Bytes.SetNumUninitialized(bVerify ? static_cast<int32>(Size) : HeaderSize);
	if (!Handle->Read(Bytes.GetData(), Bytes.Num()))
	{
		return Record;
	}

	Record.Data.Result = ECoreKitSaveResult::Corrupt;
	const int32 RecordSize = static_cast<int32>(Size) - ChecksumSize;
	if (bVerify)
	{
		uint32 StoredChecksum = 0;
		FMemory::Memcpy(&StoredChecksum, Bytes.GetData() + RecordSize, ChecksumSize);
		if (FCrc::MemCrc32(Bytes.GetData(), RecordSize) != StoredChecksum)
		{
			return Record;
		}
	}

	FMemoryReader Reader(Bytes);
	uint32 Magic = 0;
	int32 Version = 0;
	Reader << Magic << Version;
	if (Magic != FileMagic)
	{
		return Record;
	}
	if (Version != CurrentVersion)
	{
		Record.Data.Result = ECoreKitSaveResult::UnsupportedVersion;
		return Record;
	}

	int64 WrittenAtTicks = 0;
	int32 PayloadSize = 0;
	Reader << Record.Generation << WrittenAtTicks << PayloadSize;
	if (Reader.IsError() || Record.Generation == 0 || PayloadSize <= 0
		|| PayloadSize != RecordSize - HeaderSize || WrittenAtTicks < 0 || WrittenAtTicks > FDateTime::MaxValue().GetTicks())
	{
		return Record;
	}

	if (bVerify)
	{
		Record.Data.Payload.Append(Bytes.GetData() + HeaderSize, PayloadSize);
	}
	Record.Data.Info.SlotName = SlotName;
	Record.Data.Info.WrittenAtUtc = FDateTime(WrittenAtTicks);
	Record.Data.Info.FormatVersion = Version;
	Record.Data.Info.bVerified = bVerify;
	Record.Data.Result = ECoreKitSaveResult::Success;
	return Record;
}

struct FSlotFiles
{
	explicit FSlotFiles(const FString& SlotName, bool bVerify = true)
		: File(FPlatformFileManager::Get().GetPlatformFile()),
		  Directory(GetSlotDirectory()),
		  APath(FPaths::Combine(Directory, SlotName + TEXT(".a.cks"))),
		  BPath(FPaths::Combine(Directory, SlotName + TEXT(".b.cks"))),
		  A(ReadRecord(File, APath, SlotName, bVerify)), B(ReadRecord(File, BPath, SlotName, bVerify))
	{
	}

	bool HasResult(ECoreKitSaveResult Result) const
	{
		return A.Data.Result == Result || B.Data.Result == Result;
	}

	FRecord* FindNewest()
	{
		if (A.Data.Result == ECoreKitSaveResult::Success
			&& (B.Data.Result != ECoreKitSaveResult::Success || A.Generation >= B.Generation))
		{
			return &A;
		}
		return B.Data.Result == ECoreKitSaveResult::Success ? &B : nullptr;
	}

	IPlatformFile& File;
	FString Directory;
	FString APath;
	FString BPath;
	FRecord A;
	FRecord B;
};

bool CommitRecord(FSlotFiles& Files, const FString& Path, const FBufferArchive& Bytes)
{
	if (!Files.File.CreateDirectoryTree(*Files.Directory))
	{
		return false;
	}

	const FString TemporaryPath = Path + TEXT(".tmp");
	TUniquePtr<IFileHandle> Writer(Files.File.OpenWrite(*TemporaryPath));
	const bool bWritten = Writer && Writer->Write(Bytes.GetData(), Bytes.Num()) && Writer->Flush(true);
	Writer.Reset();

	// The destination is the older bank. The newest valid bank remains intact through the commit.
	if (bWritten && (!Files.File.FileExists(*Path) || Files.File.DeleteFile(*Path))
		&& Files.File.MoveFile(*Path, *TemporaryPath))
	{
		return true;
	}

	Files.File.DeleteFile(*TemporaryPath);
	return false;
}
}

bool IsValidSlotName(const FString& SlotName)
{
	if (SlotName.IsEmpty() || SlotName.Len() > 64)
	{
		return false;
	}

	for (TCHAR Character : SlotName)
	{
		const bool bAsciiLetter = (Character >= TEXT('a') && Character <= TEXT('z'))
			|| (Character >= TEXT('A') && Character <= TEXT('Z'));
		const bool bAsciiDigit = Character >= TEXT('0') && Character <= TEXT('9');
		if (!bAsciiLetter && !bAsciiDigit && Character != TEXT('-') && Character != TEXT('_'))
		{
			return false;
		}
	}

	// Keep Windows device names out of the filename stem.
	const FString Lower = SlotName.ToLower();
	if (Lower == TEXT("con") || Lower == TEXT("prn") || Lower == TEXT("aux") || Lower == TEXT("nul"))
	{
		return false;
	}
	return Lower.Len() != 4 || (!Lower.StartsWith(TEXT("com")) && !Lower.StartsWith(TEXT("lpt")))
		|| Lower[3] < TEXT('0') || Lower[3] > TEXT('9');
}

static FDiskResult ReadSlotLocked(const FString& SlotName, bool bVerify)
{
	FSlotFiles Files(SlotName, bVerify);
	if (Files.HasResult(ECoreKitSaveResult::UnsupportedVersion))
	{
		return { ECoreKitSaveResult::UnsupportedVersion };
	}
	if (FRecord* Latest = Files.FindNewest())
	{
		Latest->Data.Info.bRecoveredBackup = bVerify && (Files.HasResult(ECoreKitSaveResult::Corrupt)
			|| Files.HasResult(ECoreKitSaveResult::IOError));
		return MoveTemp(Latest->Data);
	}
	if (Files.HasResult(ECoreKitSaveResult::IOError))
	{
		return { ECoreKitSaveResult::IOError };
	}
	if (Files.HasResult(ECoreKitSaveResult::Corrupt))
	{
		return { ECoreKitSaveResult::Corrupt };
	}
	return { ECoreKitSaveResult::NotFound };
}

FDiskResult ReadSlot(const FString& SlotName)
{
	FScopeLock Lock(&SlotFiles);
	return ReadSlotLocked(SlotName, true);
}

static FCoreKitSaveCatalogEntry ReadSlotInfoLocked(const FString& SlotName)
{
	const FDiskResult Data = ReadSlotLocked(SlotName, false);
	FCoreKitSaveCatalogEntry Entry;
	Entry.Info = Data.Info;
	Entry.Info.SlotName = SlotName;
	Entry.Result = Data.Result;
	return Entry;
}

FCoreKitSaveCatalogEntry ReadSlotInfo(const FString& SlotName)
{
	FScopeLock Lock(&SlotFiles);
	return ReadSlotInfoLocked(SlotName);
}

TArray<FCoreKitSaveCatalogEntry> ListSlotInfo()
{
	FScopeLock Lock(&SlotFiles);
	TArray<FString> Files;
	FPlatformFileManager::Get().GetPlatformFile().FindFiles(Files, *GetSlotDirectory(), TEXT(".cks"));
	TSet<FString> Names;
	for (const FString& File : Files)
	{
		const FString Bank = FPaths::GetBaseFilename(File);
		if (Bank.EndsWith(TEXT(".a")) || Bank.EndsWith(TEXT(".b")))
		{
			const FString SlotName = Bank.LeftChop(2).ToLower();
			if (IsValidSlotName(SlotName))
			{
				Names.Add(SlotName);
			}
		}
	}

	TArray<FCoreKitSaveCatalogEntry> Entries;
	Entries.Reserve(Names.Num());
	for (const FString& SlotName : Names)
	{
		Entries.Add(ReadSlotInfoLocked(SlotName));
	}
	Entries.Sort([](const FCoreKitSaveCatalogEntry& A, const FCoreKitSaveCatalogEntry& B)
	{
		return A.Info.WrittenAtUtc == B.Info.WrittenAtUtc
			? A.Info.SlotName < B.Info.SlotName : A.Info.WrittenAtUtc > B.Info.WrittenAtUtc;
	});
	return Entries;
}

FDiskResult WriteSlot(const FString& SlotName, const TArray<uint8>& Payload)
{
	FScopeLock Lock(&SlotFiles);
	FSlotFiles Files(SlotName);
	if (Files.HasResult(ECoreKitSaveResult::UnsupportedVersion))
	{
		return { ECoreKitSaveResult::UnsupportedVersion };
	}
	if (Files.HasResult(ECoreKitSaveResult::IOError))
	{
		return { ECoreKitSaveResult::IOError };
	}

	const FRecord* Latest = Files.FindNewest();
	if (Latest && Latest->Generation == MAX_uint64)
	{
		return { ECoreKitSaveResult::IOError };
	}

	uint32 Magic = FileMagic;
	int32 Version = CurrentVersion;
	uint64 Generation = Latest ? Latest->Generation + 1 : 1;
	const FDateTime WrittenAt = FDateTime::UtcNow();
	int64 WrittenAtTicks = WrittenAt.GetTicks();
	int32 PayloadSize = Payload.Num();

	FBufferArchive Bytes;
	Bytes.Reserve(HeaderSize + PayloadSize + ChecksumSize);
	Bytes << Magic << Version << Generation << WrittenAtTicks << PayloadSize;
	Bytes.Serialize(const_cast<uint8*>(Payload.GetData()), PayloadSize);
	uint32 Checksum = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
	Bytes << Checksum;

	const FString& Destination = Latest == &Files.A ? Files.BPath : Files.APath;
	if (!CommitRecord(Files, Destination, Bytes))
	{
		return { ECoreKitSaveResult::IOError };
	}

	FDiskResult Result;
	Result.Result = ECoreKitSaveResult::Success;
	Result.Info.SlotName = SlotName;
	Result.Info.WrittenAtUtc = WrittenAt;
	Result.Info.FormatVersion = CurrentVersion;
	Result.Info.bVerified = true;
	return Result;
}
}
