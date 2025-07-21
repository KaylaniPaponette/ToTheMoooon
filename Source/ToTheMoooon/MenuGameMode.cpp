// MenuGameMode.cpp

#include "MenuGameMode.h"
#include "Blueprint/UserWidget.h"

void AMenuGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Check if we have a valid widget class, then create it and add to viewport.
    if (MainMenuWidgetClass)
    {
        UUserWidget* MainMenuWidget = CreateWidget<UUserWidget>(GetWorld(), MainMenuWidgetClass);
        if (MainMenuWidget)
        {
            MainMenuWidget->AddToViewport();

            // Get the player controller and set UI input mode.
            APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
            if (PlayerController)
            {
                FInputModeUIOnly InputMode;
                InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
                PlayerController->SetInputMode(InputMode);
                PlayerController->bShowMouseCursor = true;
            }
        }
    }
}