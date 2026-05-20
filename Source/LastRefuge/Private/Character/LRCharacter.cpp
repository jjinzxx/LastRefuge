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
#include "UI/LRPauseMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Items/LRItemDataAsset.h"
#include "Interfaces/LRInteractable.h"
#include "AI/LRBot.h"
#include "Components/AudioComponent.h"
#include "Components/LineBatchComponent.h"
#include "Actors/LRStorage.h"
#include "EngineUtils.h"
#include "LRSaveGame.h"

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
    ToolbarItems.SetNum(ToolbarSize);

    FootstepAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("FootstepAudio"));
    FootstepAudioComp->SetupAttachment(GetRootComponent());
    FootstepAudioComp->bAutoActivate = false;

    NoiseRingBatch = CreateDefaultSubobject<ULineBatchComponent>(TEXT("NoiseRingBatch"));
    NoiseRingBatch->SetupAttachment(GetRootComponent());
    NoiseRingBatch->bCalculateAccurateBounds = false;
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
                // 레벨 이동 후 GI 메모리에서 복원
                for (const FLRGridItem& Item : GI->PersistentInventory)
                    if (!Item.IsEmpty())
                        InventoryGrid->PlaceItem(Item.GridX, Item.GridY, Item, Item.bIsRotated);

                for (int32 i = 0; i < GI->PersistentToolbarItems.Num() && i < ToolbarSize; ++i)
                    ToolbarItems[i] = GI->PersistentToolbarItems[i];
            }
            else
            {
                // 게임 최초 실행 or 재실행 — 디스크 세이브에서 복원
                ULRInventoryGridComponent* StorageGrid = nullptr;
                for (TActorIterator<ALRStorage> It(GetWorld()); It; ++It)
                {
                    StorageGrid = It->GetStorageGrid();
                    break;
                }

                if (StorageGrid)
                {
                    // TObjectPtr → raw pointer 변환
                    TMap<FString, ULRItemDataAsset*> Registry;
                    for (const auto& [Key, Val] : GI->ItemRegistry)
                        Registry.Add(Key, Val.Get());

                    TArray<FLRGridItem> OutToolbar;
                    ULRSaveGame::Load(InventoryGrid, StorageGrid,
                        OutToolbar, Registry, this);

                    for (int32 i = 0; i < OutToolbar.Num() && i < ToolbarSize; ++i)
                        ToolbarItems[i] = OutToolbar[i];
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
    
    // 레벨 전환 후 Input Mode 초기화 (메인 메뉴 → 게임 진입 시 UI 모드 잔존 방지)
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(FInputModeGameOnly());
        PC->ResetIgnoreMoveInput();
    }

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    StatusComponent->OnHealthChanged.AddDynamic(this, &ALRCharacter::OnHealthChanged);

    // 인벤토리 변경 시 무게 기반 이동속도 재계산
    InventoryGrid->OnGridChanged.AddUObject(this, &ALRCharacter::UpdateMovementSpeed);

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

        if (IA_Toolbar1) EIC->BindAction(IA_Toolbar1, ETriggerEvent::Started, this, &ALRCharacter::HandleToolbar1);
        if (IA_Toolbar2) EIC->BindAction(IA_Toolbar2, ETriggerEvent::Started, this, &ALRCharacter::HandleToolbar2);
        if (IA_Toolbar3) EIC->BindAction(IA_Toolbar3, ETriggerEvent::Started, this, &ALRCharacter::HandleToolbar3);
        if (IA_Toolbar4) EIC->BindAction(IA_Toolbar4, ETriggerEvent::Started, this, &ALRCharacter::HandleToolbar4);

        if (IA_Menu)
            EIC->BindAction(IA_Menu, ETriggerEvent::Started, this, &ALRCharacter::TogglePauseMenu);

        if (IA_Takedown)
            EIC->BindAction(IA_Takedown, ETriggerEvent::Started, this, &ALRCharacter::TryTakedown);

        if (IA_Attack)
            EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ALRCharacter::TryAttack);
    }
}

void ALRCharacter::UpdateMovementSpeed()
{
    const float Weight      = InventoryGrid ? InventoryGrid->GetTotalWeight() : 0.f;
    const float WeightRatio = FMath::Clamp(Weight / FMath::Max(MaxCarryWeight, 1.f), 0.f, 1.f);
    const float SpeedMult   = FMath::Lerp(1.0f, 0.5f, WeightRatio);

    float BaseSpeed;
    switch (MovementState)
    {
    case ELRMovementState::Crouching: BaseSpeed = CrouchSpeed; break;
    case ELRMovementState::Running:   BaseSpeed = RunSpeed;    break;
    default:                          BaseSpeed = WalkSpeed;   break;
    }
    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * SpeedMult;
}

void ALRCharacter::SetMovementState(ELRMovementState NewState)
{
    if (MovementState == NewState) return;

    MovementState = NewState;

    switch (MovementState)
    {
    case ELRMovementState::Crouching:
        TargetCameraHeight = CrouchCameraHeight;
        TargetCapsuleHalfHeight = CrouchCapsuleHalfHeight;
        StatusComponent->UpdateNoiseRadius(StatusComponent->CrouchNoiseRadius);
        break;

    case ELRMovementState::Walking:
        TargetCameraHeight = StandCameraHeight;
        TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
        StatusComponent->UpdateNoiseRadius(StatusComponent->WalkNoiseRadius);
        break;

    case ELRMovementState::Running:
        TargetCameraHeight = StandCameraHeight;
        TargetCapsuleHalfHeight = StandCapsuleHalfHeight;
        StatusComponent->UpdateNoiseRadius(StatusComponent->RunNoiseRadius);
        break;
    }

    UpdateMovementSpeed();
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
    if (!Controller) return;

    float Sensitivity = 1.0f;
    if (ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance()))
        Sensitivity = GI->MouseSensitivity;

    AddControllerYawInput(LookVector.X * Sensitivity);
    AddControllerPitchInput(LookVector.Y * Sensitivity);
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
        if (GetVelocity().Size2D() > 10.f)
            StatusComponent->ConsumeStamina(StatusComponent->GetStaminaDrainRate() * DeltaTime);

        if (StatusComponent->IsStaminaEmpty())
            SetMovementState(ELRMovementState::Walking);
    }

    // 발자국 사운드
    const float Speed2D = GetVelocity().Size2D();
    if (Speed2D > 10.f && SFX_Footstep_Walk && FootstepAudioComp)
    {
        float Pitch = 1.0f;
        float Volume = 1.0f;
        if (MovementState == ELRMovementState::Running)
        {
            Pitch  = FootstepRunPitch;
            Volume = FootstepRunVolume;
        }
        else if (MovementState == ELRMovementState::Crouching)
        {
            Pitch  = FootstepCrouchPitch;
            Volume = FootstepCrouchVolume;
        }

        FootstepAudioComp->SetPitchMultiplier(Pitch);
        FootstepAudioComp->SetVolumeMultiplier(Volume);

        if (FootstepAudioComp->GetSound() != SFX_Footstep_Walk)
            FootstepAudioComp->SetSound(SFX_Footstep_Walk);

        if (!FootstepAudioComp->IsPlaying())
            FootstepAudioComp->Play();
    }
    else if (FootstepAudioComp && FootstepAudioComp->IsPlaying())
    {
        FootstepAudioComp->Stop();
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
        // 매 프레임 이전 선 지우기
        NoiseRingBatch->Flush();

        if (NoisePercent > 0.01f && CurrentSpeed > 10.f)
        {
            const FVector GroundPos = GetActorLocation() -
                FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 2.f);

            const float Speed = NoiseRingCycleSpeed * NoisePercent;
            constexpr int32 Segments = 40;
            constexpr float AngleStep = 2.f * PI / Segments;

            for (int32 i = 0; i < 3; i++)
            {
                NoiseRingPhase[i] = FMath::Fmod(NoiseRingPhase[i] + Speed * DeltaTime, 1.f);
                const float t     = NoiseRingPhase[i];
                const float Radius    = FMath::Lerp(30.f, NoiseRadius, t);
                const float Thickness = FMath::Lerp(2.0f, 0.0f, t);
                const FLinearColor Color(1.f, 1.f, 1.f, FMath::Lerp(1.f, 0.f, t));

                FVector Prev = GroundPos + FVector(Radius, 0.f, 2.f);
                for (int32 s = 1; s <= Segments; ++s)
                {
                    const float Angle = s * AngleStep;
                    FVector Next = GroundPos + FVector(
                        FMath::Cos(Angle) * Radius,
                        FMath::Sin(Angle) * Radius,
                        2.f);
                    NoiseRingBatch->DrawLine(Prev, Next, Color, 0, Thickness, 0.f);
                    Prev = Next;
                }
            }
        }
    }

    // --- 제압 프롬프트 ---
    if (!bStorageOpen && !bInventoryOpen)
    {
        const float TakedownRange = 150.f;
        const float TakedownAngle = 60.f;
        bool bCanTakedown = false;

        TArray<AActor*> NearBots;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALRBot::StaticClass(), NearBots);
        for (AActor* Actor : NearBots)
        {
            ALRBot* Bot = Cast<ALRBot>(Actor);
            if (!Bot || Bot->bIsDead) continue;
            if (FVector::Dist(GetActorLocation(), Bot->GetActorLocation()) > TakedownRange) continue;

            const float Angle = FMath::RadiansToDegrees(FMath::Acos(
                FVector::DotProduct(Bot->GetActorForwardVector(),
                    (GetActorLocation() - Bot->GetActorLocation()).GetSafeNormal())));
            if (Angle >= (180.f - TakedownAngle)) { bCanTakedown = true; break; }
        }

        if (bCanTakedown)
        {
            FText TakedownPrompt = FText::FromString(TEXT("[F] 제압하기"));
            if (!TakedownPrompt.EqualTo(CurrentPromptText))
            {
                CurrentPromptText = TakedownPrompt;
                OnInteractionPromptChanged.Broadcast(CurrentPromptText);
            }
            return; // 제압 프롬프트 표시 중엔 다른 프롬프트 표시 안 함
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
        if (SFX_Hit)
            UGameplayStatics::PlaySoundAtLocation(this, SFX_Hit, GetActorLocation());
    }
    return Applied;
}

void ALRCharacter::OnHealthChanged(float NewHealth, float MaxHealth)
{
    if (NewHealth <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Character] Health 0 — Game Over"));

        if (SFX_Death)
            UGameplayStatics::PlaySoundAtLocation(this, SFX_Death, GetActorLocation());

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
    if (bInventoryOpen) return;
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
            // 잠금 등 접근 차단 확인
            if (!InteractableTarget->CanInteract(this))
            {
                OnInteractionPromptChanged.Broadcast(
                    FText::FromString(TEXT("잠겨 있습니다. 보안 키카드가 필요합니다.")));
                return;
            }

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

void ALRCharacter::TryTakedown()
{
    const float TakedownRange = 150.f;
    const float TakedownAngle = 60.f; // 등 방향 ±60도

    TArray<AActor*> NearBots;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALRBot::StaticClass(), NearBots);

    for (AActor* Actor : NearBots)
    {
        ALRBot* Bot = Cast<ALRBot>(Actor);
        if (!Bot || Bot->bIsDead) continue;

        const float Dist = FVector::Dist(GetActorLocation(), Bot->GetActorLocation());
        if (Dist > TakedownRange) continue;

        // 봇의 등 방향 체크 — 플레이어가 봇 뒤에 있는지
        const FVector ToBotForward = Bot->GetActorForwardVector();
        const FVector ToPlayer = (GetActorLocation() - Bot->GetActorLocation()).GetSafeNormal();
        const float DotProduct = FVector::DotProduct(ToBotForward, ToPlayer);
        const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

        // DotProduct > 0 이면 플레이어가 봇 앞쪽 → 제압 불가
        // AngleDeg > 180 - TakedownAngle 이면 봇 등 방향
        if (AngleDeg < (180.f - TakedownAngle)) continue;

        Bot->TakedownKill();
        UE_LOG(LogTemp, Warning, TEXT("[Takedown] 제압 성공!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Takedown] 제압 실패 — 조건 미충족"));
}

void ALRCharacter::TryAttack(const FInputActionValue& Value)
{
    if (!bCanAttack) return;
    if (bInventoryOpen || bStorageOpen || bPauseMenuOpen) return;
    if (!Controller) return;

    bCanAttack = false;
    GetWorldTimerManager().SetTimer(AttackCooldownHandle, [this]()
    {
        bCanAttack = true;
    }, AttackCooldown, false);

    if (SFX_Attack)
        UGameplayStatics::PlaySoundAtLocation(this, SFX_Attack, GetActorLocation());

    FVector Start;
    FRotator Rotation;
    Controller->GetPlayerViewPoint(Start, Rotation);
    const FVector End = Start + Rotation.Vector() * AttackRange;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(ECC_Pawn);
    if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjParams, Params))
    {
        if (ALRBot* Bot = Cast<ALRBot>(Hit.GetActor()))
        {
            UGameplayStatics::ApplyDamage(Bot, AttackDamage, GetController(), this, nullptr);
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
            UE_LOG(LogTemp, Warning, TEXT("[Inventory] 위젯 생성 완료"));
        }

        // NativeDestruct가 OnGridChanged 핸들을 제거하므로 열 때마다 재등록·재빌드 필요
        constexpr float SlotSizePx = 60.f;
        InventoryWidget->InitGrid(InventoryGrid, nullptr, SlotSizePx);

        // AddToViewport → 풀스크린 오버레이로 추가.
        // InitGrid에서 GridCanvas 앵커를 (0.5, 0.5)로 설정하므로
        // UMG가 자동으로 화면 중앙에 배치 — 수동 DPI 계산 불필요.
        InventoryWidget->AddToViewport(5);
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

// ──────────────────────────────────────────────────────────
// 툴바
// ──────────────────────────────────────────────────────────
void ALRCharacter::SetToolbarSlot(int32 SlotIndex, const FLRGridItem& Item)
{
    if (!ToolbarItems.IsValidIndex(SlotIndex)) return;
    ToolbarItems[SlotIndex] = Item;
    OnToolbarSlotChanged.Broadcast(SlotIndex, Item.ItemData, Item.Quantity);
}

void ALRCharacter::ClearToolbarSlot(int32 SlotIndex)
{
    if (!ToolbarItems.IsValidIndex(SlotIndex)) return;

    FLRGridItem Item = ToolbarItems[SlotIndex];
    if (!Item.IsEmpty() && InventoryGrid)
    {
        int32 OutX, OutY; bool bRot;
        if (InventoryGrid->FindEmptySpace(Item, OutX, OutY, bRot))
            InventoryGrid->PlaceItem(OutX, OutY, Item, bRot);
        else
            UE_LOG(LogTemp, Warning, TEXT("[Toolbar] ClearSlot %d: 인벤 공간 없음 — 아이템 유실"), SlotIndex);
    }

    ToolbarItems[SlotIndex] = FLRGridItem();
    OnToolbarSlotChanged.Broadcast(SlotIndex, nullptr, 0);
}

FLRGridItem ALRCharacter::TakeToolbarItem(int32 SlotIndex)
{
    if (!ToolbarItems.IsValidIndex(SlotIndex)) return FLRGridItem();
    FLRGridItem Item = ToolbarItems[SlotIndex];
    ToolbarItems[SlotIndex] = FLRGridItem();
    OnToolbarSlotChanged.Broadcast(SlotIndex, nullptr, 0);
    return Item;
}

void ALRCharacter::UseToolbarSlot(int32 SlotIndex)
{
    if (!ToolbarItems.IsValidIndex(SlotIndex)) return;
    if (bInventoryOpen || bStorageOpen) return;

    FLRGridItem& Item = ToolbarItems[SlotIndex];
    if (Item.IsEmpty() || !Item.ItemData) return;
    if (Item.ItemData->ItemType != ELRItemType::Consumable) return;

    if (StatusComponent)
    {
        StatusComponent->RestoreHealth(Item.ItemData->HealthRestore);
        StatusComponent->RestoreStamina(Item.ItemData->StaminaRestore);
    }

    if (SFX_ItemUse)
        UGameplayStatics::PlaySoundAtLocation(this, SFX_ItemUse, GetActorLocation());

    Item.Quantity--;
    if (Item.Quantity <= 0)
    {
        ToolbarItems[SlotIndex] = FLRGridItem();
        OnToolbarSlotChanged.Broadcast(SlotIndex, nullptr, 0);
    }
    else
    {
        OnToolbarSlotChanged.Broadcast(SlotIndex, Item.ItemData, Item.Quantity);
    }
}

// ──────────────────────────────────────────────────────────
// ESC 일시정지 메뉴
// ──────────────────────────────────────────────────────────
void ALRCharacter::TogglePauseMenu()
{
    // 인벤토리/보관함이 열려 있으면 ESC로 닫기
    if (bInventoryOpen) { ToggleInventory(); return; }
    if (bStorageOpen)   { CloseStorageScreen(); return; }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    if (!bPauseMenuOpen)
    {
        if (!PauseMenuWidgetClass)
        {
            UE_LOG(LogTemp, Error, TEXT("[PauseMenu] PauseMenuWidgetClass null — BP_LRCharacter에서 WBP_PauseMenu 할당 필요"));
            return;
        }
        if (!PauseMenuWidget)
            PauseMenuWidget = CreateWidget<ULRPauseMenuWidget>(PC, PauseMenuWidgetClass);
        if (!PauseMenuWidget) return;

        PauseMenuWidget->AddToViewport(10);
        PC->SetShowMouseCursor(true);
        PC->SetInputMode(FInputModeGameAndUI());
        UGameplayStatics::SetGamePaused(this, true);
        bPauseMenuOpen   = true;
        bIgnoreLookInput = true;
    }
    else
    {
        PauseMenuWidget->RemoveFromParent();
        PauseMenuWidget  = nullptr;
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(FInputModeGameOnly());
        UGameplayStatics::SetGamePaused(this, false);
        bPauseMenuOpen   = false;
        bIgnoreLookInput = false;
    }
}

void ALRCharacter::SaveCurrentState()
{
    ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance());
    if (!GI) return;

    // 인벤토리 → GI
    if (ULRInventoryGridComponent* InvGrid = GetInventoryGrid())
    {
        GI->PersistentInventory.Empty();
        for (const auto& [ID, Item] : InvGrid->GetItems())
            if (!Item.IsEmpty())
                GI->PersistentInventory.Add(Item);
    }

    // 툴바 → GI
    GI->PersistentToolbarItems = GetToolbarItems();

    // 보관함 → GI + SaveGame에 넘길 포인터 확보
    GI->PersistentStorageItems.Empty();
    ULRInventoryGridComponent* StorageGrid = nullptr;
    for (TActorIterator<ALRStorage> It(GetWorld()); It; ++It)
    {
        StorageGrid = It->GetStorageGrid();
        if (StorageGrid)
            for (const auto& [ID, Item] : StorageGrid->GetItems())
                if (!Item.IsEmpty())
                    GI->PersistentStorageItems.Add(Item);
        break;
    }

    GI->bHasTravelData = true;

    // 디스크 저장 — StorageGrid가 없는 맵(DangerZone)은 GI 캐시를 폴백으로 전달
    ULRSaveGame::Save(GetInventoryGrid(), StorageGrid, GetToolbarItems(), this,
        StorageGrid ? nullptr : &GI->PersistentStorageItems);
}

void ALRCharacter::SaveAndGoToMainMenu()
{
    SaveCurrentState();
    UGameplayStatics::SetGamePaused(this, false);
    UGameplayStatics::OpenLevel(this, FName("L_MainMenu"));
}