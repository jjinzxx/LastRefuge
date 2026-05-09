#include "AI/LRBotAIController.h"

#include "AI/LRBot.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

ALRBotAIController::ALRBotAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALRBotAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 빙의한 Pawn이 ALRBot인지 확인
	ALRBot* Bot = Cast<ALRBot>(InPawn);
	if (!Bot) return;

	// Bot의 PerceptionComponent 캐싱 후 이벤트 바인딩
	BotPerceptionComponent = Bot->GetPerceptionComponent();
	if (BotPerceptionComponent)
	{
		BotPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
			this, &ALRBotAIController::OnPerceptionUpdated
		);
	}
}

void ALRBotAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	const bool bSensedBySight   = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
	const bool bSensedByHearing = Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>();
	const bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();

	if (bSensedBySight)
	{
		if (bSuccessfullySensed)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LRBot] Player Sighted: %s"), *Actor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[LRBot] Player Lost (Sight): %s"), *Actor->GetName());
		}
	}
	else if (bSensedByHearing)
	{
		if (bSuccessfullySensed)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LRBot] Player Heard: %s"), *Actor->GetName());
		}
	}
}