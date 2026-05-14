#include "Character/LRCharacter.h"
#include "DrawDebugHelpers.h"
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
#include "Components/LRInventoryGridComponent.h"
#include "UI/LRHudWidget.h"
#include "UI/LRInventoryGridWidget.h"
#include "UI/LRStorageWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
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

    InventoryGrid = CreateDefaultSubobject<ULRInventoryGridComponent>(TEXT("InventoryGrid"));
}

void ALRCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (InventoryGrid)
    {
        UE_LOG(LogTemp, Log, TEXT("Grid Inventory System Initialized."));

        // 레벨 이동 후 인벤토리 복원 (FLRGridItem — 위치 정보 포함)
        if (ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance()))
        {
            if (GI->bHasTravelData)
            {
                for (const FLRGridItem& Item : GI->PersistentInventory)
                {
                    if (!Item.IsEmpty())
                        InventoryGrid->PlaceItem(Item.GridX, Item.GridY, Item, Item.bIsRotated);
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

    // HUD 생성 (로컬 플레이어만)
    if (IsLocallyControlled() && HudWidgetClass)
    {
        ULRHudWidget* HudWidget = CreateWidget<ULRHudWidget>(GetWorld(), HudWidgetClass);
        if (HudWidget)
        {
            HudWidget->AddToViewport();
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

        if (IA_Interact)
            EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ALRCharacter::TryInteract);

        // 인벤토리 열기/닫기 (Tab키 — 에디터에서 IA_Inventory 할당 필요)
        if (IA_Inventory)
            EIC->BindAction(IA_Inventory, ETriggerEvent::Started, this, &ALRCharacter::ToggleInventory);
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
    if (bIgnoreLookInput) return;

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

    // // 소음 시각화 디버그 (비활성화)
    // if (StatusComponent->GetNoiseRadius() > 0.f)
    // {
    //     FColor SphereColor;
    //     switch (MovementState)
    //     {
    //     case ELRMovementState::Crouching: SphereColor = FColor::Green; break;
    //     case ELRMovementState::Walking: SphereColor = FColor::Yellow; break;
    //     case ELRMovementState::Running: SphereColor = FColor::Red; break;
    //     default: SphereColor = FColor::White;
    //     }
    //     DrawDebugSphere(GetWorld(), GetActorLocation(), StatusComponent->GetNoiseRadius(), 16, SphereColor, false, -1.f);
    // }
    // GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan, FString::Printf(TEXT("Noise Radius: %.0f"), StatusComponent->GetNoiseRadius()));
    // GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Yellow, FString::Printf(TEXT("Stamina: %.1f"), StatusComponent->GetStamina()));
    // GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Red, FString::Printf(TEXT("Health: %.1f"), StatusComponent->GetHealth()));
    
    // --- 소음 리플 링 ---
    if (StatusComponent)
    {
        const float NoiseRadius   = StatusComponent->GetNoiseRadius();
        const float NoisePercent  = FMath::Clamp(NoiseRadius / 1200.f, 0.f, 1.f);
    
        // 1. 캐릭터의 이동 속도 확인 (Size 또는 Size2D 사용)
        const float CurrentSpeed = GetVelocity().Size();

        // 2. 소음 반경이 있고 + 캐릭터가 일정 속도 이상으로 움직일 때만 실행
        // Speed > 10.f 는 완전히 멈춰있지 않을 때를 의미합니다.
        if (NoisePercent > 0.01f && CurrentSpeed > 10.f)
        {
            const FVector GroundPos = GetActorLocation() -
                FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 2.f);

            const float Speed = NoiseRingCycleSpeed * NoisePercent;

            for (int32 i = 0; i < 3; i++)
            {
                NoiseRingPhase[i] = FMath::Fmod(NoiseRingPhase[i] + Speed * DeltaTime, 1.f);
                const float t = NoiseRingPhase[i];

                const float Radius = FMath::Lerp(30.f, NoiseRadius, t);
            
                // 흰색 고정 (멀어질수록 선 두께를 얇게 해서 자연스럽게 제거)
                const float CurrentThickness = FMath::Lerp(2.0f, 0.0f, t);

                DrawDebugCircle(
                    GetWorld(), GroundPos, Radius,
                    40, FColor::White,
                    false, -1.f, 0, CurrentThickness,
                    FVector::ForwardVector, FVector::RightVector,
                    false
                );
            }
        }
        else
        {
            // 움직임이 멈췄을 때 링의 애니메이션 단계를 초기화하고 싶다면 여기서 처리 가능
            // for (int32 i = 0; i < 3; i++) { NoiseRingPhase[i] = 0.f; }
        }
    }

    // --- 상호작용 프롬프트 (조준 중인 오브젝트 표시) ---
    // 보관함이 열려 있으면 "E: 보관함 닫기" 고정 표시
    if (bStorageOpen)
    {
        FText CloseText = FText::FromString(TEXT("[E] 보관함 닫기"));
        if (!CloseText.EqualTo(CurrentPromptText))
        {
            CurrentPromptText = CloseText;
            OnInteractionPromptChanged.Broadcast(CurrentPromptText);
        }
    }
    else if (!bIsSearching && !bInventoryOpen && Controller)
    {
        FVector Start;
        FRotator Rotation;
        Controller->GetPlayerViewPoint(Start, Rotation);
        FVector End = Start + Rotation.Vector() * 200.f;

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        FText NewPrompt = FText::GetEmpty();
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            if (ILRInteractable* Target = Cast<ILRInteractable>(Hit.GetActor()))
            {
                NewPrompt = Target->GetInteractionPrompt();
            }
        }

        // 프롬프트가 바뀔 때만 브로드캐스트 (매 프레임 불필요한 UI 갱신 방지)
        if (!NewPrompt.EqualTo(CurrentPromptText))
        {
            CurrentPromptText = NewPrompt;
            OnInteractionPromptChanged.Broadcast(CurrentPromptText);
        }

        // GEngine->AddOnScreenDebugMessage(20, 0.f, FColor::White, CurrentPromptText.ToString());
    }

    // --- 수색 게이지 로직 추가 ---
    if (bIsSearching && CurrentInteractable)
    {
        CurrentSearchTime += DeltaTime;
        
        // 진행률 계산 및 브로드캐스트
        float Progress = CurrentSearchTime / SearchDuration;
        OnSearchProgressChanged.Broadcast(Progress);

        // // (임시 디버그) 화면에 진행률 표시 (비활성화)
        // ILRInteractable* ProgressTarget = Cast<ILRInteractable>(CurrentInteractable);
        // FString ProgressMsg = ProgressTarget
        //     ? FString::Printf(TEXT("%s %d%%"), *ProgressTarget->GetProgressText().ToString(), FMath::FloorToInt(Progress * 100))
        //     : FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Progress * 100));
        // GEngine->AddOnScreenDebugMessage(10, 0.f, FColor::Green, ProgressMsg);

        // 완료 처리
        if (CurrentSearchTime >= SearchDuration)
        {
            ILRInteractable* InteractableTarget = Cast<ILRInteractable>(CurrentInteractable);
            FText CompleteText = InteractableTarget
                ? InteractableTarget->GetCompleteText()
                : FText::GetEmpty();

            if (InteractableTarget)
            {
                InteractableTarget->EndInteract(this);
            }

            bIsSearching = false;
            CurrentSearchTime = 0.f;
            CurrentInteractable = nullptr;

            OnSearchEnded.Broadcast(true, CompleteText);
            // GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("완료!"));
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
    if (bStorageOpen)
    {
        CloseStorageScreen();
        return;
    }
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
                OnSearchStarted.Broadcast(InteractableTarget->GetProgressText());
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

        FText CancelText = InteractableTarget
            ? InteractableTarget->GetCancelText()
            : FText::GetEmpty();

        if (InteractableTarget)
            InteractableTarget->EndInteract(nullptr);

        bIsSearching = false;
        CurrentSearchTime = 0.f;
        CurrentInteractable = nullptr;

        OnSearchEnded.Broadcast(false, CancelText);
    }
}

void ALRCharacter::ToggleInventory()
{
    // 보관함이 열려 있으면 인벤토리를 열지 않음
    if (bStorageOpen) return;

    UE_LOG(LogTemp, Warning, TEXT("[Inventory] ToggleInventory 호출됨"));

    if (!InventoryWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Inventory] InventoryWidgetClass가 null — BP_LRCharacter에서 WBP_InventoryGrid 할당 필요"));
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[Inventory] PlayerController null"));
        return;
    }

    if (!bInventoryOpen)
    {
        if (!InventoryWidget)
        {
            InventoryWidget = CreateWidget<ULRInventoryGridWidget>(PC, InventoryWidgetClass);
            if (!InventoryWidget)
            {
                UE_LOG(LogTemp, Error, TEXT("[Inventory] CreateWidget 실패"));
                return;
            }
            constexpr float SlotSizePx = 60.f;
            InventoryWidget->InitGrid(InventoryGrid, nullptr, SlotSizePx);
            UE_LOG(LogTemp, Warning, TEXT("[Inventory] 위젯 생성 완료"));
        }

        InventoryWidget->AddToViewport(5);
        {
            const float W = InventoryGrid->GridWidth  * 60.f;
            const float H = InventoryGrid->GridHeight * 60.f;
            InventoryWidget->SetDesiredSizeInViewport(FVector2D(W, H));

            const float DPI = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
            FVector2D ScreenSize;
            GEngine->GameViewport->GetViewportSize(ScreenSize);
            InventoryWidget->SetPositionInViewport(
                (ScreenSize / DPI - FVector2D(W, H)) * 0.5f, false);
        }
        PC->SetShowMouseCursor(true);
        {
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(Mode);
        }
        PC->SetIgnoreMoveInput(true);
        bIgnoreLookInput = true;
        UE_LOG(LogTemp, Warning, TEXT("[Inventory] 열림"));
    }
    else
    {
        if (InventoryWidget)
        {
            InventoryWidget->RemoveFromParent();
            PC->SetShowMouseCursor(false);
            PC->SetInputMode(FInputModeGameOnly());
            PC->ResetIgnoreMoveInput();
            bIgnoreLookInput = false;
        }
    }

    bInventoryOpen = !bInventoryOpen;
}

// ──────────────────────────────────────────────────────────
// OpenStorageScreen / CloseStorageScreen
// ALRStorage::EndInteract 에서 호출됨.
// 이미 열려 있으면 E 재입력 시 닫힘.
// ──────────────────────────────────────────────────────────
void ALRCharacter::OpenStorageScreen(ULRInventoryGridComponent* InStorageGrid)
{
    if (bStorageOpen)
    {
        CloseStorageScreen();
        return;
    }

    if (!StorageWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Storage] StorageWidgetClass null — BP_LRCharacter에서 WBP_Storage 할당 필요"));
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    // 매번 새로 생성하여 올바른 StorageGrid로 초기화
    StorageWidget = CreateWidget<ULRStorageWidget>(PC, StorageWidgetClass);
    if (!StorageWidget) return;

    StorageWidget->InitStorage(InventoryGrid, InStorageGrid);
    StorageWidget->AddToViewport(5);

    PC->SetShowMouseCursor(true);
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
    }
    PC->SetIgnoreMoveInput(true);
    bIgnoreLookInput = true;

    bStorageOpen = true;
    UE_LOG(LogTemp, Warning, TEXT("[Storage] 보관함 UI 열림"));
}

void ALRCharacter::CloseStorageScreen()
{
    if (StorageWidget)
        StorageWidget->RemoveFromParent();

    StorageWidget    = nullptr;
    bStorageOpen     = false;
    bIgnoreLookInput = false;

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(FInputModeGameOnly());
        PC->ResetIgnoreMoveInput();
    }

    UE_LOG(LogTemp, Warning, TEXT("[Storage] 보관함 UI 닫힘"));
}