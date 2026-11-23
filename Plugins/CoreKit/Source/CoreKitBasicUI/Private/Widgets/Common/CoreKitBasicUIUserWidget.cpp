#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	float RealDelta(double& Last)
	{
		const double Now = FPlatformTime::Seconds();
		const float Delta = Last > 0.0 ? static_cast<float>(FMath::Min(Now - Last, 0.1)) : 0.f;
		Last = Now;
		return Delta;
	}

	UCoreKitBasicUIContextSubsystem* ContextFor(const UUserWidget* Widget)
	{
		const ULocalPlayer* Player = Widget ? Widget->GetOwningLocalPlayer() : nullptr;
		return Player ? Player->GetSubsystem<UCoreKitBasicUIContextSubsystem>() : nullptr;
	}
}

USoundBase* CoreKitBasicUISound::Find(FName Cue)
{
	static TMap<FName, TArray<TStrongObjectPtr<USoundBase>>> Cache;
	TArray<TStrongObjectPtr<USoundBase>>* Variants = Cache.Find(Cue);
	if (!Variants)
	{
		Variants = &Cache.Add(Cue);
		const FString Base = FString::Printf(TEXT("UI_%s"), *Cue.ToString().Replace(TEXT("."), TEXT("_")));
		auto TryLoad = [Variants](const FString& Name)
		{
			const FString Path = FString::Printf(TEXT("/CoreKit/BasicUI/Audio/%s.%s"), *Name, *Name);
			if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				Variants->Emplace(Sound);
			}
		};
		TryLoad(Base);
		for (int32 Index = 1; Index <= 3; ++Index)
		{
			TryLoad(FString::Printf(TEXT("%s_%d"), *Base, Index));
		}
	}
	if (Variants->IsEmpty())
	{
		return nullptr;
	}
	return (*Variants)[FMath::RandRange(0, Variants->Num() - 1)].Get();
}

void CoreKitBasicUISound::Play(const UObject* WorldContext, FName Cue, float Volume)
{
	if (USoundBase* Sound = Find(Cue))
	{
		UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume);
	}
}

UCoreKitBasicUIContextSubsystem* UCoreKitBasicUIUserWidget::GetUIContext() const
{
	return ContextFor(this);
}

TSharedRef<SWidget> UCoreKitBasicUIUserWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UCoreKitBasicUIUserWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	TickMotion(RealDelta(LastRealTime));
}

void UCoreKitBasicUIUserWidget::PlayCue(FName Cue) const
{
	CoreKitBasicUISound::Play(this, Cue);
}

UCoreKitBasicUIContextSubsystem* UCoreKitBasicUIMenuWidget::GetUIContext() const
{
	return ContextFor(this);
}

TSharedRef<SWidget> UCoreKitBasicUIMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UCoreKitBasicUIMenuWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	TickMotion(RealDelta(LastRealTime));
}

void UCoreKitBasicUIMenuWidget::PlayCue(FName Cue) const
{
	CoreKitBasicUISound::Play(this, Cue);
}

void UCoreKitBasicUIUserWidget::NativeConstruct() { LastRealTime = 0.; Super::NativeConstruct(); }
void UCoreKitBasicUIUserWidget::NativeDestruct() { LastRealTime = 0.; Super::NativeDestruct(); }
void UCoreKitBasicUIMenuWidget::NativeConstruct() { LastRealTime = 0.; Super::NativeConstruct(); }
void UCoreKitBasicUIMenuWidget::NativeDestruct() { LastRealTime = 0.; Super::NativeDestruct(); }
