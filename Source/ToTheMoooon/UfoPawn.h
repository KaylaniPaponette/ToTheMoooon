// UfoPawn.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UfoPawn.generated.h"

// Forward declarations
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UPhysicsHandleComponent;

UCLASS()
class TOTHEMOOOON_API AUfoPawn : public APawn
{
    GENERATED_BODY()

public:
    AUfoPawn();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // --- Components ---

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ShipMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USpringArmComponent* SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCameraComponent* Camera;

    // These components act as locations for applying thrust
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* LeftThruster;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RightThruster;

    // This component will grab and hold other physics objects
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPhysicsHandleComponent* PhysicsHandle;

    // --- Movement Properties ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float ThrustForce = 100000.0f; // Force for forward movement

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float TurnForce = 100000.0f; // Force used for turning

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float LevelingTurnSpeed = 5.0f; // How fast the ship rights itself

    // --- Tractor Beam Properties ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tractor Beam")
    float TractorBeamRange = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tractor Beam")
    float TractorBeamRadius = 250.0f; // The radius of the cone's end

private:
    // --- Input Functions ---
    void MoveHorizontal(float Value);
    void ThrustUp(float Value);
    void StartTractorBeam();
    void StopTractorBeam();

    // --- State & Helper Functions ---
    void HandleTractorBeam(float DeltaTime);

    bool bIsTractorBeamActive = false;
};