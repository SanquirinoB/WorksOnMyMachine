#include "Modules/ModuleManager.h"
#include "Brushes/SlateImageBrush.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

class FDelerictModule final : public FDefaultGameModuleImpl
{
public:
    void StartupModule() override
    {
        LoadingStyle = MakeUnique<FSlateStyleSet>(TEXT("Delerict.Loading"));
        LoadingStyle->SetContentRoot(FPaths::ProjectContentDir() / TEXT("Slate/Loading"));
        LoadingStyle->Set("Team", new FSlateImageBrush(LoadingStyle->RootToContentDir(TEXT("WorkOnMyMachine.png")), FVector2D(378, 378)));
        LoadingStyle->Set("Codex", new FSlateImageBrush(LoadingStyle->RootToContentDir(TEXT("Codex.png")), FVector2D(58, 58)));
        FSlateStyleRegistry::RegisterSlateStyle(*LoadingStyle);
    }

    void ShutdownModule() override
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*LoadingStyle);
        LoadingStyle.Reset();
    }

private:
    TUniquePtr<FSlateStyleSet> LoadingStyle;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FDelerictModule, Delerict, "Delerict");
