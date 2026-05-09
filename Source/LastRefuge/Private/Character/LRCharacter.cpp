#include "Character/LRCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Components/LRStatusComponent.h"

ALRCharacter::ALRCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    StatusComponent = CreateDefaultSubobject<ULRStatusComponent>(TEXT("StatusComponent"));

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    // 1인칭 카메라
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    GetMesh()->SetOwnerNoSee(true);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->NavAgentProps.bCanCrouch = false;
    GetCharacterMovement()->JumpZVelocity = 350.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ALRCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
}

void ALRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
            EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALRCharacter::Move);

        if (LookAction)
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALRCharacter::Look);

        if (JumpAction)
        {
            EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ALRCharacter::StartJump);
            EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ALRCharacter::StopJump);
        }
        if (CrouchAction)
            EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ALRCharacter::ToggleCrouch);

        if (SprintAction)
        {
            EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ALRCharacter::StartSprint);
            EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ALRCharacter::StopSprint);
        }
    }
}

void ALRCharacter::SetMovementState(ELRMovementState NewState)
{
    if (MovementState == NewState) return;

    MovementState = NewState;

    switch (MovementState)
    {
    case ELRMovementState::Crouching:
        GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;
        TargetCameraHeight = CrouchCameraHeight;
        TargetCapsuleHalfHeight = CrouchCapsuleHalfHeight;
        StatusComponent->UpdateNoiseRadius(StatusComponent->CrouchNoiseRadius);
        break;

    case ELRMovementState::Walking:
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
        TargetCameraHeight = StandCameraHeight;
        TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
        StatusComponent->UpdateNoiseRadius(StatusComponent->WalkNoiseRadius);
        break;

    case ELRMovementState::Running:
        GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
        TargetCameraHeight = StandCameraHeight;
        TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
        StatusComponent->UpdateNoiseRadius(StatusComponent->RunNoiseRadius);
        break;
    }
}

void ALRCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    if (Controller && !MovementVector.IsZero())
    {
        const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDir, MovementVector.Y);
        AddMovementInput(RightDir, MovementVector.X);
    }
}

void ALRCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();
    if (Controller)
    {
        AddControllerYawInput(LookVector.X);
        AddControllerPitchInput(LookVector.Y);
    }
}

void ALRCharacter::StartJump(const FInputActionValue& Value)
{
    Jump();
}

void ALRCharacter::StopJump(const FInputActionValue& Value)
{
    StopJumping();
}

void ALRCharacter::ToggleCrouch(const FInputActionValue& Value)
{
    if (MovementState == ELRMovementState::Crouching)
        SetMovementState(ELRMovementState::Walking);
    else
        SetMovementState(ELRMovementState::Crouching);
}

void ALRCharacter::StartSprint(const FInputActionValue& Value)
{
    // 앉은 상태에서는 달리기 불가
    if (MovementState != ELRMovementState::Crouching)
        SetMovementState(ELRMovementState::Running);
}

void ALRCharacter::StopSprint(const FInputActionValue& Value)
{
    if (MovementState == ELRMovementState::Running)
        SetMovementState(ELRMovementState::Walking);
}

void ALRCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 카메라 높이 보간
    FVector CameraLocation = FirstPersonCamera->GetRelativeLocation();
    CameraLocation.Z = FMath::FInterpTo(CameraLocation.Z, TargetCameraHeight, DeltaTime, CrouchInterpSpeed);
    FirstPersonCamera->SetRelativeLocation(CameraLocation);

    // 캡슐 높이 보간
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    float CurrentHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    float NewHalfHeight = FMath::FInterpTo(CurrentHalfHeight, TargetCapsuleHalfHeight, DeltaTime, CrouchInterpSpeed);
    Capsule->SetCapsuleHalfHeight(NewHalfHeight);

    // 캡슐이 줄어들 때 캐릭터가 바닥에 붙어있도록 메쉬 오프셋 조정
    FVector MeshLocation = GetMesh()->GetRelativeLocation();
    MeshLocation.Z = -NewHalfHeight;
    GetMesh()->SetRelativeLocation(MeshLocation);
    
    // 달리는 중이면 스테미나 소모
    if (MovementState == ELRMovementState::Running)
    {
        StatusComponent->ConsumeStamina(StatusComponent->GetStaminaDrainRate() * DeltaTime);

        // 스테미나 바닥나면 강제로 걷기 전환
        if (StatusComponent->IsStaminaEmpty())
        {
            SetMovementState(ELRMovementState::Walking);
        }
    }
    // MakeNoise (AI 청각 감지용)
    NoiseMakeTimer += DeltaTime;
    if (!GetVelocity().IsZero() && NoiseMakeTimer >= 0.2f)
    {
        NoiseMakeTimer = 0.f;
        MakeNoise(
            StatusComponent->GetNoiseRadius() / StatusComponent->RunNoiseRadius,
            this,
            GetActorLocation()
        );
    }
    // 소음 반경 시각화 - 나중에 비활성화
    if (StatusComponent->GetNoiseRadius() > 0.f)
    {
        FColor SphereColor;
        switch (MovementState)
        {
        case ELRMovementState::Crouching:
            SphereColor = FColor::Green;
            break;
        case ELRMovementState::Walking:
            SphereColor = FColor::Yellow;
            break;
        case ELRMovementState::Running:
            SphereColor = FColor::Red;
            break;
        default:
            SphereColor = FColor::White;
        }

        DrawDebugSphere(
            GetWorld(),
            GetActorLocation(),
            StatusComponent->GetNoiseRadius(),
            16,
            SphereColor,
            false,
            -1.f
        );
    }

    // 소음 반경 디버그 수치 출력
    GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan,
        FString::Printf(TEXT("Noise Radius: %.0f"), StatusComponent->GetNoiseRadius()));
    GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Yellow,
        FString::Printf(TEXT("Stamina: %.1f"), StatusComponent->GetStamina()));
    GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Red,
        FString::Printf(TEXT("Health: %.1f"), StatusComponent->GetHealth()));
}