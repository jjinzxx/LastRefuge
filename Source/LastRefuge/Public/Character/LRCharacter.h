#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "LRCharacter.generated.h"

class ULRStatusComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UENUM(BlueprintType)
enum class ELRMovementState : uint8
{
    Crouching,
    Walking,
    Running
};

UCLASS()
class LASTREFUGE_API ALRCharacter : public ACharacter
{
    GENERATED_BODY()
private:
    float NoiseMakeTimer = 0.f;

public:
    ALRCharacter();

    UFUNCTION(BlueprintPure, Category = "Movement")
    ELRMovementState GetMovementState() const { return MovementState; }

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    
    /** AIPerception이 이 캐릭터를 시각/청각 자극원으로 인식하기 위한 컴포넌트 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    TObjectPtr<class UAIPerceptionStimuliSourceComponent> StimuliSource;

    // === Components ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FirstPersonCamera;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
    TObjectPtr<ULRStatusComponent> StatusComponent;

    // === Input ===
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> CrouchAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;
    
    // === Crouch ===
    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float CrouchCameraHeight = 30.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float StandCameraHeight = 60.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float CrouchCapsuleHalfHeight = 55.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float StandCapsuleHalfHeight = 96.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float CrouchInterpSpeed = 10.f;

    float TargetCameraHeight = 60.f;
    float TargetCapsuleHalfHeight = 96.f;

    virtual void Tick(float DeltaTime) override;

    // === Movement ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    ELRMovementState MovementState = ELRMovementState::Walking;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float CrouchSpeed = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float WalkSpeed = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float RunSpeed = 400.f;

    void SetMovementState(ELRMovementState NewState);

    // === Input Handlers ===
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartJump(const FInputActionValue& Value);
    void StopJump(const FInputActionValue& Value);
    void ToggleCrouch(const FInputActionValue& Value);
    void StartSprint(const FInputActionValue& Value);
    void StopSprint(const FInputActionValue& Value);
    
    // === 현재 이동 상태에 따른 청각 자극을 AIPerception 시스템에 보고 ===
    void ReportMovementNoise();
};