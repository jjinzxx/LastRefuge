#include "AI/LRBotAIController.h"
#include "AI/LRBot.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/CharacterMovementComponent.h"

// Blackboard 키 이름 — BB_LRBot의 키 이름과 정확히 일치
const FName ALRBotAIController::BB_PatrolTarget = TEXT("PatrolTarget");
const FName ALRBotAIController::BB_PatrolIndex  = TEXT("PatrolIndex");

ALRBotAIController::ALRBotAIController()
{
    // BlackboardComponent는 AAIController가 기본 보유, 별도 생성 불필요
}

void ALRBotAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] OnPossess called"));

    ControlledBot = Cast<ALRBot>(InPawn);
    if (!ControlledBot)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] Cast failed"));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] Cast success"));

    UAIPerceptionComponent* PerceptionComp = ControlledBot->GetPerceptionComponent();
    if (PerceptionComp)
    {
        PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
            this, &ALRBotAIController::OnPerceptionUpdated);
    }

    UBehaviorTree* BT = ControlledBot->GetBehaviorTreeAsset();
    if (!BT)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] BT is null"));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] BT found, running"));

    RunBehaviorTree(BT);
    UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] RunBehaviorTree called"));

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] BB found, setting values"));
        BB->SetValueAsInt(BB_PatrolIndex, 0);

        bool bValid = false;
        const FVector FirstTarget = ControlledBot->GetPatrolPointLocation(0, bValid);
        if (bValid)
        {
            BB->SetValueAsVector(BB_PatrolTarget, FirstTarget);
            UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] PatrolTarget set"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] PatrolPoint[0] invalid"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[LRBotAIController] BB is null"));
    }
}

void ALRBotAIController::OnUnPossess()
{
    // 빙의 해제 시 Perception 콜백 제거
    if (ControlledBot)
    {
        if (UAIPerceptionComponent* PerceptionComp = ControlledBot->GetPerceptionComponent())
        {
            PerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(
                this, &ALRBotAIController::OnPerceptionUpdated);
        }
    }

    ControlledBot = nullptr;
    Super::OnUnPossess();
}

void ALRBotAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            UE_LOG(LogTemp, Warning, TEXT("[LRBot] Player Sighted"));

            // 추격 속도로 전환
            if (ControlledBot)
            {
                ControlledBot->GetCharacterMovement()->MaxWalkSpeed = ControlledBot->ChaseSpeed;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[LRBot] Player Lost (Sight)"));

            // 순찰 속도로 복귀
            if (ControlledBot)
            {
                ControlledBot->GetCharacterMovement()->MaxWalkSpeed = ControlledBot->PatrolSpeed;
            }
        }
    }
    else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            UE_LOG(LogTemp, Warning, TEXT("[LRBot] Player Heard"));
        }
    }
}