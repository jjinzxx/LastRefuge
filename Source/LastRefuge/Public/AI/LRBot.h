#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LRBot.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

UCLASS()
class LASTREFUGE_API ALRBot : public ACharacter
{
	GENERATED_BODY()

public:
	ALRBot();
	
	UAIPerceptionComponent* GetPerceptionComponent() const
	{
		return PerceptionComponent;
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> PerceptionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception|Sight")
	float SightRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception|Sight")
	float LoseSightRadius = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception|Sight")
	float PeripheralVisionHalfAngleDegrees = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception|Hearing")
	float HearingRange = 2000.f;
};