// UfoHudWidget.cpp

#include "UfoHudWidget.h"
#include "Components/ProgressBar.h"
#include "Kismet/GameplayStatics.h"
#include "UfoPawn.h"

void UUfoHudWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Get a reference to our UFO pawn.
	OwningPawn = Cast<AUfoPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void UUfoHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Ensure we have a valid pawn and health bar before updating.
	if (OwningPawn && HealthProgressBar)
	{
		// Update the health bar percentage based on the UFO's current health.
		const float HealthPercent = OwningPawn->CurrentHealth / OwningPawn->MaxHealth;
		HealthProgressBar->SetPercent(HealthPercent);
	}
}