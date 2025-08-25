// UfoPawn.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/UserWidget.h"
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* LeftThruster;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RightThruster;

	// Added for the gravity gun
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPhysicsHandleComponent* PhysicsHandle;

	// --- Movement Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float UpThrustForce = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MainThrustForce = 40000.0f;

		// ======== OLD: Direct left/right movement method =========
		//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
		//float HorizontalForce = 50000.0f;

	// RollForce for the axis-based movement method
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RollForce = 30000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LevelingTurnSpeed = 5.0f;

	// --- Health System Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Over")
	FName GameOverMapName;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> HudWidgetClass;

	// --- Gravity Gun Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Gun")
	float GravityGunRange = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Gun")
	float GravityGunRadius = 80.0f;

	// Distance below the ship to hold the object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Gun")
	float GravityGunHoldDistance = 125.0f;

	// Shrinking mechanic
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Gun")
	float ShrinkFactor = 0.9f; // Shrink object to 90% of its current size

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Gun")
	float MinScale = 0.1f; // Smallest an object can get 10% of original size

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Gun")
	//float GravityGunPullForce = 2500.0f;

private:
	// --- Input Functions for Action-based thrusters ---
	void ThrustLeftPressed();
	void ThrustLeftReleased();
	void ThrustRightPressed();
	void ThrustRightReleased();

	// --- Input Functions for Axis-based movement ---
	void MoveHorizontal(float Value);
	void ThrustUp(float Value);

	// --- Input Functions for Hover ---
	void HoverPressed();
	void HoverReleased();

	// --- Helper function to apply forces every frame ---
	void ApplyThrusterForces();

	// --- Input Functions for Gravity Gun ---
	void StartGravityGun();
	void StopGravityGun();
	void RotateGrabbedObject();
	void ShrinkGrabbedObject();


	// --- State & Helper Functions ---
	void HandleHovering(float DeltaTime);
	void HandleGravityGun(float DeltaTime);

	// --- Health System Functions ---
	void HandleDamage(float DamageAmount);

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// --- State variables to track input ---
	bool bIsLeftThrusterActive = false;
	bool bIsRightThrusterActive = false;

	bool bIsHoverActive = false;
	//bool bIsGravityGunActive = false;
	UPrimitiveComponent* GrabbedComponent = nullptr;

	float LockedXPosition;

	FRotator GrabbedObjectRotation;

	//// An array to hold all the components grabbed with multi-grab method.
	//TArray<UPrimitiveComponent*> GrabbedComponents;

};









//// ========================= NO ROLL FORCE METHOD ===========================
//// ========================= SIMPLIFY HOVER LOGIC ===========================
//// UfoPawn.h
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "GameFramework/Pawn.h"
//#include "UfoPawn.generated.h"
//
//// Forward declarations
//class UStaticMeshComponent;
//class USpringArmComponent;
//class UCameraComponent;
//
//UCLASS()
//class TOTHEMOOOON_API AUfoPawn : public APawn
//{
//	GENERATED_BODY()
//
//public:
//	AUfoPawn();
//
//protected:
//	virtual void BeginPlay() override;
//
//public:
//	virtual void Tick(float DeltaTime) override;
//	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
//
//	// --- Components ---
//
//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//	UStaticMeshComponent* ShipMesh;
//
//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//	USpringArmComponent* SpringArm;
//
//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//	UCameraComponent* Camera;
//
//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//	USceneComponent* LeftThruster;
//
//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//	USceneComponent* RightThruster;
//
//	// REMOVED: The PhysicsHandleComponent is no longer needed.
//
//	// --- Movement Properties ---
//
//	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
//	float MainThrustForce = 40000.0f;
//
//	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
//	float LevelingTurnSpeed = 5.0f;
//
//	// REMOVED: Tractor Beam properties are no longer needed.
//
//private:
//	// --- Input Functions for thrusters ---
//	void ThrustLeftPressed();
//	void ThrustLeftReleased();
//	void ThrustRightPressed();
//	void ThrustRightReleased();
//
//	// --- RENAMED: Input Functions for Hover ---
//	void HoverPressed();
//	void HoverReleased();
//
//	// --- Helper function to apply forces every frame ---
//	void ApplyThrusterForces();
//
//	// --- RENAMED: State & Helper Functions ---
//	void HandleHovering(float DeltaTime);
//
//	// --- State variables to track input ---
//	bool bIsLeftThrusterActive = false;
//	bool bIsRightThrusterActive = false;
//
//	// RENAMED: State variable for hover
//	bool bIsHoverActive = false;
//};



//========================= ROLL FORCE METHOD ==================================================
// // UfoPawn.h
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "GameFramework/Pawn.h"
//#include "UfoPawn.generated.h"
//
//// Forward declarations
//class UStaticMeshComponent;
//class USpringArmComponent;
//class UCameraComponent;
//class UPhysicsHandleComponent;
//
//UCLASS()
//class TOTHEMOOOON_API AUfoPawn : public APawn
//{
//    GENERATED_BODY()
//
//public:
//    AUfoPawn();
//
//protected:
//    virtual void BeginPlay() override;
//
//public:
//    virtual void Tick(float DeltaTime) override;
//    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
//
//    // --- Components ---
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    UStaticMeshComponent* ShipMesh;
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    USpringArmComponent* SpringArm;
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    UCameraComponent* Camera;
//
//    // rolling the ship
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    USceneComponent* LeftThruster;
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    USceneComponent* RightThruster;
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    UPhysicsHandleComponent* PhysicsHandle;
//
//    // --- Movement Properties ---
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
//    float MainThrustForce = 100000.0f; // Renamed for clarity
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
//    float RollForce = 50000.0f; // A new force for rolling left/right
//
//    //    // We are now using RollTorque instead of RollForce
//    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
//    //float RollTorque = 10000.0f;
//
//
//    // --- Tractor Beam Properties ---
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tractor Beam")
//    float TractorBeamRange = 1000.0f;
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tractor Beam")
//    float TractorBeamRadius = 150.0f;
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
//    float LevelingTurnSpeed = 5.0f;
//
//private:
//    // --- Input Functions ---
//    void MoveHorizontal(float Value);
//    void ThrustUp(float Value);
//    void StartTractorBeam();
//    void StopTractorBeam();
//
//    // --- State & Helper Functions ---
//    void HandleTractorBeam(float DeltaTime);
//
//    bool bIsTractorBeamActive = false;
//};











//// UfoPawn.h
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "GameFramework/Pawn.h"
//#include "UfoPawn.generated.h"
//
//// Forward declarations
//class UStaticMeshComponent;
//class USpringArmComponent;
//class UCameraComponent;
//class UPhysicsHandleComponent;
//
//UCLASS()
//class TOTHEMOOOON_API AUfoPawn : public APawn
//{
//    GENERATED_BODY()
//
//public:
//    AUfoPawn();
//
//protected:
//    virtual void BeginPlay() override;
//
//public:
//    virtual void Tick(float DeltaTime) override;
//    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
//
//    // --- Components ---
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    UStaticMeshComponent* ShipMesh;
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    USpringArmComponent* SpringArm;
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    UCameraComponent* Camera;
//
//    // These components act as locations for applying thrust
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    USceneComponent* LeftThruster;
//
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    USceneComponent* RightThruster;
//
//    // This component will grab and hold other physics objects
//    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
//    UPhysicsHandleComponent* PhysicsHandle;
//
//    // --- Movement Properties ---
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
//    float ThrustForce = 100000.0f; // Force for forward movement
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
//    float TurnForce = 100000.0f; // Force used for turning
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
//    float LevelingTurnSpeed = 5.0f; // How fast the ship rights itself
//
//    // --- Tractor Beam Properties ---
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tractor Beam")
//    float TractorBeamRange = 1000.0f;
//
//    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tractor Beam")
//    float TractorBeamRadius = 250.0f; // The radius of the cone's end
//
//private:
//    // --- Input Functions ---
//    void MoveHorizontal(float Value);
//    void ThrustUp(float Value);
//    void StartTractorBeam();
//    void StopTractorBeam();
//
//    // --- State & Helper Functions ---
//    void HandleTractorBeam(float DeltaTime);
//
//    bool bIsTractorBeamActive = false;
//};