#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LRStatusComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, NewStamina, float, MaxStamina);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTREFUGE_API ULRStatusComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULRStatusComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // === 체력 ===
    UFUNCTION(BlueprintCallable, Category = "Status")
    void ApplyDamage(float Amount);

    UFUNCTION(BlueprintPure, Category = "Status")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category = "Status")
    float GetHealthPercent() const { return Health / MaxHealth; }

    // === 스테미나 ===
    UFUNCTION(BlueprintCallable, Category = "Status")
    void ConsumeStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Status")
    void RestoreStamina(float Amount);

    UFUNCTION(BlueprintPure, Category = "Status")
    float GetStamina() const { return Stamina; }

    UFUNCTION(BlueprintPure, Category = "Status")
    float GetStaminaPercent() const { return Stamina / MaxStamina; }
    
    UFUNCTION(BlueprintPure, Category = "Status")
    float GetStaminaDrainRate() const { return StaminaDrainRate; }

    UFUNCTION(BlueprintPure, Category = "Status")
    bool IsStaminaEmpty() const { return Stamina <= 0.f; }

    // === 델리게이트 ===
    UPROPERTY(BlueprintAssignable, Category = "Status")
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Status")
    FOnStaminaChanged OnStaminaChanged;
    
    // === 소음 ===
    UFUNCTION(BlueprintPure, Category = "Status")
    float GetNoiseRadius() const { return CurrentNoiseRadius; }

    UFUNCTION(BlueprintCallable, Category = "Status")
    void UpdateNoiseRadius(float NewRadius);
    
    UPROPERTY(EditDefaultsOnly, Category = "Status")
    float CrouchNoiseRadius = 200.f;

    UPROPERTY(EditDefaultsOnly, Category = "Status")
    float WalkNoiseRadius = 600.f;

    UPROPERTY(EditDefaultsOnly, Category = "Status")
    float RunNoiseRadius = 1500.f;


protected:
    virtual void BeginPlay() override;

    // === 체력 ===
    UPROPERTY(EditDefaultsOnly, Category = "Status")
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, Category = "Status")
    float Health = 100.f;

    // === 스테미나 ===
    UPROPERTY(EditDefaultsOnly, Category = "Status")
    float MaxStamina = 100.f;

    UPROPERTY(VisibleAnywhere, Category = "Status")
    float Stamina = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Status")
    float StaminaDrainRate = 20.f;   // 뛸 때 초당 소모량

    UPROPERTY(EditDefaultsOnly, Category = "Status")
    float StaminaRegenRate = 15.f;   // 정지 시 초당 회복량

    UPROPERTY(EditDefaultsOnly, Category = "Status")
    float StaminaRegenDelay = 1.5f;  // 스테미나 소모 후 회복 시작까지 대기 시간
    


private:
    float StaminaRegenTimer = 0.f;
    bool bIsDraining = false;
    float CurrentNoiseRadius = 0.f;
};