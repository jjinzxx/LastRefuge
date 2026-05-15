#include "AI/LRBotAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULRBotAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ALRBot* Bot = Cast<ALRBot>(GetOwningActor());
	if (!Bot) return;

	Speed = Bot->GetVelocity().Size2D();
	BotState = Bot->GetBotState();
}
