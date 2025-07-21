
// MenuGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

UCLASS()
class TOTHEMOOOON_API AMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

protected:
    // Called when the game starts.
    virtual void BeginPlay() override;

public:
    // A reference to our Widget Blueprint class.
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> MainMenuWidgetClass;
};