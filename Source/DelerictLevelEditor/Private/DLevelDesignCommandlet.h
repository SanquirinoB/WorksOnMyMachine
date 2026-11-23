#pragma once

#include "Commandlets/Commandlet.h"
#include "DLevelDesignCommandlet.generated.h"

/** Creates the initial kit and lab. Existing assets and maps are preserved. */
UCLASS()
class UDLevelDesignCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UDLevelDesignCommandlet();
    int32 Main(const FString& Params) override;
};
