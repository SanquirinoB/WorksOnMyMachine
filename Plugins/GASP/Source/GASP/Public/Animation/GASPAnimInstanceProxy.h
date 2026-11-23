#pragma once

#include "Animation/AnimInstanceProxy.h"
#include "GASPAnimInstanceProxy.generated.h"

class ACharacter;
/**
 * 
 */
USTRUCT()
struct FGASPAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FGASPAnimInstanceProxy() = default;

	explicit FGASPAnimInstanceProxy(UAnimInstance* InAnimInstance);

protected:
	virtual void InitializeObjects(UAnimInstance* InAnimInstance) override;
	virtual void Update(float DeltaSeconds) override;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> CharacterOwner;
public:
	
	FORCEINLINE ACharacter* GetCharacterOwner() const { return CharacterOwner.Get(); }

};

