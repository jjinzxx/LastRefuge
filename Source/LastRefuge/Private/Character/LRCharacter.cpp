#include "Character/LRCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

ALRCharacter::ALRCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // 캡슐 크기 (사람 크기 표준)
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    // 1인칭 카메라
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // 눈 높이
    FirstPersonCamera->bUsePawnControlRotation = true;

    // 캐릭터 메쉬는 자기 자신에게 안 보이게 (1인칭)
    GetMesh()->SetOwnerNoSee(true);

    // 컨트롤러 회전: Yaw만 따라가게 (Pitch는 카메라가 처리)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    // 이동 설정
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 300.f; // Day 2에서 자세별로 변경
}

void ALRCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Enhanced Input Mapping Context 등록
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
        {
            EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALRCharacter::Move);
        }
        if (LookAction)
        {
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALRCharacter::Look);
        }
        if (JumpAction)
        {
            EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ALRCharacter::StartJump);
            EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ALRCharacter::StopJump);
        }
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