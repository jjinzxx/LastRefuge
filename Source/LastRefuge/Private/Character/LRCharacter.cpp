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
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "LRGameMode.h"
#include "LRGameInstance.h"
#include "Components/LRInventoryComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Interfaces/LRInteractable.h"

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
    
    StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
    StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
    StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
    StimuliSource->bAutoRegister = true;
    
    InventoryComponent = CreateDefaultSubobject<ULRInventoryComponent>(TEXT("InventoryComponent"));
}

void ALRCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (InventoryComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("Inventory System Initialized."));

        // 레벨 이동 후 인벤토리 복원
        if (ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance()))
        {
            if (GI->bHasTravelData)
            {
                for (const FLRItemSlot& Slot : GI->PersistentInventory)
                {
                    if (!Slot.IsEmpty())
                        InventoryComponent->AddItem(Slot.ItemData, Slot.Quantity);
                }
            }
        }
    }

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
    
    StatusComponent->OnHealthChanged.AddDynamic(this, &ALRCharacter::OnHealthChanged);
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

        // 상호작용 (E키 바인딩) - 에디터에서 IA_Interact 할당 필요
        if (IA_Interact)
        {
            EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ALRCharacter::TryInteract);
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
        // 수색 중에 움직이려고 하면 즉시 수색 취소
        if (bIsSearching) 
        {
            CancelSearch();
        }
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
    if (bIsSearching) 
    {
        CancelSearch();
    }
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
    if (MovementState != ELRMovementState::Crouching)
        SetMovementState(ELRMovementState::Running);
}

void ALRCharacter::StopSprint(const FInputActionValue& Value)
{
    if (MovementState == ELRMovementState::Running)
        SetMovementState(ELRMovementState::Walking);
}

void ALRCharacter::ReportMovementNoise()
{
    if (!StatusComponent) return;

    const float CurrentSpeed = GetVelocity().Size2D();
    if (CurrentSpeed < 10.f) return;

    const float NoiseRadius = StatusComponent->GetNoiseRadius();
    if (NoiseRadius <= 0.f) return;

    const float Loudness = FMath::Clamp(NoiseRadius / 1500.f, 0.f, 1.f);

    UAISense_Hearing::ReportNoiseEvent(
        GetWorld(),
        GetActorLocation(),
        Loudness,
        this,
        NoiseRadius,
        NAME_None
    );
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

    // 캡슐 오프셋 조정
    FVector MeshLocation = GetMesh()->GetRelativeLocation();
    MeshLocation.Z = -NewHalfHeight;
    GetMesh()->SetRelativeLocation(MeshLocation);
    
    if (MovementState == ELRMovementState::Running)
    {
        StatusComponent->ConsumeStamina(StatusComponent->GetStaminaDrainRate() * DeltaTime);

        if (StatusComponent->IsStaminaEmpty())
        {
            SetMovementState(ELRMovementState::Walking);
        }
    }

    // 소음 보고
    NoiseMakeTimer += DeltaTime;
    if (!GetVelocity().IsZero() && NoiseMakeTimer >= 0.2f)
    {
        NoiseMakeTimer = 0.f;
        ReportMovementNoise();
    }

    // 소음 시각화 및 디버그
    if (StatusComponent->GetNoiseRadius() > 0.f)
    {
        FColor SphereColor;
        switch (MovementState)
        {
        case ELRMovementState::Crouching: SphereColor = FColor::Green; break;
        case ELRMovementState::Walking: SphereColor = FColor::Yellow; break;
        case ELRMovementState::Running: SphereColor = FColor::Red; break;
        default: SphereColor = FColor::White;
        }

        DrawDebugSphere(GetWorld(), GetActorLocation(), StatusComponent->GetNoiseRadius(), 16, SphereColor, false, -1.f);
    }

    GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan, FString::Printf(TEXT("Noise Radius: %.0f"), StatusComponent->GetNoiseRadius()));
    GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Yellow, FString::Printf(TEXT("Stamina: %.1f"), StatusComponent->GetStamina()));
    GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Red, FString::Printf(TEXT("Health: %.1f"), StatusComponent->GetHealth()));
    
    // --- 상호작용 프롬프트 (조준 중인 오브젝트 표시) ---
    if (!bIsSearching && Controller)
    {
        FVector Start;
        FRotator Rotation;
        Controller->GetPlayerViewPoint(Start, Rotation);
        FVector End = Start + Rotation.Vector() * 200.f;

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            if (ILRInteractable* Target = Cast<ILRInteractable>(Hit.GetActor()))
            {
                GEngine->AddOnScreenDebugMessage(20, 0.f, FColor::White, Target->GetInteractionPrompt().ToString());
            }
        }
    }

    // --- 수색 게이지 로직 추가 ---
    if (bIsSearching && CurrentInteractable)
    {
        CurrentSearchTime += DeltaTime;
        
        // (임시 UI) 화면에 0~100% 진행률 표시
        float Progress = CurrentSearchTime / SearchDuration;
        ILRInteractable* ProgressTarget = Cast<ILRInteractable>(CurrentInteractable);
        FString ProgressMsg = ProgressTarget
            ? FString::Printf(TEXT("%s %d%%"), *ProgressTarget->GetProgressText().ToString(), FMath::FloorToInt(Progress * 100))
            : FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Progress * 100));
        GEngine->AddOnScreenDebugMessage(10, 0.f, FColor::Green, ProgressMsg);

        // 3초 도달 시 완료 처리
        if (CurrentSearchTime >= SearchDuration)
        {
            ILRInteractable* InteractableTarget = Cast<ILRInteractable>(CurrentInteractable);
            if (InteractableTarget)
            {
                InteractableTarget->EndInteract(this); // 상자에게 완료되었다고 알림
            }
            
            bIsSearching = false;
            CurrentSearchTime = 0.f;
            CurrentInteractable = nullptr;
            
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("수색 완료!"));
        }
    }
}

float ALRCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (StatusComponent && Applied > 0.f)
    {
        CancelSearch();
        StatusComponent->ApplyDamage(Applied);
    }
    return Applied;
}

void ALRCharacter::OnHealthChanged(float NewHealth, float MaxHealth)
{
    if (NewHealth <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Character] Health 0 — Game Over"));

        if (ALRGameMode* GM = Cast<ALRGameMode>(GetWorld()->GetAuthGameMode()))
        {
            GM->OnPlayerDied(GetController());
        }
    }
}

void ALRCharacter::TryInteract()
{
    if (bIsSearching) return;
    if (Controller == nullptr) return;

    FVector Start;
    FRotator Rotation;
    Controller->GetPlayerViewPoint(Start, Rotation);

    FVector End = Start + (Rotation.Vector() * 200.0f); // 200 거리 탐색

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); 

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
    {
        AActor* HitActor = HitResult.GetActor();
        ILRInteractable* InteractableTarget = Cast<ILRInteractable>(HitActor);
        if (InteractableTarget)
        {
            CurrentInteractable = HitActor;
            SearchDuration = InteractableTarget->GetInteractionDuration();

            if (SearchDuration > 0.f)
            {
                bIsSearching = true;
                CurrentSearchTime = 0.f;
                InteractableTarget->BeginInteract(this);
                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, InteractableTarget->GetStartText().ToString());
            }
            else
            {
                InteractableTarget->BeginInteract(this);
                InteractableTarget->EndInteract(this);
                CurrentInteractable = nullptr;
            }
        }
    }
}

void ALRCharacter::CancelSearch()
{
    if (bIsSearching && CurrentInteractable)
    {
        ILRInteractable* InteractableTarget = Cast<ILRInteractable>(CurrentInteractable);
        if (InteractableTarget)
        {
            InteractableTarget->EndInteract(nullptr); 
        }
        
        bIsSearching = false;
        CurrentSearchTime = 0.f;
        CurrentInteractable = nullptr;
        
        ILRInteractable* CancelTarget = Cast<ILRInteractable>(CurrentInteractable);
        if (CancelTarget)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, CancelTarget->GetCancelText().ToString());
        }
    }
}