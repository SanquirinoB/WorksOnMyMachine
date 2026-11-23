#include "CoreKitNavigation/CoreKitInputIconSet.h"

#include "Engine/Texture2D.h"

bool UCoreKitInputIconSet::FindKey(FKey Key, FCoreKitKeyPresentation& OutPresentation) const
{
	if (const FCoreKitKeyPresentation* Found = Keys.Find(Key); Found && Found->IsSet())
	{
		OutPresentation = *Found;
		return true;
	}
	OutPresentation = FCoreKitKeyPresentation();
	return false;
}

void UCoreKitInputIconSet::SetKey(FKey Key, const FCoreKitKeyPresentation& Presentation)
{
	if (!Key.IsValid())
	{
		return;
	}
	Modify();
	if (Presentation.IsSet())
	{
		Keys.Add(Key, Presentation);
	}
	else
	{
		Keys.Remove(Key);
	}
}
