#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"

namespace
{
	bool GReducedMotion = false;

	/** Cubic-bezier (0,0) (x1,y1) (x2,y2) (1,1), solved for x with Newton iterations. */
	float Bezier(float X1, float Y1, float X2, float Y2, float X)
	{
		auto Sample = [](float A1, float A2, float T)
		{
			const float U = 1.f - T;
			return 3.f * U * U * T * A1 + 3.f * U * T * T * A2 + T * T * T;
		};
		auto Slope = [](float A1, float A2, float T)
		{
			const float U = 1.f - T;
			return 3.f * U * U * A1 + 6.f * U * T * (A2 - A1) + 3.f * T * T * (1.f - A2);
		};
		float T = X;
		for (int32 Iteration = 0; Iteration < 8; ++Iteration)
		{
			const float Error = Sample(X1, X2, T) - X;
			const float D = Slope(X1, X2, T);
			if (FMath::Abs(Error) < 1e-4f || FMath::Abs(D) < 1e-6f)
			{
				break;
			}
			T = FMath::Clamp(T - Error / D, 0.f, 1.f);
		}
		return Sample(Y1, Y2, T);
	}
}

float UCoreKitBasicUIMotionSettings::Duration(ECoreKitBasicUIMotion Token) const
{
	switch (Token)
	{
	case ECoreKitBasicUIMotion::Snap: return Snap;
	case ECoreKitBasicUIMotion::Settle: return Settle;
	case ECoreKitBasicUIMotion::Slam: return Slam;
	case ECoreKitBasicUIMotion::Deal: return Deal;
	case ECoreKitBasicUIMotion::Exit: return Exit;
	case ECoreKitBasicUIMotion::Pulse: return Pulse;
	default: return Settle;
	}
}

float CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion Token, float Alpha)
{
	const float A = FMath::Clamp(Alpha, 0.f, 1.f);
	switch (Token)
	{
	case ECoreKitBasicUIMotion::Snap: return Bezier(.2f, .9f, .3f, 1.f, A);
	case ECoreKitBasicUIMotion::Settle: return Bezier(.16f, 1.f, .3f, 1.f, A);
	case ECoreKitBasicUIMotion::Slam: return Bezier(.34f, 1.56f, .64f, 1.f, A);
	case ECoreKitBasicUIMotion::Deal: return Bezier(.22f, 1.f, .36f, 1.f, A);
	case ECoreKitBasicUIMotion::Exit: return Bezier(.4f, 0.f, 1.f, 1.f, A);
	case ECoreKitBasicUIMotion::Pulse: return 0.5f - 0.5f * FMath::Cos(A * 2.f * PI);
	default: return A;
	}
}

bool CoreKitBasicUIMotion::IsReducedMotion()
{
	return GReducedMotion;
}

void CoreKitBasicUIMotion::SetReducedMotion(bool bReduced)
{
	GReducedMotion = bReduced;
}

void FCoreKitBasicUITween::Start(float InTo, ECoreKitBasicUIMotion InCurve, float InDuration, float InDelay, bool bFromCurrent)
{
	Start(bFromCurrent ? Value : From, InTo, InCurve, InDuration, InDelay);
}

void FCoreKitBasicUITween::Start(float InFrom, float InTo, ECoreKitBasicUIMotion InCurve, float InDuration, float InDelay)
{
	From = InFrom;
	To = InTo;
	Curve = InCurve;
	Duration = InDuration >= 0.f ? InDuration : GetDefault<UCoreKitBasicUIMotionSettings>()->Duration(InCurve);
	Delay = InDelay;
	Elapsed = 0.f;
	bActive = true;
	Value = From;
	if (CoreKitBasicUIMotion::IsReducedMotion())
	{
		Snap(To);
	}
}

void FCoreKitBasicUITween::Snap(float InValue)
{
	From = To = Value = InValue;
	bActive = false;
	Elapsed = Duration;
}

bool FCoreKitBasicUITween::Tick(float DeltaSeconds)
{
	if (!bActive)
	{
		return false;
	}
	Elapsed += DeltaSeconds;
	if (Elapsed < Delay)
	{
		return true;
	}
	const float A = Alpha();
	Value = FMath::Lerp(From, To, CoreKitBasicUIMotion::Ease(Curve, A));
	if (A >= 1.f && Curve != ECoreKitBasicUIMotion::Pulse)
	{
		Value = To;
		bActive = false;
	}
	else if (A >= 1.f)
	{
		// Pulse loops.
		Elapsed = Delay;
	}
	return bActive;
}
