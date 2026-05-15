#include "AI/LRBot.h"
#include "BehaviorTree/BehaviorTree.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Kismet/GameplayStatics.h"

ALRBot::ALRBot()
{
	PrimaryActorTick.bCanEverTick = true;
	

	// AIPerception 컴포넌트 생성
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	// Sight 설정
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngleDegrees;
	SightConfig->SetMaxAge(5.f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	// Hearing 설정
	UAISenseConfig_Hearing* HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->ConfigureSense(*HearingConfig);
	PerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
	
	// 기본 이동 속도를 순찰 속도로 설정
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
}

void ALRBot::BeginPlay()
{
	Super::BeginPlay();
}

FVector ALRBot::GetPatrolPointLocation(int32 Index, bool& bIsValid) const
{
	if (!PatrolPoints.IsValidIndex(Index))
	{
		bIsValid = false;
		return FVector::ZeroVector;
	}

	const AActor* Point = PatrolPoints[Index];
	if (!IsValid(Point))
	{
		bIsValid = false;
		return FVector::ZeroVector;
	}

	bIsValid = true;
	return Point->GetActorLocation();
}

void ALRBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_EDITOR
	// 봇 전방 방향 화살표 (파란색)
	const FVector Start = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const FVector End = Start + Forward * 150.f;

	DrawDebugDirectionalArrow(
		GetWorld(),
		Start,
		End,
		30.f,        // 화살표 크기
		FColor::Blue,
		false,       // 지속 여부
		-1.f,        // 지속 시간 (-1 = 1프레임)
		0,
		3.f          // 선 두께
	);
#endif
}

void ALRBot::SetBotState(ELRBotState NewState)
{
	if (CurrentState == NewState) return;
	CurrentState = NewState;

	float TargetSpeed = (NewState == ELRBotState::Combat) ? ChaseSpeed : PatrolSpeed;
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		MoveComp->MaxWalkSpeed = TargetSpeed;

	USoundBase* VoiceSFX = nullptr;
	switch (NewState)
	{
	case ELRBotState::Combat:     VoiceSFX = SFX_Alert;          break;
	case ELRBotState::Suspicious: VoiceSFX = SFX_Suspicious;     break;
	case ELRBotState::Patrol:     VoiceSFX = SFX_ReturnToPatrol; break;
	}
	if (VoiceSFX)
		UGameplayStatics::PlaySoundAtLocation(this, VoiceSFX, GetActorLocation());

	UE_LOG(LogTemp, Warning, TEXT("[LRBot] State → %s"),
		*UEnum::GetValueAsString(NewState));
}