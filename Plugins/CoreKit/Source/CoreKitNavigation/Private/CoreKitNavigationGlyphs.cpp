#include "CoreKitNavigation/CoreKitNavigationGlyphs.h"

#include "Engine/Texture2D.h"

UTexture2D* UCoreKitNavigationGlyphs::FindGlyph(FKey Key) const
{
	const TObjectPtr<UTexture2D>* Glyph = Glyphs.Find(Key);
	return Glyph ? Glyph->Get() : nullptr;
}
