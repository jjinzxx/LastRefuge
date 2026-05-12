#include "Components/LRStatusComponent.h"

ULRStatusComponent::ULRStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULRStatusComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	Stamina = MaxStamina;
}

void ULRStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 스테미나 소모 중이 아니면 회복 타이머 진행
	if (!bIsDraining)
	{
		if (Stamina < MaxStamina)
		{
			StaminaRegenTimer += DeltaTime;
			if (StaminaRegenTimer >= StaminaRegenDelay)
			{
				RestoreStamina(StaminaRegenRate * DeltaTime);
			}
		}
	}

	// 매 프레임 드레인 플래그 초기화 (ConsumeStamina가 호출되면 다시 true로)
	bIsDraining = false;
}

void ULRStatusComponent::ApplyDamage(float Amount)
{
	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.f)
	{
		// 사망 처리는 Character에서 델리게이트로 받아서 처리
		// Day 6에서 GameMode와 연결
	}
}

void ULRStatusComponent::ConsumeStamina(float Amount)
{
	bIsDraining = true;
	StaminaRegenTimer = 0.f; // 회복 타이머 리셋

	Stamina = FMath::Clamp(Stamina - Amount, 0.f, MaxStamina);
	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
}

void ULRStatusComponent::UpdateNoiseRadius(float NewRadius)
{
	CurrentNoiseRadius = NewRadius;
}

void ULRStatusComponent::RestoreHealth(float Amount)
{
	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void ULRStatusComponent::RestoreStamina(float Amount)
{
	Stamina = FMath::Clamp(Stamina + Amount, 0.f, MaxStamina);
	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
}