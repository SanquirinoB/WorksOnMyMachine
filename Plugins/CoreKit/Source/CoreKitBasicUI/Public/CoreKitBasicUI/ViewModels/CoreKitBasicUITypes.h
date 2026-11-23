#pragma once

#include "Engine/Texture2D.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"
#include "CoreKitBasicUITypes.generated.h"

/** Run phase as the UI sees it. */
UENUM(BlueprintType)
enum class ECoreKitBasicUIRunPhase : uint8
{
	Survival,
	/** Final warning interval before the configured boss arrival (production 04:00). */
	BossInbound,
	Boss,
	Victory,
	Defeat
};

UENUM(BlueprintType)
enum class ECoreKitBasicUIDamageSource : uint8
{
	Rifle,
	Katana,
	Electric,
	Other
};

UENUM(BlueprintType)
enum class ECoreKitBasicUIPickupKind : uint8
{
	XP,
	Heal
};

UENUM(BlueprintType)
enum class ECoreKitBasicUIMinimapKind : uint8
{
	DroneSphere,
	DroneCube,
	DronePyramid,
	DroneOther,
	Boss,
	XPOrb,
	RepairKit
};

/** Presentation of one card (Docs/UI/Spec/06-Cards.md). */
USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUICardData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") ECoreKitBasicUICardSuit Suit = ECoreKitBasicUICardSuit::Sight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bSpecial = false;
	/** "SIGHT · RIFLE" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText SuitLabel;
	/** Effect sentence split so the key part can be coloured: Pre + Key + Post. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText EffectPre;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText EffectKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText EffectPost;
	/** One summed-effect string for build lists ("Rifle damage +30%"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText SummedEffect;
	/** Card Draw projection footer ("OWNED ×2 → RIFLE DMG +45%"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText Footer;
	/** Short label for compact lists (FP, RF, EM, PIERCE...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText Abbrev;
	/** Copies owned (0 when offered and not owned). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 Copies = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") TSoftObjectPtr<UTexture2D> ArtScene;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") TSoftObjectPtr<UTexture2D> ArtEffect;
	/** Subject of the art in UV (0..1), for small crops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FVector2D ArtFocus = FVector2D(0.5f, 0.47f);

	/** Accent: magenta for every special, else the suit colour. */
	FLinearColor Accent() const { return bSpecial ? CoreKitBasicUIColor::Magenta : UCoreKitBasicUIStyle::SuitColor(Suit); }

	bool operator==(const FCoreKitBasicUICardData& Other) const
	{
		return Id == Other.Id && Copies == Other.Copies && SummedEffect.EqualTo(Other.SummedEffect) && Footer.EqualTo(Other.Footer);
	}
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIWeaponSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 SlotNumber = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText ShortName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText LongName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bActive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bMelee = false;
	/** T_Icon_Rifle / T_Icon_Katana */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FName Icon;

	bool operator==(const FCoreKitBasicUIWeaponSlotData& Other) const
	{
		return SlotNumber == Other.SlotNumber && bActive == Other.bActive && bMelee == Other.bMelee && Icon == Other.Icon && ShortName.EqualTo(Other.ShortName);
	}
};

UENUM(BlueprintType)
enum class ECoreKitBasicUITimelineMarkKind : uint8
{
	StageStarted,
	LevelUp,
	BossInbound,
	BossSpawned,
	Death,
	Victory
};

/** One mark on the run timeline of the Victory / Defeat screens. */
USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUITimelineMark
{
	GENERATED_BODY()

	/** Run seconds (survival clock, then boss seconds after 600). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float Seconds = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") ECoreKitBasicUITimelineMarkKind Kind = ECoreKitBasicUITimelineMarkKind::StageStarted;
	/** Stage index, level reached... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 Value = 0;

	bool operator==(const FCoreKitBasicUITimelineMark& Other) const
	{
		return Kind == Other.Kind && Value == Other.Value && FMath::IsNearlyEqual(Seconds, Other.Seconds);
	}
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIMinimapEntity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") ECoreKitBasicUIMinimapKind Kind = ECoreKitBasicUIMinimapKind::DroneSphere;
	/** World XY. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FVector2D Position = FVector2D::ZeroVector;
	/** Locking on from behind or off-screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bAlert = false;

	bool operator==(const FCoreKitBasicUIMinimapEntity& Other) const
	{
		return Kind == Other.Kind && Position.Equals(Other.Position, 1.f) && bAlert == Other.bAlert;
	}
};
