#include "AI/LRBotAIController.h"
#include "AI/LRBot.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"

const FName ALRBotAIController::BB_PatrolTarget(TEXT("PatrolTarget"));
const FName ALRBotAIController::BB_PatrolIndex(TEXT("PatrolIndex"));
const FName ALRBotAIController::BB_PlayerActor(TEXT("PlayerActor"));
const FName ALRBotAIController::BB_BotState(TEXT("BotState"));
const FName ALRBotAIController::BB_LKL(TEXT("LKL"));

void ALRBotAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledBot = Cast<ALRBot>(InPawn);
    if (!ControlledBot) return;

    if (UBehaviorTree* BT = ControlledBot->BehaviorTreeAsset)
        RunBehaviorTree(BT);

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsInt(BB_PatrolIndex, 0);
        bool bValid;
        FVector FirstPoint = ControlledBot->GetPatrolPointLocation(0, bValid);
        if (bValid) BB->SetValueAsVector(BB_PatrolTarget, FirstPoint);
        BB->SetValueAsInt(BB_BotState, 0);
    }

    if (UAIPerceptionComponent* PC = ControlledBot->GetPerceptionComponent())
    {
        PC->OnTargetPerceptionUpdated.AddDynamic(
            this, &ALRBotAIController::OnPerceptionUpdated);
    }
}

void ALRBotAIController::OnUnPossess()
{
    StopFiring();

    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(CombatTrackingHandle);

    if (ControlledBot)
    {
        if (UAIPerceptionComponent* PC = ControlledBot->GetPerceptionComponent())
        {
            PC->OnTargetPerceptionUpdated.RemoveDynamic(
                this, &ALRBotAIController::OnPerceptionUpdated);
        }
    }
    ControlledBot = nullptr;
    TrackedPlayer = nullptr;

    Super::OnUnPossess();
}

// ── Perception ──────────────────────────────────────────
void ALRBotAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor || !ControlledBot) return;

    const bool bSight   = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
    const bool bHearing = Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>();

    if (bSight)
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            TrackedPlayer = Actor;
            ControlledBot->LastKnownLocation = Actor->GetActorLocation();
            EnterCombat(Actor);
        }
        else
        {
            EnterSuspicious(ControlledBot->LastKnownLocation);
        }
    }
    else if (bHearing)
    {
        if (Stimulus.WasSuccessfullySensed() &&
            ControlledBot->GetBotState() == ELRBotState::Patrol)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LRBot] Heard — going Suspicious"));
            EnterSuspicious(Stimulus.StimulusLocation);
        }
    }
}

// ── 상태 전환 ────────────────────────────────────────────
void ALRBotAIController::EnterCombat(AActor* PlayerActor)
{
    UE_LOG(LogTemp, Warning, TEXT("[LRBot] → COMBAT"));
    ControlledBot->SetBotState(ELRBotState::Combat);

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsObject(BB_PlayerActor, PlayerActor);
        BB->SetValueAsInt(BB_BotState, 2);
    }

    StartFiring();

    // 0.1초마다 플레이어 위치를 BB PatrolTarget에 갱신
    GetWorld()->GetTimerManager().ClearTimer(CombatTrackingHandle);
    GetWorld()->GetTimerManager().SetTimer(
        CombatTrackingHandle,
        this,
        &ALRBotAIController::UpdateCombatTarget,
        0.1f, true, 0.f);
}

void ALRBotAIController::EnterSuspicious(const FVector& InLKL)
{
    UE_LOG(LogTemp, Warning, TEXT("[LRBot] → SUSPICIOUS"));
    StopFiring();

    GetWorld()->GetTimerManager().ClearTimer(CombatTrackingHandle);

    TrackedPlayer = nullptr;
    ControlledBot->SetBotState(ELRBotState::Suspicious);

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsObject(BB_PlayerActor, nullptr);
        BB->SetValueAsVector(BB_LKL, InLKL);
        BB->SetValueAsVector(BB_PatrolTarget, InLKL);
        BB->SetValueAsInt(BB_BotState, 1);
    }
}

void ALRBotAIController::EnterPatrol()
{
    UE_LOG(LogTemp, Log, TEXT("[LRBot] → PATROL"));
    ControlledBot->SetBotState(ELRBotState::Patrol);

    GetWorld()->GetTimerManager().ClearTimer(CombatTrackingHandle);

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsObject(BB_PlayerActor, nullptr);
        BB->SetValueAsInt(BB_BotState, 0);

        // Suspicious에서 LKL로 덮어쓴 PatrolTarget을 현재 PatrolIndex 위치로 복원
        if (ControlledBot && ControlledBot->GetPatrolPointCount() > 0)
        {
            const int32 Idx = BB->GetValueAsInt(BB_PatrolIndex);
            bool bValid = false;
            const FVector Loc = ControlledBot->GetPatrolPointLocation(Idx, bValid);
            if (bValid)
            {
                BB->SetValueAsVector(BB_PatrolTarget, Loc);
            }
        }
    }
}

void ALRBotAIController::AbortToPatrol()
{
    UE_LOG(LogTemp, Warning, TEXT("[LRBot] AbortToPatrol (player died)"));

    StopFiring();

    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(CombatTrackingHandle);

    TrackedPlayer = nullptr;
    if (ControlledBot)
        ControlledBot->LastKnownLocation = FVector::ZeroVector;

    EnterPatrol();
}

// ── Combat 실시간 위치 추적 ──────────────────────────────
void ALRBotAIController::UpdateCombatTarget()
{
    if (!TrackedPlayer || !ControlledBot) return;

    FVector PlayerLocation = TrackedPlayer->GetActorLocation();
    ControlledBot->LastKnownLocation = PlayerLocation;

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        // PatrolTarget을 0.1초마다 갱신 → Move To가 계속 재이동
        BB->SetValueAsVector(BB_PatrolTarget, PlayerLocation);
    }
}

// ── 사격 ────────────────────────────────────────────────
void ALRBotAIController::StartFiring()
{
    if (GetWorld()->GetTimerManager().IsTimerActive(FireTimerHandle)) return;
    GetWorld()->GetTimerManager().SetTimer(
        FireTimerHandle, this, &ALRBotAIController::FireAtPlayer,
        ControlledBot->FireInterval, true, 0.5f);
}

void ALRBotAIController::StopFiring()
{
    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
}

void ALRBotAIController::FireAtPlayer()
{
    if (!ControlledBot || !TrackedPlayer) { StopFiring(); return; }

    FVector Start = ControlledBot->GetActorLocation() + FVector(0, 0, 60.f);
    FVector End   = TrackedPlayer->GetActorLocation();

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(ControlledBot);

    // TODO: 근접 전투 시스템 도입 시 봇 위협 수단 재설계
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, Start, End, ECC_Pawn, Params);
    
#if WITH_EDITOR
    DrawDebugLine(GetWorld(), Start, bHit ? Hit.Location : End,
        bHit ? FColor::Red : FColor::Yellow, false, 0.8f, 0, 1.5f);
#endif

    if (bHit && Hit.GetActor() == TrackedPlayer)
    {
        UGameplayStatics::ApplyDamage(
            TrackedPlayer, ControlledBot->FireDamage,
            this, ControlledBot, nullptr);

        UE_LOG(LogTemp, Warning, TEXT("[LRBot] Hit! %.0f dmg"), ControlledBot->FireDamage);
    }
}