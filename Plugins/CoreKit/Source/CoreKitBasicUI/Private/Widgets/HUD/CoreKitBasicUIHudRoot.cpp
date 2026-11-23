#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudRoot.h"

#include "Components/AudioComponent.h"
#include "Widgets/HUD/CoreKitBasicUIHudCommon.h"
#include "CoreKitBasicUI/Effects/CoreKitBasicUIScreenEffectsComponent.h"
#include "CoreKitBasicUI/Events/CoreKitBasicUIEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "StructUtils/InstancedStruct.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudBlocks.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudBuild.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudEvents.h"

using namespace CoreKitBasicUIHudBuild;

namespace
{
	// Title safe 90 %: 96 / 54 px at 1920 x 1080 (same margins as UCoreKitBasicUIPrimaryLayout's SafeCanvas).
	const FMargin SafeInset(96.f, 54.f, 96.f, 54.f);
}

void UCoreKitBasicUIHudRoot::Setup(UCanvasPanel* InScreenCanvas, UCanvasPanel* InSafeCanvas)
{
	LayoutScreenCanvas = InScreenCanvas;
	LayoutSafeCanvas = InSafeCanvas;
	// The HUD never takes clicks or focus.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

template <typename T>
T* UCoreKitBasicUIHudRoot::AddBlock(UCanvasPanel* Layer, FName Name, FVector2D Anchor, FVector2D Position, bool bScaled, int32 ZOrder)
{
	T* Block = WidgetTree->ConstructWidget<T>(T::StaticClass(), Name);
	// Anchor = alignment = render pivot: the block grows from its corner / edge of the safe area with HUD scale.
	CoreKitBasicUIBuild::Place(Layer, Block, FAnchors(Anchor.X, Anchor.Y), Position, FVector2D::ZeroVector, Anchor, ZOrder);
	Block->SetRenderTransformPivot(Anchor);
	Blocks.Add(Block);
	if (bScaled)
	{
		FScaledBlock Entry;
		Entry.Widget = Block;
		Entry.Position = Position;
		ScaledBlocks.Add(Entry);
	}
	return Block;
}

UImage* UCoreKitBasicUIHudRoot::AddVignette(FVector2D Anchor, FVector2D Extent, float Alpha)
{
	// CSS radial-gradient(ellipse at the corner, rgba(10,10,12,a) 0, transparent 70 %): transparent at ~0.99 x extent.
	UImage* Image = Vignette(WidgetTree, Alpha);
	CoreKitBasicUIBuild::Place(MainLayer, Image, FAnchors(Anchor.X, Anchor.Y), FVector2D::ZeroVector, Extent * 2.83f, FVector2D(0.5f, 0.5f), -10);
	return Image;
}

void UCoreKitBasicUIHudRoot::BuildTree()
{
	RootCanvas = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("HudCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	ScreenFx = WidgetTree->ConstructWidget<UCoreKitBasicUIHudScreenFx>(UCoreKitBasicUIHudScreenFx::StaticClass(), TEXT("ScreenFx"));
	CoreKitBasicUIBuild::Fill(RootCanvas, ScreenFx, FMargin(0.f), 0);
	Blocks.Add(ScreenFx);

	MainLayer = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("MainLayer"));
	CoreKitBasicUIBuild::Fill(RootCanvas, MainLayer, FMargin(0.f), 1);
	HandLayer = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("HandLayer"));
	CoreKitBasicUIBuild::Fill(RootCanvas, HandLayer, FMargin(0.f), 2);

	// Dark radial vignettes behind each block (02-HUD: rgba(10,10,12,.45-.6), no opaque panels).
	AddVignette(FVector2D(0.f, 1.f), FVector2D(680.f, 280.f), 0.6f);
	AddVignette(FVector2D(1.f, 1.f), FVector2D(600.f, 280.f), 0.6f);
	AddVignette(FVector2D(0.5f, 1.f), FVector2D(340.f, 220.f), 0.5f);
	TopCenterVignette = AddVignette(FVector2D(0.5f, 0.f), FVector2D(300.f, 170.f), 0.5f);
	AddVignette(FVector2D(0.f, 0.f), FVector2D(420.f, 170.f), 0.45f);
	AddVignette(FVector2D(1.f, 0.f), FVector2D(420.f, 320.f), 0.45f);

	// Centre of the screen (960, 540).
	DamageArcs = AddBlock<UCoreKitBasicUIHudDamageArcs>(MainLayer, TEXT("DamageArcs"), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector, true, 1);
	Crosshair = AddBlock<UCoreKitBasicUIHudCrosshair>(MainLayer, TEXT("Crosshair"), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector, true, 2);
	// Boss attack warning: x 740-1180, y 620.
	BossWarning = WidgetTree->ConstructWidget<UCoreKitBasicUIHudBossWarning>(UCoreKitBasicUIHudBossWarning::StaticClass(), TEXT("BossWarning"));
	CoreKitBasicUIBuild::Place(MainLayer, BossWarning, FAnchors(0.5f, 0.5f), FVector2D(0.f, 80.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f), 3);
	BossWarning->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
	Blocks.Add(BossWarning);
	ScaledBlocks.Add({ BossWarning.Get(), FVector2D(0.f, 80.f) });

	SafeMain = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("SafeMain"));
	CoreKitBasicUIBuild::Fill(MainLayer, SafeMain, SafeInset, 4);
	KillCounter = AddBlock<UCoreKitBasicUIHudKillCounter>(SafeMain, TEXT("KillCounter"), FVector2D(0.f, 0.f), FVector2D::ZeroVector);
	SurvivedChip = AddBlock<UCoreKitBasicUIHudSurvivedChip>(SafeMain, TEXT("SurvivedChip"), FVector2D(0.f, 0.f), FVector2D(0.f, 48.f));
	Timer = AddBlock<UCoreKitBasicUIHudTimerBlock>(SafeMain, TEXT("Timer"), FVector2D(0.5f, 0.f), FVector2D::ZeroVector);
	BossBar = AddBlock<UCoreKitBasicUIHudBossBar>(SafeMain, TEXT("BossBar"), FVector2D(0.5f, 0.f), FVector2D::ZeroVector);
	Banner = AddBlock<UCoreKitBasicUIHudBanner>(SafeMain, TEXT("Banner"), FVector2D(0.5f, 0.f), FVector2D(0.f, 78.f));
	Minimap = AddBlock<UCoreKitBasicUIHudMinimap>(SafeMain, TEXT("Minimap"), FVector2D(1.f, 0.f), FVector2D::ZeroVector);

	// Off-screen lock-on plates: right 96 / top 476 (40 px tall: centre 44 px above the screen centre).
	ThreatRight = WidgetTree->ConstructWidget<UCoreKitBasicUIHudThreatIndicator>(UCoreKitBasicUIHudThreatIndicator::StaticClass(), TEXT("ThreatRight"));
	ThreatRight->SetRightEdge(true);
	CoreKitBasicUIBuild::Place(SafeMain, ThreatRight, FAnchors(1.f, 0.5f), FVector2D(0.f, -44.f), FVector2D::ZeroVector, FVector2D(1.f, 0.5f));
	ThreatRight->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
	ThreatLeft = WidgetTree->ConstructWidget<UCoreKitBasicUIHudThreatIndicator>(UCoreKitBasicUIHudThreatIndicator::StaticClass(), TEXT("ThreatLeft"));
	ThreatLeft->SetRightEdge(false);
	CoreKitBasicUIBuild::Place(SafeMain, ThreatLeft, FAnchors(0.f, 0.5f), FVector2D(0.f, -44.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f));
	ThreatLeft->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	Blocks.Add(ThreatRight);
	Blocks.Add(ThreatLeft);
	ScaledBlocks.Add({ ThreatRight.Get(), FVector2D(0.f, -44.f) });
	ScaledBlocks.Add({ ThreatLeft.Get(), FVector2D(0.f, -44.f) });

	Health = AddBlock<UCoreKitBasicUIHudHealthBlock>(SafeMain, TEXT("Health"), FVector2D(0.f, 1.f), FVector2D::ZeroVector);
	// Dash ring: left 420, bottom 74.
	Dash = AddBlock<UCoreKitBasicUIHudDashRing>(SafeMain, TEXT("Dash"), FVector2D(0.f, 1.f), FVector2D(324.f, -20.f));
	// Pickup feed and weapon swap banner: bottom 158.
	PickupFeed = AddBlock<UCoreKitBasicUIHudPickupFeed>(SafeMain, TEXT("PickupFeed"), FVector2D(0.f, 1.f), FVector2D(0.f, -104.f));
	Weapon = AddBlock<UCoreKitBasicUIHudWeaponBlock>(SafeMain, TEXT("Weapon"), FVector2D(1.f, 1.f), FVector2D::ZeroVector);
	WeaponSwap = AddBlock<UCoreKitBasicUIHudWeaponSwap>(SafeMain, TEXT("WeaponSwap"), FVector2D(1.f, 1.f), FVector2D(0.f, -104.f));

	SafeHand = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("SafeHand"));
	CoreKitBasicUIBuild::Fill(HandLayer, SafeHand, SafeInset);
	Hand = AddBlock<UCoreKitBasicUIHudBuildHand>(SafeHand, TEXT("BuildHand"), FVector2D(0.5f, 1.f), FVector2D::ZeroVector);
	// TAB panel: left 700, bottom 150, 520 wide.
	Inspector = AddBlock<UCoreKitBasicUIHudBuildInspector>(SafeHand, TEXT("BuildInspector"), FVector2D(0.5f, 1.f), FVector2D(0.f, -96.f));

	MainDim.Snap(1.f);
	HandDim.Snap(1.f);
	Visible.Snap(1.f);
}

void UCoreKitBasicUIHudRoot::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (const UCoreKitBasicUIContextSubsystem* Context = GetUIContext())
	{
		if (UCoreKitBasicUIEventChannel* Events = Context->GetEvents())
		{
			Channel = Events;
			EventHandle = Events->OnAnyEvent.AddUObject(this, &UCoreKitBasicUIHudRoot::HandleEvent);
		}
	}
}

void UCoreKitBasicUIHudRoot::NativeDestruct()
{
	if (UCoreKitBasicUIEventChannel* Events = Channel.Get())
	{
		Events->OnAnyEvent.Remove(EventHandle);
	}
	Channel.Reset();
	EventHandle.Reset();
	SetLoop(HeartbeatLoop, NAME_None, false);
	SetLoop(AlarmLoop, NAME_None, false);
	Super::NativeDestruct();
}

bool UCoreKitBasicUIHudRoot::HasStaticDash() const
{
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	const UCoreKitBasicUIBuildViewModel* Build = Context ? Context->GetBuild() : nullptr;
	return Build && Build->Cards.ContainsByPredicate([](const FCoreKitBasicUICardData& Card) { return CoreKitBasicUIHudLogic::IsStaticDash(Card.Id); });
}

void UCoreKitBasicUIHudRoot::HandleEvent(FGameplayTag Event, const FInstancedStruct& Payload)
{
	if (!Crosshair)
	{
		return;
	}
	UCoreKitBasicUIScreenEffectsComponent* Effects = UCoreKitBasicUIScreenEffectsComponent::Get(this);
	if (Event == CoreKitBasicUIEvent::Hit)
	{
		if (const FCoreKitBasicUIHitEvent* Hit = Payload.GetPtr<FCoreKitBasicUIHitEvent>())
		{
			Crosshair->PlayHit(Hit->Source);
			if (Hit->bKill)
			{
				Crosshair->PlayKill();
				KillCounter->PlayPop();
				PlayCue(TEXT("HUD.Kill"));
			}
			else if (Clock - LastHitCue > 0.05)
			{
				// Fast fire and piercing hits: one tick per 50 ms at most.
				LastHitCue = Clock;
				PlayCue(TEXT("HUD.Hit"));
			}
		}
	}
	else if (Event == CoreKitBasicUIEvent::DamageTaken)
	{
		ScreenFx->PlayDamageFlash();
		Health->OnDamageTaken();
		// Shake and glitch: UCoreKitBasicUIScreenEffectsComponent reacts to the same event itself.
		PlayCue(TEXT("HUD.DamageTaken"));
		if (const FCoreKitBasicUIDamageTakenEvent* Damage = Payload.GetPtr<FCoreKitBasicUIDamageTakenEvent>())
		{
			if (Damage->bHasSource)
			{
				DamageArcs->AddHit(Damage->SourceLocation);
			}
		}
	}
	else if (Event == CoreKitBasicUIEvent::Pickup || Event == CoreKitBasicUIEvent::Healed)
	{
		if (const FCoreKitBasicUIPickupEvent* Pickup = Payload.GetPtr<FCoreKitBasicUIPickupEvent>())
		{
			const bool bHeal = Event == CoreKitBasicUIEvent::Healed || Pickup->Kind == ECoreKitBasicUIPickupKind::Heal;
			if (bHeal)
			{
				// Healed and Pickup(Heal) may both report the same repair kit: keep one line.
				if (Clock - LastHealFeed < 0.05 && FMath::IsNearlyEqual(LastHealAmount, Pickup->Amount))
				{
					return;
				}
				LastHealFeed = Clock;
				LastHealAmount = Pickup->Amount;
				PickupFeed->AddPickup(ECoreKitBasicUIPickupKind::Heal, Pickup->Amount);
				PlayCue(TEXT("HUD.PickupHeal"));
			}
			else
			{
				// The XP chime is played by gameplay (SC_Pickups_XP).
				PickupFeed->AddPickup(ECoreKitBasicUIPickupKind::XP, Pickup->Amount);
			}
		}
	}
	else if (Event == CoreKitBasicUIEvent::Dashed)
	{
		ScreenFx->PlaySpeedLines();
		// FOV kick: UCoreKitBasicUIScreenEffectsComponent reacts to Dashed itself.
		Dash->OnDashed(HasStaticDash());
	}
	else if (Event == CoreKitBasicUIEvent::DashReady)
	{
		Dash->PlayReady();
	}
	else if (Event == CoreKitBasicUIEvent::WeaponSwapped)
	{
		if (const FCoreKitBasicUIWeaponEvent* Swap = Payload.GetPtr<FCoreKitBasicUIWeaponEvent>())
		{
			WeaponSwap->Play(Swap->SlotNumber, Swap->LongName, Swap->bMelee);
		}
	}
	else if (Event == CoreKitBasicUIEvent::LevelUp)
	{
		LastLevelUp = Clock;
		Health->PlayLevelUp();
		Banner->NotifyLevelUp();
	}
	else if (Event == CoreKitBasicUIEvent::CardChosen)
	{
		if (const FCoreKitBasicUICardEvent* Card = Payload.GetPtr<FCoreKitBasicUICardEvent>())
		{
			Hand->OnCardChosen(Card->Card, Card->bStacked);
		}
	}
	else if (Event == CoreKitBasicUIEvent::StageChanged)
	{
		if (const FCoreKitBasicUIStageEvent* Stage = Payload.GetPtr<FCoreKitBasicUIStageEvent>())
		{
			Banner->QueueStage(Stage->StageIndex, Stage->Name, Stage->Subtitle);
		}
	}
	else if (Event == CoreKitBasicUIEvent::BossSpawned)
	{
		// Boss entry is one of the three moments allowed a glitch (07-Motion rule 5).
		if (Effects)
		{
			Effects->PlayGlitch(0.15f);
		}
	}
	else if (Event == CoreKitBasicUIEvent::BossTelegraph)
	{
		if (const FCoreKitBasicUIBossAttackEvent* Attack = Payload.GetPtr<FCoreKitBasicUIBossAttackEvent>())
		{
			BossWarning->OnTelegraph(Attack->AttackName, Attack->Hint, Attack->Duration);
		}
	}
	else if (Event == CoreKitBasicUIEvent::ThreatLock)
	{
		if (const FCoreKitBasicUIThreatEvent* Threat = Payload.GetPtr<FCoreKitBasicUIThreatEvent>())
		{
			ThreatLeft->SetThreat(Threat->Source.Get(), Threat->bLocking);
			ThreatRight->SetThreat(Threat->Source.Get(), Threat->bLocking);
		}
	}
}

void UCoreKitBasicUIHudRoot::ApplyHudScale(float Scale)
{
	AppliedScale = Scale;
	for (const FScaledBlock& Entry : ScaledBlocks)
	{
		if (UWidget* Widget = Entry.Widget.Get())
		{
			// Scale around the block's anchor on the safe area (offsets scale too).
			FWidgetTransform Transform;
			Transform.Scale = FVector2D(Scale, Scale);
			Transform.Translation = Entry.Position * (Scale - 1.f);
			Widget->SetRenderTransform(Transform);
		}
	}
}

void UCoreKitBasicUIHudRoot::SetLoop(TObjectPtr<UAudioComponent>& Component, FName Cue, bool bPlay, float VolumeMultiplier)
{
	if (!bPlay)
	{
		if (Component)
		{
			Component->Stop();
		}
		return;
	}
	if (!Component)
	{
		USoundBase* Sound = CoreKitBasicUISound::Find(Cue);
		if (!Sound)
		{
			return;
		}
		// Persistent 2D UI voice (keeps playing while the world is paused); the wave is imported looping.
		Component = UGameplayStatics::CreateSound2D(this, Sound, VolumeMultiplier, 1.f, 0.f, nullptr, false, false);
	}
	if (Component && !Component->IsPlaying())
	{
		Component->Play();
	}
}

void UCoreKitBasicUIHudRoot::UpdateLoops(bool bLowHealth, bool bAlarm)
{
	SetLoop(HeartbeatLoop, TEXT("HUD.LowHealthHeartbeat"), bLowHealth);
	SetLoop(AlarmLoop, TEXT("Boss.InboundAlarm"), bAlarm, 0.5f);
}

void UCoreKitBasicUIHudRoot::TickMotion(float Dt)
{
	Clock += Dt;
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	const UCoreKitBasicUIRunViewModel* Run = Context ? Context->GetRun() : nullptr;
	const UCoreKitBasicUIProgressionViewModel* Progression = Context ? Context->GetProgression() : nullptr;
	const UCoreKitBasicUIPreferencesViewModel* Prefs = Context ? Context->GetPreferences() : nullptr;
	const UCoreKitBasicUIVitalsViewModel* Vitals = Context ? Context->GetVitals() : nullptr;
	const UCoreKitBasicUIBossViewModel* Boss = Context ? Context->GetBoss() : nullptr;

	// HUD scale (Preferences, 0.75..1.25).
	const float Scale = Prefs ? FMath::Clamp(Prefs->HudScale, 0.5f, 1.5f) : 1.f;
	if (!FMath::IsNearlyEqual(Scale, AppliedScale))
	{
		ApplyHudScale(Scale);
	}

	// Card Draw: HUD at 0.25 and hand at 0.35 in 250 ms; back in 250 ms.
	const bool bOffer = Progression && Progression->bOfferOpen;
	if (bOffer != bDimmed)
	{
		bDimmed = bOffer;
		MainDim.Start(MainDim.Value, bOffer ? CoreKitBasicUIHudTiming::HudDimOpacity : 1.f, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::HudDim);
		HandDim.Start(HandDim.Value, bOffer ? CoreKitBasicUIHudTiming::HandDimOpacity : 1.f, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::HudDim);
	}
	if (bOffer && !bLastOffer && Clock - LastLevelUp > 2.0)
	{
		// No LevelUp event before this offer: run the XP flash and hold banners anyway.
		Health->PlayLevelUp();
		Banner->NotifyLevelUp();
	}
	bLastOffer = bOffer;
	MainDim.Tick(Dt);
	HandDim.Tick(Dt);
	MainLayer->SetRenderOpacity(MainDim.Value);
	ScreenFx->SetRenderOpacity(MainDim.Value);
	HandLayer->SetRenderOpacity(HandDim.Value);

	// End screens own the screen: the HUD leaves (EXIT) on Victory / Defeat and comes back on a new run.
	const bool bEnded = Run && (Run->Phase == ECoreKitBasicUIRunPhase::Victory || Run->Phase == ECoreKitBasicUIRunPhase::Defeat);
	if (bEnded != bHidden)
	{
		bHidden = bEnded;
		Visible.Start(Visible.Value, bEnded ? 0.f : 1.f, bEnded ? ECoreKitBasicUIMotion::Exit : ECoreKitBasicUIMotion::Settle);
	}
	Visible.Tick(Dt);
	RootCanvas->SetRenderOpacity(Visible.Value);

	// Boss phase: the top-centre vignette widens for the 720 px bar (800 x 180 at .55).
	const bool bBoss = (Run && Run->Phase == ECoreKitBasicUIRunPhase::Boss) || (Boss && Boss->bVisible);
	if (bBoss != bLastBoss && TopCenterVignette)
	{
		bLastBoss = bBoss;
		if (UCanvasPanelSlot* VignetteSlot = Cast<UCanvasPanelSlot>(TopCenterVignette->Slot))
		{
			VignetteSlot->SetSize((bBoss ? FVector2D(400.f, 180.f) : FVector2D(300.f, 170.f)) * 2.83f);
		}
		TopCenterVignette->SetColorAndOpacity(CoreKitBasicUIColor::SootA(bBoss ? 0.55f : 0.5f));
	}

	for (UCoreKitBasicUIHudBlock* Block : Blocks)
	{
		if (Block)
		{
			Block->TickHud(Dt);
		}
	}

	// Loops: heartbeat only while low health and the setting is on; alarm during the 09:50 countdown.
	const bool bPaused = Run && Run->bPaused;
	const bool bActive = !bHidden && !bPaused && !bOffer;
	const float MaxHealth = Vitals ? FMath::Max(Vitals->MaxHealth, 1.f) : 1.f;
	const bool bLow = Vitals && Vitals->Health > 0.f && (Vitals->bLowHealth || Vitals->Health / MaxHealth < CoreKitBasicUIHudTiming::LowHealthFraction);
	const bool bHeartbeat = bActive && bLow && (!Prefs || Prefs->bLowHealthHeartbeat);
	const bool bAlarm = bActive && Banner && Banner->IsBossCountdownActive();
	UpdateLoops(bHeartbeat, bAlarm);
}
