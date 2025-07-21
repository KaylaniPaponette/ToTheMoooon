// MainMenuWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

// Forward declare the Button class
class UButton;

UCLASS()
class TOTHEMOOOON_API UMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    // This is like an "Awake" or "Start" function for widgets.
    virtual void NativeConstruct() override;

private:
    // Binds this C++ variable to the UMG widget named "PlayButton".
    UPROPERTY(meta = (BindWidget))
    UButton* PlayButton;

    // Binds this C++ variable to the UMG widget named "QuitButton".
    UPROPERTY(meta = (BindWidget))
    UButton* QuitButton;

    // Function that will be called when the Play button is clicked.
    UFUNCTION()
    void OnPlayClicked();

    // Function that will be called when the Quit button is clicked.
    UFUNCTION()
    void OnQuitClicked();
};