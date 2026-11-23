#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudEvents.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Components/Border.h"
#include "Widgets/HUD/CoreKitBasicUIHudCommon.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIArcWidget.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIHud"

using namespace CoreKitBasicUIHudBuild;

namespace
{
	/** Places a widget on a 200 x 200 centre canvas by its centre offset from the screen centre. */
	UCanvasPanelSlot* PlaceCentered(UCanvasPanel* Canvas, UWidget* Widget, FVector2D CenterOffset, FVector2D Size, int32 Z = 0)
	{
		return CoreKitBasicUIBuild::Place(Canvas, Widget, FAnchors(0.5f, 0.5f), CenterOffset, Size, FVector2D(0.5f, 0.5f), Z);
	}

	UImage* MakeTick(UWidgetTree* Tree, UCanvasPanel* Canvas, FVector2D Center, FVector2D Size, float Angle, const FLinearColor& Color, bool bOutline)
	{
		if (bOutline)
		{
			UImage* Outline = Solid(Tree, CoreKitBasicUIColor::WithAlpha(FLinearColor::Black, 0.5f));
			PlaceCentered(Canvas, Outline, Center, Size + FVector2D(2.f, 2.f));
			Outline->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			SetXform(Outline, FVector2D::ZeroVector, 1.f, Angle);
		}
		UImage* Part = Solid(Tree, Color);
		PlaceCentered(Canvas, Part, Center, Size, 1);
		Part->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SetXform(Part, FVector2D::ZeroVector, 1.f, Angle);
		return Part;
	}

	UCanvasPanel* CenterLayer(UWidgetTree* Tree, UCanvasPanel* Parent, FName Name)
	{
		UCanvasPanel* Layer = CoreKitBasicUIBuild::Canvas(Tree, Name);
		CoreKitBasicUIBuild::Fill(Parent, Layer);
		Layer->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		return Layer;
	}

	UCoreKitBasicUIArcWidget* MakeArc(UWidgetTree* Tree, FName Name, float Thickness, const FLinearColor& Color)
	{
		UCoreKitBasicUIArcWidget* Arc = Tree->ConstructWidget<UCoreKitBasicUIArcWidget>(UCoreKitBasicUIArcWidget::StaticClass(), Name);
		Arc->SetThickness(Thickness);
		Arc->SetArcColor(Color);
		Arc->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		return Arc;
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Crosshair, hit marker, kill confirm, reload ring

void UCoreKitBasicUIHudCrosshair::BuildTree()
{
	UCanvasPanel* Root = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("CrosshairRoot"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Root, 200.f, 200.f, TEXT("Crosshair"));

	// Rifle: 4 px dot + 4 ticks of 2 x 10 at 12 px, white with a 1 px black 50 % outline.
	Cross = CenterLayer(WidgetTree, Root, TEXT("Cross"));
	CrossParts.Add(MakeTick(WidgetTree, Cross, FVector2D::ZeroVector, FVector2D(4.f, 4.f), 0.f, FLinearColor::White, true));
	CrossParts.Add(MakeTick(WidgetTree, Cross, FVector2D(0.f, -17.f), FVector2D(2.f, 10.f), 0.f, FLinearColor::White, true));
	CrossParts.Add(MakeTick(WidgetTree, Cross, FVector2D(0.f, 17.f), FVector2D(2.f, 10.f), 0.f, FLinearColor::White, true));
	CrossParts.Add(MakeTick(WidgetTree, Cross, FVector2D(-17.f, 0.f), FVector2D(10.f, 2.f), 0.f, FLinearColor::White, true));
	CrossParts.Add(MakeTick(WidgetTree, Cross, FVector2D(17.f, 0.f), FVector2D(10.f, 2.f), 0.f, FLinearColor::White, true));

	// Katana: magenta dot + lower arc of radius 30 centred 8 px above the aim point.
	Blade = CenterLayer(WidgetTree, Root, TEXT("Blade"));
	BladeDot = Solid(WidgetTree, CoreKitBasicUIColor::Magenta, TEXT("BladeDot"));
	PlaceCentered(Blade, BladeDot, FVector2D::ZeroVector, FVector2D(4.f, 4.f));
	BladeArc = MakeArc(WidgetTree, TEXT("BladeArc"), 2.f, CoreKitBasicUIColor::Magenta);
	BladeArc->SetArc(90.f, 180.f);
	PlaceCentered(Blade, BladeArc, FVector2D(0.f, -8.f), FVector2D(62.f, 62.f));
	Blade->SetRenderOpacity(0.f);

	// Hit marker: 4 ticks of 11 x 3 at 45 degrees.
	HitMarker = CenterLayer(WidgetTree, Root, TEXT("HitMarker"));
	HitTicks.Add(MakeTick(WidgetTree, HitMarker, FVector2D(-9.5f, -13.5f), FVector2D(11.f, 3.f), 45.f, FLinearColor::White, false));
	HitTicks.Add(MakeTick(WidgetTree, HitMarker, FVector2D(9.5f, -13.5f), FVector2D(11.f, 3.f), -45.f, FLinearColor::White, false));
	HitTicks.Add(MakeTick(WidgetTree, HitMarker, FVector2D(-9.5f, 13.5f), FVector2D(11.f, 3.f), -45.f, FLinearColor::White, false));
	HitTicks.Add(MakeTick(WidgetTree, HitMarker, FVector2D(9.5f, 13.5f), FVector2D(11.f, 3.f), 45.f, FLinearColor::White, false));
	HitMarker->SetRenderOpacity(0.f);

	// Kill confirm: 18 x 4 X strokes + 88 px ring of 2 px.
	KillMarker = CenterLayer(WidgetTree, Root, TEXT("KillMarker"));
	MakeTick(WidgetTree, KillMarker, FVector2D(-15.f, -22.f), FVector2D(18.f, 4.f), 45.f, FLinearColor::White, false);
	MakeTick(WidgetTree, KillMarker, FVector2D(15.f, -22.f), FVector2D(18.f, 4.f), -45.f, FLinearColor::White, false);
	MakeTick(WidgetTree, KillMarker, FVector2D(-15.f, 22.f), FVector2D(18.f, 4.f), -45.f, FLinearColor::White, false);
	MakeTick(WidgetTree, KillMarker, FVector2D(15.f, 22.f), FVector2D(18.f, 4.f), 45.f, FLinearColor::White, false);
	KillMarker->SetRenderOpacity(0.f);
	KillRing = MakeArc(WidgetTree, TEXT("KillRing"), 2.f, CoreKitBasicUIColor::WhiteA(0.8f));
	PlaceCentered(Root, KillRing, FVector2D::ZeroVector, FVector2D(88.f, 88.f));
	KillRing->SetRenderOpacity(0.f);

	// Reload: 64 px ring of 3 px, "RELOADING" 10 px at +40 px.
	ReloadRing = MakeArc(WidgetTree, TEXT("ReloadRing"), 3.f, CoreKitBasicUIColor::White);
	ReloadRing->SetProgress(0.f);
	PlaceCentered(Root, ReloadRing, FVector2D::ZeroVector, FVector2D(64.f, 64.f));
	CoreKitBasicUIBuild::FTextStyle LabelStyle = Sh(M(10.f, 500, CoreKitBasicUIColor::White, 0.2f));
	LabelStyle.Justify = ETextJustify::Center;
	ReloadLabel = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Reloading", "RELOADING"), LabelStyle, TEXT("ReloadLabel"));
	CoreKitBasicUIBuild::Place(Root, ReloadLabel, FAnchors(0.5f, 0.5f), FVector2D(0.f, 40.f), FVector2D(120.f, 16.f), FVector2D(0.5f, 0.f));
	ReloadRing->SetRenderOpacity(0.f);
	ReloadLabel->SetRenderOpacity(0.f);

	// "R LOW AMMO" (large warning next to the crosshair; the compact block says "LOW").
	UHorizontalBox* Low = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("LowAmmoWarning"));
	UCoreKitBasicUIHudKeycap* Key = WidgetTree->ConstructWidget<UCoreKitBasicUIHudKeycap>(UCoreKitBasicUIHudKeycap::StaticClass(), TEXT("LowAmmoKey"));
	Key->SetAccent(CoreKitBasicUIColor::Magenta, CoreKitBasicUIColor::Magenta);
	Key->SetMetrics(30.f, 13.f, 1.5f, 3.f);
	if (UInputAction* Reload = CoreKitBasicUIHudUtil::FindInputAction(TEXT("IA_Reload")))
	{
		Key->SetAction(Reload);
	}
	else
	{
		Key->SetKey(EKeys::R);
	}
	CoreKitBasicUIBuild::AddH(Low, Key, FMargin(0.f, 0.f, 8.f, 0.f));
	CoreKitBasicUIBuild::AddH(Low, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("LowAmmo", "LOW AMMO"), Sh(M(12.f, 500, CoreKitBasicUIColor::Magenta, 0.14f))));
	CoreKitBasicUIBuild::Place(Root, Low, FAnchors(0.5f, 0.5f), FVector2D(56.f, 0.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
	LowAmmo = Low;
	Show(LowAmmo, false);
	Morph.Snap(0.f);
}

void UCoreKitBasicUIHudCrosshair::PlayHit(ECoreKitBasicUIDamageSource Source)
{
	// Tint follows the weapon (rifle white, katana magenta), never the crosshair colour.
	const FLinearColor Tint = Source == ECoreKitBasicUIDamageSource::Katana ? CoreKitBasicUIColor::Magenta : FLinearColor::White;
	for (UImage* Part : HitTicks)
	{
		Part->SetColorAndOpacity(Tint);
	}
	Hit.Start(CoreKitBasicUIHudTiming::HitMarker);
}

void UCoreKitBasicUIHudCrosshair::PlayKill()
{
	Kill.Start(CoreKitBasicUIHudTiming::KillConfirm);
}

void UCoreKitBasicUIHudCrosshair::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUILoadoutViewModel* Loadout = Ctx ? Ctx->GetLoadout() : nullptr;
	const UCoreKitBasicUIPreferencesViewModel* Prefs = Ctx ? Ctx->GetPreferences() : nullptr;
	const float Motion = MotionAmount();

	if (Prefs && !Prefs->CrosshairColor.Equals(LastColor))
	{
		LastColor = Prefs->CrosshairColor;
		for (UImage* Part : CrossParts)
		{
			Part->SetColorAndOpacity(LastColor);
		}
	}

	if (Loadout)
	{
		const bool bNowMelee = Loadout->bMeleeActive;
		if (!bMorphInit || bNowMelee != bMelee)
		{
			// Cross <-> dot + arc: 160 ms SETTLE (scale 0.3 and 45 degrees).
			if (bMorphInit)
			{
				Morph.Start(Morph.Value, bNowMelee ? 1.f : 0.f, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::CrosshairMorph);
			}
			else
			{
				Morph.Snap(bNowMelee ? 1.f : 0.f);
			}
			bMelee = bNowMelee;
			bMorphInit = true;
		}
	}
	Morph.Tick(Dt);
	const float MorphAmount = FMath::Clamp(Morph.Value, 0.f, 1.f);
	Cross->SetRenderOpacity(1.f - MorphAmount);
	SetXform(Cross, FVector2D::ZeroVector, FMath::Lerp(1.f, FMath::Lerp(1.f, 0.3f, Motion), MorphAmount), 45.f * MorphAmount * Motion);
	Blade->SetRenderOpacity(MorphAmount);
	SetXform(Blade, FVector2D::ZeroVector, FMath::Lerp(FMath::Lerp(1.f, 0.4f, Motion), 1.f, MorphAmount));

	if (Hit.Tick(Dt))
	{
		const float E = Ease(ECoreKitBasicUIMotion::Snap, Hit.Alpha());
		HitMarker->SetRenderOpacity(1.f - E);
		SetXform(HitMarker, FVector2D::ZeroVector, FMath::Lerp(1.f + 0.5f * Motion, 1.f, E));
	}
	if (Kill.Tick(Dt))
	{
		const float A = Kill.Alpha();
		const float E = Ease(ECoreKitBasicUIMotion::Snap, A);
		KillMarker->SetRenderOpacity(1.f - Seg(A, 0.55f, 1.f));
		SetXform(KillMarker, FVector2D::ZeroVector, FMath::Lerp(Motion > 0.f ? 0.6f : 1.f, Motion > 0.f ? 1.5f : 1.f, E));
		KillRing->SetRenderOpacity(0.95f * (1.f - E));
		SetXform(KillRing, FVector2D::ZeroVector, FMath::Lerp(Motion > 0.f ? 0.3f : 1.f, Motion > 0.f ? 1.7f : 1.f, E));
	}

	// Reload ring follows the real reload progress (LINEAR); fades out at the end.
	const bool bReloading = Loadout && Loadout->bReloading && !bMelee;
	if (bReloading)
	{
		LastReloadProgress = FMath::Clamp(Loadout->ReloadProgress, 0.f, 1.f);
		ReloadVisible = 1.f;
	}
	else
	{
		ReloadVisible = FMath::Max(0.f, ReloadVisible - Dt / 0.1f);
		if (ReloadVisible > 0.f)
		{
			LastReloadProgress = 1.f;
		}
	}
	ReloadRing->SetProgress(LastReloadProgress);
	ReloadRing->SetRenderOpacity(ReloadVisible);
	ReloadLabel->SetRenderOpacity(ReloadVisible);

	const bool bLow = Loadout && !bMelee && !bReloading && Loadout->bLowAmmo;
	Show(LowAmmo, bLow);
}

// ---------------------------------------------------------------------------------------------------------------
// Damage direction arcs

void UCoreKitBasicUIHudDamageArcs::BuildTree()
{
	UOverlay* Root = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("DamageArcStack"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Root, 440.f, 440.f, TEXT("DamageArcs"));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UCoreKitBasicUIArcWidget* GlowArc = MakeArc(WidgetTree, *FString::Printf(TEXT("DamageGlow%d"), Index), 16.f, CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Orange, 0.3f));
		CoreKitBasicUIBuild::AddO(Root, GlowArc, HAlign_Fill, VAlign_Fill, FMargin(15.f));
		UCoreKitBasicUIArcWidget* Arc = MakeArc(WidgetTree, *FString::Printf(TEXT("DamageArc%d"), Index), 6.f, CoreKitBasicUIColor::Orange);
		CoreKitBasicUIBuild::AddO(Root, Arc, HAlign_Fill, VAlign_Fill, FMargin(20.f));
		GlowArc->SetRenderOpacity(0.f);
		Arc->SetRenderOpacity(0.f);
		Glows.Add(GlowArc);
		Arcs.Add(Arc);
		State.AddDefaulted();
	}
}

void UCoreKitBasicUIHudDamageArcs::AddHit(const FVector& SourceLocation)
{
	int32 Free = State.IndexOfByPredicate([](const FArc& Arc) { return !Arc.Life.IsRunning(); });
	if (Free == INDEX_NONE)
	{
		float Oldest = -1.f;
		for (int32 Index = 0; Index < State.Num(); ++Index)
		{
			if (State[Index].Life.Time > Oldest)
			{
				Oldest = State[Index].Life.Time;
				Free = Index;
			}
		}
	}
	if (State.IsValidIndex(Free))
	{
		State[Free].Source = FVector2D(SourceLocation);
		State[Free].Life.Start(CoreKitBasicUIHudTiming::DamageArc);
	}
}

void UCoreKitBasicUIHudDamageArcs::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIMinimapViewModel* Minimap = Ctx ? Ctx->GetMinimap() : nullptr;
	FVector2D PlayerPosition = Minimap ? Minimap->PlayerPosition : FVector2D::ZeroVector;
	float PlayerYaw = Minimap ? Minimap->PlayerYaw : 0.f;
	// Prefer the live view point (the Minimap ViewModel refreshes at 10-15 Hz).
	if (const APlayerController* Controller = GetOwningPlayer())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		PlayerPosition = FVector2D(ViewLocation);
		PlayerYaw = ViewRotation.Yaw;
	}
	for (int32 Index = 0; Index < State.Num(); ++Index)
	{
		FArc& Arc = State[Index];
		if (!Arc.Life.Tick(Dt))
		{
			continue;
		}
		const float A = Arc.Life.Alpha();
		const float Opacity = A < CoreKitBasicUIHudTiming::DamageArcHold ? 1.f : 1.f - Seg(A, CoreKitBasicUIHudTiming::DamageArcHold, 1.f);
		const float Direction = CoreKitBasicUIHudLogic::RelativeDirectionDeg(PlayerPosition, PlayerYaw, Arc.Source);
		Arcs[Index]->SetArc(Direction - 45.f, 90.f);
		Glows[Index]->SetArc(Direction - 45.f, 90.f);
		Arcs[Index]->SetRenderOpacity(Arc.Life.IsRunning() ? Opacity : 0.f);
		Glows[Index]->SetRenderOpacity(Arc.Life.IsRunning() ? Opacity : 0.f);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Screen effects

void UCoreKitBasicUIHudScreenFx::BuildTree()
{
	UCanvasPanel* Root = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("ScreenFx"));
	WidgetTree->RootWidget = Root;

	// Warm boss tint: rgba(40,12,0,.42) at the top fading to a light Soot veil.
	BossTint = Solid(WidgetTree, CoreKitBasicUIColor::SootA(0.17f), TEXT("BossTint"));
	CoreKitBasicUIBuild::Fill(Root, BossTint);
	FSlateBrush Warm = UCoreKitBasicUIStyle::Image(TEXT("T_UI_GradV"), CoreKitBasicUIColor::Hex(0x280C00, 0.42f));
	Warm.Mirroring = ESlateBrushMirrorType::Vertical;
	BossTintTop = CoreKitBasicUIBuild::Image(WidgetTree, Warm, TEXT("BossTintTop"));
	UCanvasPanelSlot* WarmSlot = CoreKitBasicUIBuild::Fill(Root, BossTintTop);
	WarmSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.55f));
	WarmSlot->SetOffsets(FMargin(0.f));
	BossTint->SetRenderOpacity(0.f);
	BossTintTop->SetRenderOpacity(0.f);

	// Low health: inset 0 0 240px 30px rgba(255,59,78,.48) + dark red corners, pulsing 1 Hz.
	LowDark = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_EdgeVignette"), CoreKitBasicUIColor::Hex(0x46000C, 0.32f)), TEXT("LowHealthDark"));
	CoreKitBasicUIBuild::Fill(Root, LowDark);
	LowVignette = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_EdgeVignette"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Red, 0.48f)), TEXT("LowHealthVignette"));
	CoreKitBasicUIBuild::Fill(Root, LowVignette);
	LowDark->SetRenderOpacity(0.f);
	LowVignette->SetRenderOpacity(0.f);

	// Damage flash: inset 0 0 200px 40px rgba(255,59,78,.7).
	Flash = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_EdgeVignette"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Red, 0.7f)), TEXT("DamageFlash"));
	CoreKitBasicUIBuild::Fill(Root, Flash);
	Flash->SetRenderOpacity(0.f);

	// Dash speed lines: 2 px lines every 48 px, transparent in the centre.
	SpeedLines = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("SpeedLines"));
	CoreKitBasicUIBuild::Fill(Root, SpeedLines);
	for (int32 Row = 0; Row < 23; ++Row)
	{
		const float Y = (46.f + 48.f * Row) / 1080.f;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			FSlateBrush Brush = UCoreKitBasicUIStyle::Image(TEXT("T_UI_GradH"), CoreKitBasicUIColor::WhiteA(0.55f));
			Brush.Mirroring = Side == 0 ? ESlateBrushMirrorType::Horizontal : ESlateBrushMirrorType::NoMirror;
			UImage* Line = CoreKitBasicUIBuild::Image(WidgetTree, Brush);
			UCanvasPanelSlot* LineSlot = SpeedLines->AddChildToCanvas(Line);
			LineSlot->SetAnchors(Side == 0 ? FAnchors(0.f, Y, 0.38f, Y) : FAnchors(0.62f, Y, 1.f, Y));
			LineSlot->SetOffsets(FMargin(0.f, -1.f, 0.f, 2.f));
			Line->SetRenderTransformPivot(FVector2D(Side == 0 ? 0.f : 1.f, 0.5f));
		}
	}
	SpeedLines->SetRenderOpacity(0.f);
	LowAmount.Snap(0.f);
	BossAmount.Snap(0.f);
}

void UCoreKitBasicUIHudScreenFx::PlayDamageFlash()
{
	FlashTimer.Start(CoreKitBasicUIHudTiming::DamageFlash);
}

void UCoreKitBasicUIHudScreenFx::PlaySpeedLines()
{
	if (!IsReducedMotion())
	{
		Speed.Start(CoreKitBasicUIHudTiming::SpeedLines);
	}
}

void UCoreKitBasicUIHudScreenFx::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIVitalsViewModel* Vitals = Ctx ? Ctx->GetVitals() : nullptr;
	const UCoreKitBasicUIRunViewModel* Run = Ctx ? Ctx->GetRun() : nullptr;
	const UCoreKitBasicUIBossViewModel* Boss = Ctx ? Ctx->GetBoss() : nullptr;

	if (Vitals)
	{
		const float MaxHealth = FMath::Max(Vitals->MaxHealth, 1.f);
		const bool bNowLow = Vitals->Health > 0.f && (Vitals->bLowHealth || Vitals->Health / MaxHealth < CoreKitBasicUIHudTiming::LowHealthFraction);
		if (bNowLow != bLow)
		{
			bLow = bNowLow;
			LowAmount.Start(LowAmount.Value, bLow ? 1.f : 0.f, bLow ? ECoreKitBasicUIMotion::Settle : ECoreKitBasicUIMotion::Exit);
		}
	}
	LowAmount.Tick(Dt);
	PulseTime += Dt;
	const float Period = Token(ECoreKitBasicUIMotion::Pulse);
	const float Pulse = CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Pulse, FMath::Fmod(PulseTime, Period) / Period);
	LowVignette->SetRenderOpacity(LowAmount.Value * (0.35f + 0.65f * Pulse));
	LowDark->SetRenderOpacity(LowAmount.Value);

	if (FlashTimer.Tick(Dt))
	{
		const float A = FlashTimer.Alpha();
		const float Peak = CoreKitBasicUIHudTiming::DamageFlashPeak;
		const float Opacity = A < Peak ? A / Peak : 1.f - Ease(ECoreKitBasicUIMotion::Snap, Seg(A, Peak, 1.f));
		Flash->SetRenderOpacity(FlashTimer.IsRunning() ? Opacity : 0.f);
	}

	if (Speed.Tick(Dt))
	{
		// opacity 0 -> .9 at 30 % -> 0; lines stretch 0.6 -> 1.3 (ease-out).
		const float A = Speed.Alpha();
		const float E = Ease(ECoreKitBasicUIMotion::Settle, A);
		SpeedLines->SetRenderOpacity(Speed.IsRunning() ? (A < 0.3f ? 0.9f * A / 0.3f : 0.9f * (1.f - Seg(A, 0.3f, 1.f))) : 0.f);
		const float ScaleX = FMath::Lerp(0.6f, 1.3f, E);
		for (UWidget* Line : SpeedLines->GetAllChildren())
		{
			FWidgetTransform Transform;
			Transform.Scale = FVector2D(ScaleX, 1.f);
			Line->SetRenderTransform(Transform);
		}
	}

	const bool bNowBoss = (Run && Run->Phase == ECoreKitBasicUIRunPhase::Boss) || (Boss && Boss->bVisible);
	if (bNowBoss != bBoss)
	{
		bBoss = bNowBoss;
		BossAmount.Start(BossAmount.Value, bBoss ? 1.f : 0.f, ECoreKitBasicUIMotion::Deal, CoreKitBasicUIHudTiming::BossBarGrow);
	}
	BossAmount.Tick(Dt);
	BossTint->SetRenderOpacity(BossAmount.Value);
	BossTintTop->SetRenderOpacity(BossAmount.Value);
}

// ---------------------------------------------------------------------------------------------------------------
// Pickup feed

void UCoreKitBasicUIHudPickupFeed::BuildTree()
{
	Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("PickupFeed"));
	WidgetTree->RootWidget = Column;
	for (int32 Index = 0; Index < CoreKitBasicUIHudTiming::FeedMaxLines; ++Index)
	{
		UOverlay* Line = CoreKitBasicUIBuild::Overlay(WidgetTree, *FString::Printf(TEXT("FeedLine%d"), Index));
		CoreKitBasicUIBuild::AddO(Line, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Chamfer(6, CoreKitBasicUIColor::SootA(0.7f))));
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddO(Line, Row, HAlign_Left, VAlign_Center, FMargin(10.f, 0.f, 12.f, 0.f));
		UImage* Heal = CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_Icon_Cross"), CoreKitBasicUIColor::Mint, 16.f);
		UImage* XP = CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_Icon_Diamond"), CoreKitBasicUIColor::Lime, 14.f);
		CoreKitBasicUIBuild::AddH(Row, Heal, FMargin(0.f, 0.f, 8.f, 0.f));
		CoreKitBasicUIBuild::AddH(Row, XP, FMargin(0.f, 0.f, 8.f, 0.f));
		UTextBlock* Text = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), D(20.f, 700, CoreKitBasicUIColor::Lime));
		CoreKitBasicUIBuild::AddH(Row, Text);
		UTextBlock* Count = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), M(11.f, 500, CoreKitBasicUIColor::TextDim, 0.f));
		CoreKitBasicUIBuild::AddH(Row, Count, FMargin(8.f, 0.f, 0.f, 0.f));
		USizeBox* Box = CoreKitBasicUIBuild::Box(WidgetTree, Line, 0.f, 28.f);
		CoreKitBasicUIBuild::AddV(Column, Box, FMargin(0.f, Index == 0 ? 0.f : 4.f, 0.f, 0.f), HAlign_Left);
		Show(Box, false);
		LineWidgets.Add(Box);
		LineTexts.Add(Text);
		LineCounts.Add(Count);
		LineHeal.Add(Heal);
		LineXP.Add(XP);
	}
}

void UCoreKitBasicUIHudPickupFeed::AddPickup(ECoreKitBasicUIPickupKind Kind, float Amount)
{
	Model.Add(Kind, Amount);
}

void UCoreKitBasicUIHudPickupFeed::TickMotion(float Dt)
{
	Model.Tick(Dt);
	const float Motion = MotionAmount();
	for (int32 Index = 0; Index < LineWidgets.Num(); ++Index)
	{
		const bool bUsed = Model.Lines.IsValidIndex(Index);
		Show(LineWidgets[Index], bUsed);
		if (!bUsed)
		{
			continue;
		}
		const FCoreKitBasicUIPickupFeedLine& Line = Model.Lines[Index];
		const bool bHeal = Line.Kind == ECoreKitBasicUIPickupKind::Heal;
		const FLinearColor Color = bHeal ? CoreKitBasicUIColor::Mint : CoreKitBasicUIColor::Lime;
		SetTextIfChanged(LineTexts[Index], FCoreKitBasicUIPickupFeedModel::LineText(Line));
		SetTextColor(LineTexts[Index], Color);
		SetTextIfChanged(LineCounts[Index], Line.Count > 1 ? FText::FromString(FString::Printf(TEXT("×%d"), Line.Count)) : FText::GetEmpty());
		Show(LineCounts[Index], Line.Count > 1);
		Show(LineHeal[Index], bHeal);
		Show(LineXP[Index], !bHeal);
		// Enter 24 px from the left in 200 ms SETTLE, hold, fade from 82 % of the 1.8 s life.
		const float Enter = Ease(ECoreKitBasicUIMotion::Settle, FMath::Clamp(Line.Age / CoreKitBasicUIHudTiming::FeedEnter, 0.f, 1.f));
		const float Fade = 1.f - Seg(Line.Age / CoreKitBasicUIHudTiming::FeedLife, CoreKitBasicUIHudTiming::FeedFadeFrom, 1.f);
		const float Oldest = (Index == 0 && Model.Lines.Num() > 1) ? 0.7f : 1.f;
		LineWidgets[Index]->SetRenderOpacity(Enter * Fade * Oldest);
		SetXform(LineWidgets[Index], FVector2D(-24.f * (1.f - Enter) * Motion, 0.f));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Banner slot: stage change + boss inbound countdown

void UCoreKitBasicUIHudBanner::BuildTree()
{
	UOverlay* Root = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("BannerStack"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Root, 520.f, 0.f, TEXT("Banner"));

	// Stage banner: 4 px tape (440) over a 40 px plate.
	UVerticalBox* Stage = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("StageBanner"));
	StageRoot = Stage;
	CoreKitBasicUIBuild::AddO(Root, Stage, HAlign_Center, VAlign_Top);
	UOverlay* Tape = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("StageTape"));
	CoreKitBasicUIBuild::AddO(Tape, Solid(WidgetTree, CoreKitBasicUIColor::Soot));
	StageTapeStripes = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Tape(8, CoreKitBasicUIColor::Magenta), TEXT("StageTapeStripes"));
	CoreKitBasicUIBuild::AddO(Tape, StageTapeStripes);
	Tape->SetClipping(EWidgetClipping::ClipToBounds);
	Tape->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	StageTape = Tape;
	CoreKitBasicUIBuild::AddV(Stage, CoreKitBasicUIBuild::Box(WidgetTree, Tape, 440.f, 4.f), FMargin(0.f), HAlign_Center);
	UBorder* Plate = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(0.82f)), FMargin(22.f, 0.f), TEXT("StagePlate"));
	Plate->SetVerticalAlignment(VAlign_Center);
	StagePlate = Plate;
	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
	Plate->SetContent(Row);
	StageNumber = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), D(28.f, 800, CoreKitBasicUIColor::Magenta), TEXT("StageNumber"));
	CoreKitBasicUIBuild::AddH(Row, StageNumber, FMargin(0.f, 0.f, 14.f, 0.f));
	StageName = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), D(24.f, 800), TEXT("StageName"));
	CoreKitBasicUIBuild::AddH(Row, StageName, FMargin(0.f, 0.f, 14.f, 0.f));
	StageSubtitle = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), M(10.f, 500, CoreKitBasicUIColor::TextDim, 0.16f), TEXT("StageSubtitle"));
	CoreKitBasicUIBuild::AddH(Row, StageSubtitle);
	USizeBox* PlateBox = CoreKitBasicUIBuild::Box(WidgetTree, Plate, 0.f, 40.f);
	PlateBox->SetMaxDesiredWidth(440.f);
	CoreKitBasicUIBuild::AddV(Stage, PlateBox, FMargin(0.f), HAlign_Center);
	Show(StageRoot, false);

	// Boss inbound: 520 x 44, 22 px scrolling tapes, WARNING · HEAVY MECH INBOUND 00:0n.
	UOverlay* Inbound = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("BossInbound"));
	InboundRoot = Inbound;
	CoreKitBasicUIBuild::AddO(Root, CoreKitBasicUIBuild::Box(WidgetTree, Inbound, 520.f, 44.f), HAlign_Center, VAlign_Top);
	CoreKitBasicUIBuild::AddO(Inbound, Solid(WidgetTree, CoreKitBasicUIColor::SootA(0.88f)));
	UHorizontalBox* InboundRow = CoreKitBasicUIBuild::Row(WidgetTree);
	CoreKitBasicUIBuild::AddO(Inbound, InboundRow);
	auto AddTape = [this, InboundRow]()
	{
		UCanvasPanel* Clip = CoreKitBasicUIBuild::Canvas(WidgetTree);
		Clip->SetClipping(EWidgetClipping::ClipToBounds);
		CoreKitBasicUIBuild::Place(Clip, Solid(WidgetTree, CoreKitBasicUIColor::Soot), FAnchors(0.f, 0.f), FVector2D::ZeroVector, FVector2D(22.f, 44.f));
		UImage* Stripes = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Tape(8, CoreKitBasicUIColor::Orange));
		CoreKitBasicUIBuild::Place(Clip, Stripes, FAnchors(0.f, 0.f), FVector2D(-23.f, 0.f), FVector2D(68.f, 44.f), FVector2D::ZeroVector, 1);
		InboundTapes.Add(Stripes);
		CoreKitBasicUIBuild::AddH(InboundRow, CoreKitBasicUIBuild::Box(WidgetTree, Clip, 22.f, 44.f), FMargin(0.f), VAlign_Fill);
	};
	AddTape();
	UHorizontalBox* Center = CoreKitBasicUIBuild::Row(WidgetTree);
	UOverlay* CenterBox = CoreKitBasicUIBuild::Overlay(WidgetTree);
	CoreKitBasicUIBuild::AddO(CenterBox, Center, HAlign_Center, VAlign_Center);
	CoreKitBasicUIBuild::AddH(InboundRow, CenterBox, FMargin(0.f), VAlign_Fill, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddH(Center, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Warning", "WARNING"), M(10.f, 500, CoreKitBasicUIColor::Orange, 0.24f)), FMargin(0.f, 0.f, 12.f, 0.f));
	CoreKitBasicUIBuild::AddH(Center, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("MechInbound", "HEAVY MECH INBOUND"), D(24.f, 800)), FMargin(0.f, 0.f, 12.f, 0.f));
	InboundCount = CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(TEXT("00:10")), D(24.f, 700, CoreKitBasicUIColor::Orange), TEXT("InboundCount"));
	CoreKitBasicUIBuild::AddH(Center, InboundCount);
	AddTape();
	Show(InboundRoot, false);
}

void UCoreKitBasicUIHudBanner::QueueStage(int32 StageIndex, const FText& Name, const FText& Subtitle)
{
	// Banners at 2:00, 5:00, 8:00 only; each stage once.
	if (StageIndex <= 0 || StageIndex <= LastQueuedStage)
	{
		return;
	}
	LastQueuedStage = StageIndex;
	FQueued Entry;
	Entry.Index = StageIndex;
	Entry.Name = Name;
	Entry.Subtitle = Subtitle;
	Queue.Add(Entry);
}

void UCoreKitBasicUIHudBanner::ShowStage(int32 StageIndex, const FText& Name, const FText& Subtitle)
{
	static const TCHAR* DefaultNames[] = { TEXT("INTRO"), TEXT("PRESSURE"), TEXT("SURVIVAL"), TEXT("CHAOS") };
	static const TCHAR* DefaultSubtitles[] = { TEXT(""), TEXT("RANGED DRONES INBOUND"), TEXT(""), TEXT("MAX DRONE DENSITY") };
	const int32 Safe = FMath::Clamp(StageIndex, 0, 3);
	const FLinearColor Color = Safe == 3 ? CoreKitBasicUIColor::Orange : CoreKitBasicUIColor::Magenta;
	StageNumber->SetText(CoreKitBasicUIHudLogic::StageNumber2(StageIndex));
	SetTextColor(StageNumber, Color);
	StageName->SetText(Name.IsEmpty() ? FText::FromString(DefaultNames[Safe]) : Name.ToUpper());
	const FText Sub = Subtitle.IsEmpty() ? FText::FromString(DefaultSubtitles[Safe]) : Subtitle.ToUpper();
	StageSubtitle->SetText(Sub);
	Show(StageSubtitle, !Sub.IsEmpty());
	StageTapeStripes->SetBrush(UCoreKitBasicUIStyle::Tape(8, Color));
	Show(StageRoot, true);
	StageLife.Start(CoreKitBasicUIHudTiming::StageBanner);
	PlayHudCue(TEXT("HUD.StageChange"));
	CoreKitBasicUIHudUtil::SetAccessibleText(StageRoot, FText::Format(COREKIT_BASICUI_TEXT("StageA11y", "Stage {0} {1}"), FText::AsNumber(StageIndex + 1), StageName->GetText()));
}

void UCoreKitBasicUIHudBanner::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIRunViewModel* Run = Ctx ? Ctx->GetRun() : nullptr;
	const UCoreKitBasicUIProgressionViewModel* Progression = Ctx ? Ctx->GetProgression() : nullptr;
	const float Motion = MotionAmount();
	HeroBusy = FMath::Max(0.f, HeroBusy - Dt);

	if (Run)
	{
		// Fallback when no StageChanged event arrives: the Run ViewModel stage index.
		if (LastRunStage != INDEX_NONE && Run->StageIndex > LastRunStage)
		{
			QueueStage(Run->StageIndex, Run->StageName, Run->StageSubtitle);
		}
		if (LastRunStage == INDEX_NONE || Run->StageIndex < LastRunStage)
		{
			// First frame or a run restarted in the same world: no banner for the current stage.
			Queue.Reset();
			LastQueuedStage = LastRunStage == INDEX_NONE ? FMath::Max(LastQueuedStage, Run->StageIndex) : Run->StageIndex;
		}
		LastRunStage = Run->StageIndex;

		// 09:50 boss countdown replaces the banner slot.
		const bool bNowInbound = Run->BossInboundSeconds >= 0.f || Run->Phase == ECoreKitBasicUIRunPhase::BossInbound;
		if (bNowInbound != bInbound)
		{
			bInbound = bNowInbound;
			InboundTransition.Start(bInbound ? Token(ECoreKitBasicUIMotion::Settle) : Token(ECoreKitBasicUIMotion::Exit));
			bInboundExit = !bInbound;
			if (bInbound)
			{
				Show(InboundRoot, true);
				StageLife.Stop();
				Show(StageRoot, false);
				InboundTime = 0.f;
				CoreKitBasicUIHudUtil::SetAccessibleText(InboundRoot, COREKIT_BASICUI_TEXT("InboundA11y", "Warning, Heavy Mech inbound"));
			}
		}
		if (bInbound)
		{
			SetTextIfChanged(InboundCount, FText::FromString(CoreKitBasicUIHudLogic::FormatClock(FMath::Max(Run->BossInboundSeconds, 0.f))));
		}
	}

	if (bInbound || bInboundExit)
	{
		InboundTime += Dt;
		// Tape scrolls one tile every 0.6 s; the count blinks every 0.5 s (steps: 1 / 0.3).
		const float Scroll = FMath::Fmod(InboundTime, CoreKitBasicUIHudTiming::BossTapeScroll) / CoreKitBasicUIHudTiming::BossTapeScroll;
		for (UImage* Tape : InboundTapes)
		{
			SetXform(Tape, FVector2D(23.f * Scroll, 0.f));
		}
		InboundCount->SetRenderOpacity(FMath::Fmod(InboundTime, CoreKitBasicUIHudTiming::BossInboundBlink) < CoreKitBasicUIHudTiming::BossInboundBlink * 0.5f ? 1.f : 0.3f);
		InboundTransition.Tick(Dt);
		const float A = InboundTransition.Alpha();
		if (bInbound)
		{
			const float E = Ease(ECoreKitBasicUIMotion::Settle, A);
			InboundRoot->SetRenderOpacity(E);
			SetXform(InboundRoot, FVector2D(0.f, -12.f * (1.f - E) * Motion));
		}
		else
		{
			const float E = Ease(ECoreKitBasicUIMotion::Exit, A);
			InboundRoot->SetRenderOpacity(1.f - E);
			SetXform(InboundRoot, FVector2D(0.f, -8.f * E * Motion));
			if (!InboundTransition.IsRunning())
			{
				bInboundExit = false;
				Show(InboundRoot, false);
			}
		}
	}

	// Stage banners wait for LEVEL UP / Card Draw and for the boss countdown (one hero at a time).
	const bool bBlocked = bInbound || HeroBusy > 0.f || (Progression && Progression->bOfferOpen);
	if (!StageLife.IsRunning() && !bBlocked && Queue.Num() > 0)
	{
		const FQueued Next = Queue[0];
		Queue.RemoveAt(0);
		ShowStage(Next.Index, Next.Name, Next.Subtitle);
	}
	if (StageLife.Tick(Dt))
	{
		// Tape wipes left to right in the first 10 %; drop 12 px in 0-8 %, hold, exit up 8 px in 88-100 %.
		const float A = StageLife.Alpha();
		const float In = Ease(ECoreKitBasicUIMotion::Settle, Seg(A, 0.f, 0.08f));
		const float Out = Ease(ECoreKitBasicUIMotion::Exit, Seg(A, 0.88f, 1.f));
		StageRoot->SetRenderOpacity(In * (1.f - Out));
		SetXform(StageRoot, FVector2D(0.f, (-12.f * (1.f - In) - 8.f * Out) * Motion));
		FWidgetTransform TapeTransform;
		TapeTransform.Scale = FVector2D(FMath::Max(Ease(ECoreKitBasicUIMotion::Settle, Seg(A, 0.f, 0.1f)), 0.001f), 1.f);
		StageTape->SetRenderTransform(TapeTransform);
		if (!StageLife.IsRunning())
		{
			Show(StageRoot, false);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Boss attack warning

void UCoreKitBasicUIHudBossWarning::BuildTree()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("BossWarning"));
	Content = Column;
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Column, 440.f, 0.f, TEXT("BossWarningBox"));
	UOverlay* Plate = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("WarningPlate"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Plate, 0.f, 44.f), FMargin(0.f), HAlign_Center);
	CoreKitBasicUIBuild::AddO(Plate, Solid(WidgetTree, CoreKitBasicUIColor::SootA(0.86f)));
	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
	CoreKitBasicUIBuild::AddO(Plate, Row);
	CoreKitBasicUIBuild::AddH(Row, TapeStrip(WidgetTree, 8, CoreKitBasicUIColor::Orange, 16.f, 44.f), FMargin(0.f), VAlign_Fill);
	UHorizontalBox* Text = CoreKitBasicUIBuild::Row(WidgetTree);
	CoreKitBasicUIBuild::AddH(Row, Text, FMargin(18.f, 0.f), VAlign_Center);
	AttackText = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), D(28.f, 800, CoreKitBasicUIColor::Orange), TEXT("AttackName"));
	CoreKitBasicUIBuild::AddH(Text, AttackText, FMargin(0.f, 0.f, 12.f, 0.f));
	HintText = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), M(10.f, 500, CoreKitBasicUIColor::White, 0.18f), TEXT("AttackHint"));
	CoreKitBasicUIBuild::AddH(Text, HintText);
	CoreKitBasicUIBuild::AddH(Row, TapeStrip(WidgetTree, 8, CoreKitBasicUIColor::Orange, 16.f, 44.f), FMargin(0.f), VAlign_Fill);

	UOverlay* Bar = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("TelegraphBar"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Bar, 260.f, 4.f), FMargin(0.f, 6.f, 0.f, 0.f), HAlign_Center);
	CoreKitBasicUIBuild::AddO(Bar, Solid(WidgetTree, CoreKitBasicUIColor::SootA(0.7f)));
	Progress = Solid(WidgetTree, CoreKitBasicUIColor::Orange, TEXT("TelegraphProgress"));
	CoreKitBasicUIBuild::AddO(Bar, Progress);
	SetFillX(Progress, 0.f);
	SetRenderOpacity(0.f);
	Show(Content, false);
}

void UCoreKitBasicUIHudBossWarning::OnTelegraph(const FText& AttackName, const FText& Hint, float Duration)
{
	EventName = AttackName;
	EventHint = Hint;
	EventLife.Start(FMath::Max(Duration, 0.3f));
}

void UCoreKitBasicUIHudBossWarning::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIBossViewModel* Boss = Ctx ? Ctx->GetBoss() : nullptr;
	EventLife.Tick(Dt);
	const bool bLive = Boss && Boss->bAttackActive;
	const bool bWant = bLive || EventLife.IsRunning();
	if (bWant != bShown)
	{
		bShown = bWant;
		Transition.Start(bShown ? Token(ECoreKitBasicUIMotion::Snap) : Token(ECoreKitBasicUIMotion::Exit));
		if (bShown)
		{
			Show(Content, true);
		}
	}
	if (bShown)
	{
		const FText Name = bLive && !Boss->AttackName.IsEmpty() ? Boss->AttackName : EventName;
		const FText Hint = bLive && !Boss->AttackHint.IsEmpty() ? Boss->AttackHint : EventHint;
		SetTextIfChanged(AttackText, Name.IsEmpty() ? COREKIT_BASICUI_TEXT("MechAttack", "MECH ATTACK") : Name.ToUpper());
		SetTextIfChanged(HintText, Hint.IsEmpty() ? COREKIT_BASICUI_TEXT("GetClear", "GET CLEAR") : Hint.ToUpper());
		SetFillX(Progress, bLive ? Boss->TelegraphProgress : EventLife.Alpha());
	}
	if (Transition.Tick(Dt))
	{
		const float A = Transition.Alpha();
		SetRenderOpacity(bShown ? Ease(ECoreKitBasicUIMotion::Snap, A) : 1.f - Ease(ECoreKitBasicUIMotion::Exit, A));
		if (!bShown && !Transition.IsRunning())
		{
			Show(Content, false);
		}
		else if (bShown && A <= KINDA_SMALL_NUMBER)
		{
			CoreKitBasicUIHudUtil::SetAccessibleText(Content, FText::Format(COREKIT_BASICUI_TEXT("WarningA11y", "Alert: {0}. {1}"), AttackText->GetText(), HintText->GetText()));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Off-screen lock-on

void UCoreKitBasicUIHudThreatIndicator::BuildTree()
{
	const float Width = 196.f;
	const float Point = 14.f / Width;
	UOverlay* Stack = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("LockOnPlate"));
	Plate = Stack;
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Stack, Width, 40.f, TEXT("LockOn"));
	UCoreKitBasicUIHudShape* Shape = WidgetTree->ConstructWidget<UCoreKitBasicUIHudShape>(UCoreKitBasicUIHudShape::StaticClass(), TEXT("LockOnShape"));
	Shape->SetShapeColor(CoreKitBasicUIColor::SootA(0.72f));
	// The plate points away from its edge (right edge: point on the left), as in HUDCombat.
	Shape->SetPoints(bRightEdge
		? TArray<FVector2D>{ FVector2D(Point, 0.f), FVector2D(1.f, 0.f), FVector2D(1.f, 1.f), FVector2D(Point, 1.f), FVector2D(0.f, 0.5f) }
		: TArray<FVector2D>{ FVector2D(0.f, 0.f), FVector2D(1.f - Point, 0.f), FVector2D(1.f, 0.5f), FVector2D(1.f - Point, 1.f), FVector2D(0.f, 1.f) });
	CoreKitBasicUIBuild::AddO(Stack, Shape);

	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
	CoreKitBasicUIBuild::AddO(Stack, Row, bRightEdge ? HAlign_Right : HAlign_Left, VAlign_Center, bRightEdge ? FMargin(18.f, 0.f, 14.f, 0.f) : FMargin(14.f, 0.f, 18.f, 0.f));
	UOverlay* RingStack = CoreKitBasicUIBuild::Overlay(WidgetTree);
	Ring = MakeArc(WidgetTree, TEXT("LockOnRing"), 4.f, CoreKitBasicUIColor::Orange);
	Ring->SetTrackColor(CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Orange, 0.2f));
	CoreKitBasicUIBuild::AddO(RingStack, Ring);
	CoreKitBasicUIBuild::AddO(RingStack, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Circle"), CoreKitBasicUIColor::Soot, FVector2D(16.f, 16.f))), HAlign_Center, VAlign_Center);
	USizeBox* RingBox = CoreKitBasicUIBuild::Box(WidgetTree, RingStack, 24.f, 24.f);
	UTextBlock* Label = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("LockingOn", "LOCKING ON"), M(11.f, 700, CoreKitBasicUIColor::Orange, 0.16f), TEXT("LockOnLabel"));
	FSlateBrush ChevronBrush = UCoreKitBasicUIStyle::Image(TEXT("T_Icon_Chevrons"), CoreKitBasicUIColor::Orange, FVector2D(20.f, 20.f));
	ChevronBrush.Mirroring = bRightEdge ? ESlateBrushMirrorType::NoMirror : ESlateBrushMirrorType::Horizontal;
	UImage* Chevron = CoreKitBasicUIBuild::Image(WidgetTree, ChevronBrush, TEXT("LockOnChevron"));
	if (bRightEdge)
	{
		CoreKitBasicUIBuild::AddH(Row, RingBox, FMargin(0.f, 0.f, 10.f, 0.f));
		CoreKitBasicUIBuild::AddH(Row, Label, FMargin(0.f, 0.f, 10.f, 0.f));
		CoreKitBasicUIBuild::AddH(Row, Chevron);
	}
	else
	{
		CoreKitBasicUIBuild::AddH(Row, Chevron, FMargin(0.f, 0.f, 10.f, 0.f));
		CoreKitBasicUIBuild::AddH(Row, Label, FMargin(0.f, 0.f, 10.f, 0.f));
		CoreKitBasicUIBuild::AddH(Row, RingBox);
	}
	SetRenderOpacity(0.f);
	Show(Plate, false);
}

void UCoreKitBasicUIHudThreatIndicator::SetThreat(AActor* Source, bool bLocking)
{
	if (!Source)
	{
		return;
	}
	if (bLocking)
	{
		Sources.AddUnique(Source);
	}
	else
	{
		Sources.Remove(Source);
	}
}

void UCoreKitBasicUIHudThreatIndicator::TickMotion(float Dt)
{
	Sources.RemoveAll([](const TWeakObjectPtr<AActor>& Source) { return !Source.IsValid(); });
	bool bWant = false;
	APlayerController* Controller = GetOwningPlayer();
	if (Controller && Sources.Num() > 0)
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		int32 ViewX = 0;
		int32 ViewY = 0;
		Controller->GetViewportSize(ViewX, ViewY);
		for (const TWeakObjectPtr<AActor>& Source : Sources)
		{
			const FVector Location = Source->GetActorLocation();
			FVector2D Screen;
			const bool bProjected = Controller->ProjectWorldLocationToScreen(Location, Screen, true);
			const bool bOnScreen = bProjected && Screen.X >= 0.f && Screen.Y >= 0.f && Screen.X <= ViewX && Screen.Y <= ViewY;
			if (bOnScreen)
			{
				continue;
			}
			const float Relative = CoreKitBasicUIHudLogic::RelativeDirectionDeg(FVector2D(ViewLocation), ViewRotation.Yaw, FVector2D(Location));
			if (IsRightSide(Relative) == bRightEdge)
			{
				bWant = true;
				break;
			}
		}
	}
	if (bWant != bShown)
	{
		bShown = bWant;
		Transition.Start(bShown ? Token(ECoreKitBasicUIMotion::Settle) : Token(ECoreKitBasicUIMotion::Exit));
		if (bShown)
		{
			Show(Plate, true);
		}
	}
	if (Transition.Tick(Dt))
	{
		// Lateral entry: 24 px inward from its edge.
		const float A = Transition.Alpha();
		const float Visible = bShown ? Ease(ECoreKitBasicUIMotion::Settle, A) : 1.f - Ease(ECoreKitBasicUIMotion::Exit, A);
		SetRenderOpacity(Visible);
		SetXform(Plate, FVector2D((bRightEdge ? 24.f : -24.f) * (1.f - Visible) * MotionAmount(), 0.f));
		if (!bShown && !Transition.IsRunning())
		{
			Show(Plate, false);
		}
	}
	if (bShown)
	{
		// Charge ring: no per-drone progress is published, so it fills over one PULSE period and repeats.
		PulseTime += Dt;
		const float Period = Token(ECoreKitBasicUIMotion::Pulse);
		Ring->SetProgress(FMath::Fmod(PulseTime, Period) / Period);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Weapon swap banner

void UCoreKitBasicUIHudWeaponSwap::BuildTree()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("WeaponSwap"));
	Content = Column;
	WidgetTree->RootWidget = Column;
	SlotText = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), Sh(M(10.f, 500, CoreKitBasicUIColor::White, 0.2f)), TEXT("SwapSlot"));
	CoreKitBasicUIBuild::AddV(Column, SlotText, FMargin(0.f), HAlign_Right);
	UHorizontalBox* FrameRow = CoreKitBasicUIBuild::Row(WidgetTree);
	NameFrame = CoreKitBasicUIBuild::Box(WidgetTree, FrameRow, 200.f, 40.f, TEXT("SwapNameFrame"));
	CoreKitBasicUIBuild::AddV(Column, NameFrame, FMargin(0.f), HAlign_Right);
	NameText = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), Sh(D(38.f, 800)), TEXT("SwapName"));
	NameClip = CoreKitBasicUIBuild::Box(WidgetTree, NameText, 0.f, 40.f, TEXT("SwapNameClip"));
	NameClip->SetClipping(EWidgetClipping::ClipToBounds);
	CoreKitBasicUIBuild::AddH(FrameRow, NameClip, FMargin(0.f), VAlign_Fill);
	UOverlay* SlashStack = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("SwapSlash"));
	SlashGlow = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::White), TEXT("SwapSlashGlow"));
	CoreKitBasicUIBuild::AddO(SlashStack, SlashGlow, HAlign_Fill, VAlign_Fill, FMargin(-10.f, -10.f));
	Slash = Solid(WidgetTree, CoreKitBasicUIColor::White, TEXT("SwapSlashLine"));
	CoreKitBasicUIBuild::AddO(SlashStack, Slash);
	SlashStack->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, SlashStack, 260.f, 3.f), FMargin(0.f, 2.f, 0.f, 0.f), HAlign_Right);
	SetRenderOpacity(0.f);
	Show(Content, false);
}

void UCoreKitBasicUIHudWeaponSwap::Play(int32 SlotNumber, const FText& LongName, bool bMelee)
{
	if (SlotNumber == LastShownSlot && Clock - LastShownClock < 0.3)
	{
		return;
	}
	LastShownSlot = SlotNumber;
	LastShownClock = Clock;
	const FLinearColor Color = bMelee ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::White;
	const FText Name = LongName.IsEmpty() ? (bMelee ? COREKIT_BASICUI_TEXT("EnergyKatana", "ENERGY KATANA") : COREKIT_BASICUI_TEXT("AssaultRifle", "ASSAULT RIFLE")) : LongName.ToUpper();
	SlotText->SetText(FText::Format(COREKIT_BASICUI_TEXT("Equipped", "{0} · EQUIPPED"), FText::AsNumber(SlotNumber)));
	NameText->SetText(Name);
	SetTextColor(NameText, Color);
	Slash->SetColorAndOpacity(Color);
	SlashGlow->SetColorAndOpacity(CoreKitBasicUIColor::WithAlpha(Color, 0.6f));
	NameWidth = FMath::CeilToFloat(CoreKitBasicUIHudUtil::MeasureText(Name, UCoreKitBasicUIStyle::Font(ECoreKitBasicUIFontFamily::Display, 800, CoreKitBasicUIBuild::Px(38.f)))) + 4.f;
	NameFrame->SetWidthOverride(NameWidth);
	Show(Content, true);
	Life.Start(CoreKitBasicUIHudTiming::SwapName);
}

void UCoreKitBasicUIHudWeaponSwap::TickMotion(float Dt)
{
	Clock += Dt;
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUILoadoutViewModel* Loadout = Ctx ? Ctx->GetLoadout() : nullptr;
	if (Loadout)
	{
		int32 Active = Loadout->Slots.IndexOfByPredicate([](const FCoreKitBasicUIWeaponSlotData& Data) { return Data.bActive; });
		if (Active == INDEX_NONE)
		{
			Active = Loadout->ActiveSlot;
		}
		if (LastActive != INDEX_NONE && Active != LastActive && Loadout->Slots.IsValidIndex(Active))
		{
			const FCoreKitBasicUIWeaponSlotData& Data = Loadout->Slots[Active];
			Play(Data.SlotNumber, Data.LongName.IsEmpty() ? Data.ShortName : Data.LongName, Data.bMelee || Loadout->bMeleeActive);
		}
		LastActive = Active;
	}
	if (Life.Tick(Dt))
	{
		// Name: revealed left to right in the first 18 % (SETTLE), holds, fades from 75 %; slash 500 ms.
		const float A = Life.Alpha();
		const float Reveal = Ease(ECoreKitBasicUIMotion::Settle, Seg(A, 0.f, 0.18f));
		const float Fade = 1.f - Seg(A, 0.75f, 1.f);
		NameClip->SetWidthOverride(FMath::Max(NameWidth * Reveal, 0.01f));
		SetRenderOpacity(Fade);
		const float SlashA = FMath::Clamp(Life.Time / CoreKitBasicUIHudTiming::SwapSlash, 0.f, 1.f);
		if (UWidget* SlashStack = Slash->GetParent())
		{
			FWidgetTransform Transform;
			Transform.Scale = FVector2D(FMath::Max(Ease(ECoreKitBasicUIMotion::Settle, Seg(SlashA, 0.f, 0.35f)), 0.001f), 1.f);
			SlashStack->SetRenderTransform(Transform);
			SlashStack->SetRenderOpacity(1.f - Seg(SlashA, 0.35f, 1.f));
		}
		if (!Life.IsRunning())
		{
			Show(Content, false);
		}
	}
}

#undef LOCTEXT_NAMESPACE
