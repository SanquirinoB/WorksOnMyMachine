#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIOverlayCommon.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "CoreKitFrontend/CoreKitFrontendSettings.h"
#include "Engine/Texture2D.h"
#include "Components/ButtonSlot.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "CoreKitBasicUI/Flow/CoreKitBasicUIHUD.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICommonWidgets.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "UObject/StrongObjectPtr.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIOverlay"

namespace CoreKitBasicUIOverlay
{
	FSlateBrush Outline(const FLinearColor& Color, float Px)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(UCoreKitBasicUIStyle::Texture(TEXT("T_UI_White")));
		Brush.DrawAs = ESlateBrushDrawType::Border;
		Brush.ImageSize = FVector2D(16.f, 16.f);
		const float M = FMath::Clamp(Px / 16.f, 0.f, 0.5f);
		Brush.Margin = FMargin(M);
		Brush.TintColor = FSlateColor(Color);
		return Brush;
	}

	UCanvasPanel* AddChamferClip(UWidgetTree* Tree, UCanvasPanel* Parent, FVector2D Position, FVector2D Size, float Cut, FName Name, int32 ZOrder)
	{
		UCanvasPanel* Outer = CoreKitBasicUIBuild::Canvas(Tree, Name);
		Outer->SetClipping(EWidgetClipping::ClipToBounds);
		Outer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		CoreKitBasicUIBuild::Place(Parent, Outer, FAnchors(0.f, 0.f), Position, Size, FVector2D::ZeroVector, ZOrder);
		if (Cut <= 0.f)
		{
			return Outer;
		}

		// Rotated clip: its top edge lies on the chamfer line x + y = Cut and its interior covers the whole rectangle.
		const float W = Size.X;
		const float H = Size.Y;
		const float Side = (W + 2.f * H + Cut) / UE_SQRT_2 + 8.f;
		const FVector2D Corner(-H, Cut + H);
		UCanvasPanel* Rotated = CoreKitBasicUIBuild::Canvas(Tree);
		Rotated->SetClipping(EWidgetClipping::ClipToBounds);
		Rotated->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		CoreKitBasicUIBuild::Place(Outer, Rotated, FAnchors(0.f, 0.f), Corner, FVector2D(Side, Side));
		Rotated->SetRenderTransformPivot(FVector2D::ZeroVector);
		FWidgetTransform RotatedTransform;
		RotatedTransform.Angle = -45.f;
		Rotated->SetRenderTransform(RotatedTransform);

		// Counter-rotated content: local (x, y) lands back on (x, y) of the outer rectangle.
		UCanvasPanel* Inner = CoreKitBasicUIBuild::Canvas(Tree);
		Inner->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		CoreKitBasicUIBuild::Place(Rotated, Inner, FAnchors(0.f, 0.f), FVector2D::ZeroVector, Size);
		Inner->SetRenderTransformPivot(FVector2D::ZeroVector);
		FWidgetTransform InnerTransform;
		InnerTransform.Angle = 45.f;
		InnerTransform.Translation = FQuat2D(FMath::DegreesToRadians(45.f)).TransformPoint(-Corner);
		Inner->SetRenderTransform(InnerTransform);
		return Inner;
	}

	TArray<UImage*> AddChamferOutline(UWidgetTree* Tree, UCanvasPanel* Parent, FVector2D Position, FVector2D Size, float Cut, float Thickness, const FLinearColor& Color, int32 ZOrder)
	{
		TArray<UImage*> Edges;
		auto Edge = [&](FVector2D EdgePos, FVector2D EdgeSize)
		{
			UImage* Image = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(Color));
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
			CoreKitBasicUIBuild::Place(Parent, Image, FAnchors(0.f, 0.f), Position + EdgePos, EdgeSize, FVector2D::ZeroVector, ZOrder);
			Edges.Add(Image);
			return Image;
		};
		const float C = FMath::Max(0.f, Cut);
		Edge(FVector2D(C, 0.f), FVector2D(Size.X - C, Thickness));
		Edge(FVector2D(0.f, C), FVector2D(Thickness, Size.Y - C));
		Edge(FVector2D(Size.X - Thickness, 0.f), FVector2D(Thickness, Size.Y));
		Edge(FVector2D(0.f, Size.Y - Thickness), FVector2D(Size.X, Thickness));
		if (C > 0.f)
		{
			const float Length = C * UE_SQRT_2 + Thickness;
			const FVector2D Center = FVector2D(C * 0.5f, C * 0.5f) + FVector2D(1.f, 1.f) / UE_SQRT_2 * (Thickness * 0.5f);
			UImage* Diagonal = Edge(Center - FVector2D(Length * 0.5f, Thickness * 0.5f), FVector2D(Length, Thickness));
			Diagonal->SetRenderTransformAngle(-45.f);
		}
		return Edges;
	}

	FString Clock(float Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
		return FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
	}

	FString TwoDigits(int32 Value)
	{
		return FString::Printf(TEXT("%02d"), FMath::Max(0, Value));
	}

	FText RestartBody(float RunSeconds)
	{
		return FText::Format(COREKIT_BASICUI_TEXT("RestartBody", "Your build and {0} of progress will be lost. A fresh run starts at 00:00."),
			FText::FromString(Clock(RunSeconds)));
	}

	FText QuitToMenuBody(float RunSeconds, int32 Level, int32 Cards)
	{
		return FText::Format(COREKIT_BASICUI_TEXT("QuitBody", "You will lose this run: {0} survived, level {1} and {2} {2}|plural(one=card,other=cards)."),
			FText::FromString(Clock(RunSeconds)), FText::FromString(TwoDigits(Level)), FText::AsNumber(FMath::Max(0, Cards)));
	}

	TArray<FEffectWord> SplitEffect(const FString& Pre, const FString& Key, const FString& Post)
	{
		const FString Full = Pre + Key + Post;
		const int32 KeyStart = Pre.Len();
		const int32 KeyEnd = KeyStart + Key.Len();
		TArray<FEffectWord> Words;
		FEffectWord Current;
		auto Flush = [&]()
		{
			if (!Current.IsEmpty())
			{
				Words.Add(MoveTemp(Current));
				Current.Reset();
			}
		};
		for (int32 Index = 0; Index < Full.Len(); ++Index)
		{
			const TCHAR Char = Full[Index];
			if (FChar::IsWhitespace(Char))
			{
				Flush();
				continue;
			}
			const bool bKey = Index >= KeyStart && Index < KeyEnd;
			if (Current.IsEmpty() || Current.Last().bKey != bKey)
			{
				FEffectRun& Run = Current.AddDefaulted_GetRef();
				Run.bKey = bKey;
			}
			Current.Last().Text.AppendChar(Char);
		}
		Flush();
		return Words;
	}

	FBox2f CropUV(FVector2D Focus, FVector2D CropPx, FVector2D ScenePx)
	{
		const FVector2f SizeUV(
			FMath::Clamp(static_cast<float>(CropPx.X / FMath::Max(1.0, ScenePx.X)), 0.01f, 1.f),
			FMath::Clamp(static_cast<float>(CropPx.Y / FMath::Max(1.0, ScenePx.Y)), 0.01f, 1.f));
		FVector2f Min(static_cast<float>(Focus.X) - SizeUV.X * 0.5f, static_cast<float>(Focus.Y) - SizeUV.Y * 0.5f);
		Min.X = FMath::Clamp(Min.X, 0.f, 1.f - SizeUV.X);
		Min.Y = FMath::Clamp(Min.Y, 0.f, 1.f - SizeUV.Y);
		return FBox2f(Min, Min + SizeUV);
	}

	UTexture2D* LoadCardArt(const TSoftObjectPtr<UTexture2D>& Art)
	{
		if (Art.IsNull())
		{
			return nullptr;
		}
		if (UTexture2D* Loaded = Art.Get())
		{
			return Loaded;
		}
		static TSet<FSoftObjectPath> Missing;
		const FSoftObjectPath Path = Art.ToSoftObjectPath();
		if (Missing.Contains(Path))
		{
			return nullptr;
		}
		if (!FPackageName::DoesPackageExist(Path.GetLongPackageName()))
		{
			Missing.Add(Path);
			return nullptr;
		}
		UTexture2D* Loaded = Art.LoadSynchronous();
		if (!Loaded)
		{
			Missing.Add(Path);
		}
		return Loaded;
	}

	UCoreKitBasicUIScreenRouter* FindRouter(const UUserWidget* Widget)
	{
		const APlayerController* Controller = Widget ? Widget->GetOwningPlayer() : nullptr;
		const ACoreKitBasicUIHUD* Hud = Controller ? Cast<ACoreKitBasicUIHUD>(Controller->GetHUD()) : nullptr;
		return Hud ? Hud->GetRouter() : nullptr;
	}

	FText StageName(int32 StageIndex)
	{
		switch (StageIndex)
		{
		case 0: return COREKIT_BASICUI_TEXT("Intro", "INTRO");
		case 1: return COREKIT_BASICUI_TEXT("Pressure", "PRESSURE");
		case 2: return COREKIT_BASICUI_TEXT("Survival", "SURVIVAL");
		default: return COREKIT_BASICUI_TEXT("Chaos", "CHAOS");
		}
	}

	namespace
	{
		int32 GLowPassCount = 0;

		USoundMix* LowPassMix()
		{
			static TStrongObjectPtr<USoundMix> Mix;
			if (!Mix.IsValid())
			{
				USoundClass* Music = GetDefault<UCoreKitFrontendSettings>()->MusicSoundClass.LoadSynchronous();
				if (!Music)
				{
					return nullptr;
				}
				USoundMix* NewMix = NewObject<USoundMix>(GetTransientPackage(), TEXT("CoreKitBasicUI_PauseLowPass"));
				FSoundClassAdjuster Adjuster;
				Adjuster.SoundClassObject = Music;
				Adjuster.LowPassFilterFrequency = 900.f;
				Adjuster.VolumeAdjuster = 0.8f;
				Adjuster.bApplyToChildren = true;
				NewMix->SoundClassEffects.Add(Adjuster);
				NewMix->FadeInTime = 0.2f;
				NewMix->FadeOutTime = 0.14f;
				Mix.Reset(NewMix);
			}
			return Mix.Get();
		}
	}

	void PushLowPass(const UObject* WorldContext)
	{
		if (GLowPassCount++ == 0)
		{
			if (USoundMix* Mix = LowPassMix())
			{
				UGameplayStatics::PushSoundMixModifier(WorldContext, Mix);
			}
		}
	}

	void PopLowPass(const UObject* WorldContext)
	{
		if (GLowPassCount <= 0)
		{
			return;
		}
		if (--GLowPassCount == 0)
		{
			if (USoundMix* Mix = LowPassMix())
			{
				UGameplayStatics::PopSoundMixModifier(WorldContext, Mix);
			}
		}
	}

	float CssLineOffset(const FSlateFontInfo& Font, float CssLineHeightPx)
	{
		// Fallback: Saira / Barlow line boxes are about 1.5 em in Slate (size is in points: px = pt / 0.75).
		float SlateHeight = Font.Size / 0.75f * 1.5f;
		if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
		{
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			SlateHeight = static_cast<float>(Measure->GetMaxCharacterHeight(Font));
		}
		return (CssLineHeightPx - SlateHeight) * 0.5f;
	}

	void FillButtonContent(UCoreKitBasicUIActionButton* Button)
	{
		UCoreKitButton* Inner = Button ? Button->GetButton() : nullptr;
		if (UButtonSlot* ContentSlot = Inner ? Cast<UButtonSlot>(Inner->GetContentSlot()) : nullptr)
		{
			ContentSlot->SetPadding(FMargin(0.f));
			ContentSlot->SetHorizontalAlignment(HAlign_Fill);
			ContentSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	void Pose(UWidget* Widget, FVector2D Offset, float Scale, float AngleDeg, float Opacity)
	{
		if (Widget)
		{
			CoreKitBasicUIBuild::SetTransform(Widget, Offset, Scale, AngleDeg);
			Widget->SetRenderOpacity(FMath::Clamp(Opacity, 0.f, 1.f));
		}
	}
}

void FCoreKitBasicUICardDrawFlow::Reset()
{
	State = EState::Intro;
	ChooseElapsed = 0.f;
	PickElapsed = 0.f;
	Picked = INDEX_NONE;
}

void FCoreKitBasicUICardDrawFlow::BeginChoose()
{
	State = EState::Choose;
	ChooseElapsed = 0.f;
	PickElapsed = 0.f;
	Picked = INDEX_NONE;
}

void FCoreKitBasicUICardDrawFlow::Tick(float DeltaSeconds)
{
	if (State == EState::Choose)
	{
		ChooseElapsed += DeltaSeconds;
	}
	else if (State == EState::Picking || State == EState::Committed)
	{
		PickElapsed += DeltaSeconds;
	}
}

bool FCoreKitBasicUICardDrawFlow::TryPick(int32 Index, int32 NumOffers)
{
	if (IsLocked() || Index < 0 || Index >= NumOffers)
	{
		return false;
	}
	State = EState::Picking;
	Picked = Index;
	PickElapsed = 0.f;
	return true;
}

bool FCoreKitBasicUICardDrawFlow::ConsumeCommit(float CommitDelaySeconds)
{
	if (State != EState::Picking || PickElapsed < CommitDelaySeconds)
	{
		return false;
	}
	State = EState::Committed;
	++Commits;
	return true;
}

void FCoreKitBasicUICardDrawFlow::RevertPick()
{
	BeginChoose();
}

#undef LOCTEXT_NAMESPACE
