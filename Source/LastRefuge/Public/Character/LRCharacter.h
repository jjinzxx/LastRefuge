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
class ULRInventoryComponent;
class ULRHudWidget;

UENUM(BlueprintType)
enum class ELRMovementState : uint8
{
    Crouching,
    Walking,
    Running
};

// 수색/이동 게이지 진행률 (0.0 ~ 1.0)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSearchProgressChanged, float, Progress);
// 수색/이동 시작 — 베이스 텍스트 전달 ("수색중", "이동중" 등)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSearchStarted, FText, ProgressBaseText);
// 수색/이동 종료 — bCompleted: true=완료, false=취소 / StatusText: 취소 시 표시할 텍스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSearchEnded, bool, bCompleted, FText, StatusText);
// 조준 중인 오브젝트의 프롬프트 텍스트 변경 (비어있으면 프롬프트 숨김)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionPromptChanged, FText, Prompt);

UCLASS()
class LASTREFUGE_API ALRCharacter : public ACharacter
{
    GENERATED_BODY()
    
private:
    float NoiseMakeTimer = 0.f;
    
    UFUNCTION()
    void OnHealthChanged(float NewHealth, float MaxHealth);
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<ULRInventoryComponent> InventoryComponent;

public:
    ALRCharacter();

    // === HUD ===
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULRHudWidget> HudWidgetClass;

    // === UI 델리게이트 ===
    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnSearchProgressChanged OnSearchProgressChanged;

    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnSearchStarted OnSearchStarted;

    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnSearchEnded OnSearchEnded;

    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnInteractionPromptChanged OnInteractionPromptChanged;

    UFUNCTION(BlueprintPure, Category = "Movement")
    ELRMovementState GetMovementState() const { return MovementState; }

    /** 엔진 데미지 파이프라인을 ULRStatusComponent로 라우팅 */
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

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
    
    /** 상호작용(E키) 입력 액션 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* IA_Interact;

    /** 상호작용 시도 (LineTrace) */
    void TryInteract();

    /** 수색 중단 (인터럽트) */
    void CancelSearch();

    // --- 수색 게이지 관련 상태 변수 ---
    bool bIsSearching = false;
    float CurrentSearchTime = 0.f;
    float SearchDuration = 0.f;

    // --- 소음 리플 링 (3D 월드, 바닥에 DrawDebugCircle) ---
    float NoiseRingPhase[3] = { 0.f, 0.333f, 0.667f };
    static constexpr float NoiseRingCycleSpeed = 1.0f;

    // 현재 상호작용 중인 대상 기억
    UPROPERTY()
    AActor* CurrentInteractable;

    // 현재 표시 중인 프롬프트 (변경 감지용)
    FText CurrentPromptText;
};