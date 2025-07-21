// MainMenuWidget.cpp

#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Bind our C++ functions to the OnClicked events of the buttons.
    if (PlayButton)
    {
        PlayButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnPlayClicked);
    }

    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);
    }
}

void UMainMenuWidget::OnPlayClicked()
{
    // Get a reference to the world and open the game level.
    UGameplayStatics::OpenLevel(this, FName("GameMap"));
}

void UMainMenuWidget::OnQuitClicked()
{
    // Get the player controller and tell the game to quit.
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (PlayerController)
    {
        PlayerController->ConsoleCommand("quit");
    }
}