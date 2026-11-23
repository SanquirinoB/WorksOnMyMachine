#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "DataDriven/DataContainer.h"
#include "DataDriven/DataDrivenAssetLibrary.h"
#include "DataDriven/DataDrivenBlueprintLibrary.h"
#include "DataDriven/DataDrivenComponent.h"
#include "DataDriven/DataDrivenTypes.h"
#include "DataDriven/DataFragmentBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataDrivenLookupContractTest,
	"DataDriven.Container.LookupContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDataDrivenLookupContractTest::RunTest(const FString& Parameters)
{
	UDataContainer* Container = NewObject<UDataContainer>();
	EDataDrivenLookupResult Result = EDataDrivenLookupResult::Success;

	TestNotNull(TEXT("A generic data container can be instantiated"), Container);
	#if WITH_EDITOR
	FDataValidationContext ValidationContext;
	TestEqual(TEXT("Empty containers validate successfully"), Container->IsDataValid(ValidationContext), EDataValidationResult::Valid);
	#endif
	TestNull(TEXT("Missing fragment class returns no fragment"), UDataDrivenBlueprintLibrary::GetDataFragmentByClass(Container, nullptr, Result));
	TestEqual(TEXT("Missing fragment class is reported"), Result, EDataDrivenLookupResult::InvalidFragmentClass);
	TestFalse(TEXT("Missing fragment class is not reported as present"), UDataDrivenBlueprintLibrary::HasDataFragmentOfClass(Container, nullptr));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataDrivenComponentLookupTest,
	"DataDriven.Component.TagLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDataDrivenComponentLookupTest::RunTest(const FString& Parameters)
{
	UDataDrivenComponent* Component = NewObject<UDataDrivenComponent>();
	EDataDrivenLookupResult Result = EDataDrivenLookupResult::Success;

	TestNull(TEXT("Invalid tags do not resolve containers"), UDataDrivenBlueprintLibrary::GetDataContainerByTag(Component, FGameplayTag(), Result));
	TestEqual(TEXT("Invalid tags report a dedicated result"), Result, EDataDrivenLookupResult::InvalidTag);
	TestFalse(TEXT("Invalid tags are not reported as present"), Component->HasDataContainer(FGameplayTag()));
	TestFalse(TEXT("Invalid entries are rejected"), Component->SetDataContainer(FGameplayTag(), NewObject<UDataContainer>()));
	TestTrue(TEXT("A new component starts with an empty map"), Component->GetDataContainerMap().IsEmpty());
	Component->ClearDataContainerMap();
	TestTrue(TEXT("Clearing an empty map is safe"), Component->GetDataContainerMap().IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataDrivenAssetLibraryValidationTest,
	"DataDriven.Assets.InvalidRequests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FDataDrivenAssetLibraryValidationTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Invalid class requests return an empty result"), UDataDrivenAssetLibrary::LoadAssetsByClass(nullptr).IsEmpty());
	TestTrue(TEXT("Invalid primary asset types return an empty result"), UDataDrivenAssetLibrary::LoadPrimaryAssetsFromType(FPrimaryAssetType()).IsEmpty());
	return true;
}

#endif
