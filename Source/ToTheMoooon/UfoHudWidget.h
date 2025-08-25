// UfoHudWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UfoHudWidget.generated.h"

class UProgressBar;
class AUfoPawn;

UCLASS()
class TOTHEMOOOON_API UUfoHudWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// This function is called every frame for widgets.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// This function is called when the widget is constructed.
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthProgressBar;

	// A pointer to our UFO pawn.
	UPROPERTY()
	AUfoPawn* OwningPawn;
};