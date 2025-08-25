// UfoPawn.cpp

#include "UfoPawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h" // For gravity gun
#include "Kismet/KismetSystemLibrary.h" // Sphere trace
#include "Kismet/GameplayStatics.h" // For OpenLevel

AUfoPawn::AUfoPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	SetRootComponent(ShipMesh);
	ShipMesh->SetSimulatePhysics(true);
	ShipMesh->SetEnableGravity(true);
	ShipMesh->SetNotifyRigidBodyCollision(true);


	LeftThruster = CreateDefaultSubobject<USceneComponent>(TEXT("LeftThruster"));
	LeftThruster->SetupAttachment(ShipMesh);

	RightThruster = CreateDefaultSubobject<USceneComponent>(TEXT("RightThruster"));
	RightThruster->SetupAttachment(ShipMesh);

	FBodyInstance* BodyInstance = ShipMesh->GetBodyInstance();
	BodyInstance->bLockYTranslation = true;
	BodyInstance->bLockYRotation = true;
	// Set to true to stop from moving forward/back
	BodyInstance->bLockXRotation = true; // Set to false to allow the ship to roll

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 1500.0f;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bDoCollisionTest = false;
	SpringArm->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
	Camera->SetOrthoWidth(400.0f);

	// Create the Physics Handle Component for gravity gun
	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
	// ADDDED to make the handle connection rigid
	PhysicsHandle->LinearDamping = 200.0f;
	PhysicsHandle->LinearStiffness = 7500.0f;
	PhysicsHandle->AngularDamping = 500.0f;
	PhysicsHandle->AngularStiffness = 1500.0f;

	CurrentHealth = MaxHealth;
	ShipMesh->OnComponentHit.AddDynamic(this, &AUfoPawn::OnHit);

}

void AUfoPawn::BeginPlay()
{
	Super::BeginPlay();
	LockedXPosition = GetActorLocation().X; // Store the initial X position to lock it

	// Create and display the HUD widget
	if (HudWidgetClass)
	{
		UUserWidget* HudWidget = CreateWidget<UUserWidget>(GetWorld(), HudWidgetClass);
		if (HudWidget)
		{
			HudWidget->AddToViewport();
		}
	}

	// Configure input mode and cursor visibility
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		// Set input mode to Game and UI to allow interaction with both the game and UI elements.
		FInputModeGameAndUI InputMode;
		PlayerController->SetInputMode(InputMode);

		// Hide the mouse cursor for a cleaner look
		PlayerController->bShowMouseCursor = false;
	}
}

void AUfoPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Manually lock the X position every frame to prevent drift
	FVector CurrentLocation = GetActorLocation();
	if (CurrentLocation.X != LockedXPosition)
	{
		CurrentLocation.X = LockedXPosition;
		SetActorLocation(CurrentLocation);
	}
	
	if (bIsHoverActive)
	{
		HandleHovering(DeltaTime);
	}
	else
	{
		ApplyThrusterForces();
	}

	// Handle the gravity gun logic
	//if (bIsGravityGunActive)				-- OLD: Multi-grab version
	if (PhysicsHandle->GetGrabbedComponent())
	{
		HandleGravityGun(DeltaTime);
	}
}

void AUfoPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Action-based thruster bindings
	PlayerInputComponent->BindAction("ThrustLeft", IE_Pressed, this, &AUfoPawn::ThrustLeftPressed);
	PlayerInputComponent->BindAction("ThrustLeft", IE_Released, this, &AUfoPawn::ThrustLeftReleased);
	PlayerInputComponent->BindAction("ThrustRight", IE_Pressed, this, &AUfoPawn::ThrustRightPressed);
	PlayerInputComponent->BindAction("ThrustRight", IE_Released, this, &AUfoPawn::ThrustRightReleased);

	// Hover binding
	PlayerInputComponent->BindAction("Hover", IE_Pressed, this, &AUfoPawn::HoverPressed);
	PlayerInputComponent->BindAction("Hover", IE_Released, this, &AUfoPawn::HoverReleased);

	// ADDED: Axis-based movement bindings
	PlayerInputComponent->BindAxis("MoveHorizontal", this, &AUfoPawn::MoveHorizontal);
	PlayerInputComponent->BindAxis("ThrustUp", this, &AUfoPawn::ThrustUp);

	// Gravity Gun bindings
	PlayerInputComponent->BindAction("GravityGun", IE_Pressed, this, &AUfoPawn::StartGravityGun);
	PlayerInputComponent->BindAction("GravityGun", IE_Released, this, &AUfoPawn::StopGravityGun);
	PlayerInputComponent->BindAction("RotateObject", IE_Pressed, this, &AUfoPawn::RotateGrabbedObject);
	PlayerInputComponent->BindAction("ShrinkObject", IE_Pressed, this, &AUfoPawn::ShrinkGrabbedObject);

}

// --- Action-based Thruster Input Functions ---
void AUfoPawn::ThrustLeftPressed()
{
	bIsLeftThrusterActive = true;
}

void AUfoPawn::ThrustLeftReleased()
{
	bIsLeftThrusterActive = false;
}

void AUfoPawn::ThrustRightPressed()
{
	bIsRightThrusterActive = true;
}

void AUfoPawn::ThrustRightReleased()
{
	bIsRightThrusterActive = false;
}

// --- Hover Input Functions ---
void AUfoPawn::HoverPressed()
{
	bIsHoverActive = true;
}

void AUfoPawn::HoverReleased()
{
	bIsHoverActive = false;
}

// --- Action-based Thruster Logic ---
void AUfoPawn::ApplyThrusterForces()
{
	const FVector ForceDirection = GetActorUpVector() * MainThrustForce;

	if (bIsLeftThrusterActive)
	{
		ShipMesh->AddForceAtLocation(ForceDirection, LeftThruster->GetComponentLocation());
	}

	if (bIsRightThrusterActive)
	{
		ShipMesh->AddForceAtLocation(ForceDirection, RightThruster->GetComponentLocation());
	}
}

// --- Axis-based Movement Functions ---
void AUfoPawn::MoveHorizontal(float Value)
{
	//// Prevent rolling if hover is active
	//if (bIsHoverActive)
	//{
	//	return;
	//}

	if (FMath::Abs(Value) > 0.1f)
	{
		// Use FMath::Abs(Value) to ensure the force is always upward relative to the world
		// The direction of the roll is handled by which thruster force is applied to
		const FVector ForceDirection = FVector::UpVector * FMath::Abs(Value) * RollForce;

		if (Value > 0) // E to roll right
		{
			// Apply upward force on the left thruster to cause a roll right
			ShipMesh->AddForceAtLocation(ForceDirection, LeftThruster->GetComponentLocation());
		}
		else // Q to roll left
		{
			// Apply upward force on the right thruster to roll left
			ShipMesh->AddForceAtLocation(ForceDirection, RightThruster->GetComponentLocation());
		}
	}
}
				// ======== OLD: Direct left/right movement method =========
				//void AUfoPawn::MoveHorizontal(float Value)
				//{
				//	// Prevent moving if hover is active
				//	if (bIsHoverActive)
				//	{
				//		return;
				//	}
				//
				//	if (FMath::Abs(Value) > 0.1f)
				//	{
				//		// Apply force along the world's Y-axis for direct left/right movement
				//		const FVector ForceDirection = FVector::RightVector * Value * HorizontalForce;
				//		ShipMesh->AddForce(ForceDirection);
				//	}
				//}

void AUfoPawn::ThrustUp(float Value)
{
	//// Prevent thrusting if hover is active
	//if (bIsHoverActive)
	//{
	//	return;
	//}
	if (FMath::Abs(Value) > 0.1f)
	{
		// Apply force along the SHIP'S up-vector.
		// When rolled, this will push the ship sideways.
		const FVector ForceDirection = GetActorUpVector() * Value * UpThrustForce;
		ShipMesh->AddForce(ForceDirection);
	}
}


// --- Hovering Implementation ---
void AUfoPawn::HandleHovering(float DeltaTime)
{
	// 1. Hover: Counteract any existing vertical velocity to stop vertical movement
	const float CurrentZVelocity = ShipMesh->GetPhysicsLinearVelocity().Z;
	const float HoverForce = -CurrentZVelocity / DeltaTime; // Damping force
	ShipMesh->AddForce(FVector(0, 0, HoverForce), NAME_None, true); // Use Accel Change for mass-independent force

	// 2. Level Out: Smoothly rotate the ship to be flat
	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation = FRotator(0, CurrentRotation.Yaw, 0); // Keep yaw, but zero pitch and roll
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, LevelingTurnSpeed);
	ShipMesh->SetWorldRotation(NewRotation);
}

// --- Health System Implementation ---
void AUfoPawn::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

	// Check whether collision is with a valid actor/component and not the grabbed object
	if (OtherActor && OtherActor != this && OtherComp != GrabbedComponent && !OtherActor->ActorHasTag(TEXT("NoDamage")))
	{
		UE_LOG(LogTemp, Warning, TEXT("OnHit fired! Collided with: %s"), *OtherActor->GetName());

		// Apply 10 points of damage
		HandleDamage(1.0f);
	}
}

void AUfoPawn::HandleDamage(float DamageAmount)
{
	// Subtract damage from current health, ensuring it doesn't go below zero.
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

	// Log the damage and current health to the output log for debugging.
	UE_LOG(LogTemp, Warning, TEXT("UFO took %f damage. Current health: %f"), DamageAmount, CurrentHealth);

	// Check if the pawn should be destroyed.
	if (CurrentHealth <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("UFO has been destroyed! GAME OVER."));
		// Check if the map name has been set in the editor
		if (!GameOverMapName.IsNone())
		{
			UGameplayStatics::OpenLevel(this, GameOverMapName);
			UE_LOG(LogTemp, Error, TEXT("UFO has been destroyed! Loading GameOverMapName."), GameOverMapName);

		}

		Destroy();
	}
}




//  -------------------- OLD PHYSICS HANDLE SINGLE GRAB Gravity Gun Logic ----------------
 // --- Gravity Gun Implementation ---

void AUfoPawn::StartGravityGun()
{
	// Log when we try to fire the gun
	UE_LOG(LogTemp, Warning, TEXT("Attempting to fire Gravity Gun..."));

	if (GrabbedComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("...Failed: Already holding an object."));
		return;
	}

	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0, 0, GravityGunRange);
	TArray<FHitResult> OutHits;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		Start,
		End,
		GravityGunRadius,
		{ UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_PhysicsBody) },
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		OutHits,
		true
	);

	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("...Success: Trace HIT something!"));
		for (const FHitResult& Hit : OutHits)
		{
			UPrimitiveComponent* HitComponent = Hit.GetComponent();
			if (HitComponent && HitComponent->IsSimulatingPhysics())
			{
				UE_LOG(LogTemp, Warning, TEXT("...Success: Found a physics object to grab: %s"), *HitComponent->GetName());

				GrabbedComponent = HitComponent;
				GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
				//PhysicsHandle->GrabComponent(GrabbedComponent, NAME_None, Hit.ImpactPoint, true);
				// ADD THIS: Reset rotation when grabbing a new object
				GrabbedObjectRotation = HitComponent->GetComponentRotation();

				// CHANGE THIS FUNCTION: This version allows us to control rotation
				PhysicsHandle->GrabComponentAtLocationWithRotation(GrabbedComponent, NAME_None, Hit.ImpactPoint, GrabbedObjectRotation);

				// Check if the grab was successful
				if (PhysicsHandle->GetGrabbedComponent())
				{
					UE_LOG(LogTemp, Warning, TEXT("...Success: PhysicsHandle GRABBED the component!"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("...Failed: PhysicsHandle FAILED to grab the component."));
				}

				return;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("...Failed: Trace MISSED everything."));
	}
}

void AUfoPawn::StopGravityGun()
{
	if (GrabbedComponent)
	{
		// Re-enable collision with the ship
		GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);

		// Release it from the physics handle
		PhysicsHandle->ReleaseComponent();

		// Clear our pointer
		GrabbedComponent = nullptr;
	}
}

void AUfoPawn::HandleGravityGun(float DeltaTime)
{
	if (PhysicsHandle->GetGrabbedComponent())
	{
		FVector TargetLocation = GetActorLocation() - FVector(0, 0, GravityGunHoldDistance);
		PhysicsHandle->SetTargetLocation(TargetLocation);

		// ADDED to set the target rotation every frame
		PhysicsHandle->SetTargetRotation(GrabbedObjectRotation);

		// The rotation is already locked by GrabComponentWithRotation, so no need to set it every frame
		// Force the object to maintain the saved rotation
		//PhysicsHandle->SetTargetRotation(GrabbedObjectRotation);
	}
}

void AUfoPawn::RotateGrabbedObject()
{
	// We only want to do this if we are holding an object
	if (PhysicsHandle->GetGrabbedComponent())
	{
		// Add 90 degrees to the Yaw (left-right rotation)
		GrabbedObjectRotation.Roll += 90.0f;
	}
}

void AUfoPawn::ShrinkGrabbedObject()
{
	if (GrabbedComponent)
	{
		// Get the Actor that the component belongs to
		AActor* GrabbedActor = GrabbedComponent->GetOwner();
		if (GrabbedActor)
		{
			// Get the current scale of the actor
			FVector CurrentScale = GrabbedActor->GetActorScale3D();
			// Calculate the new scale
			FVector NewScale = CurrentScale * ShrinkFactor;

			// Check that the new scale is above the minimum threshold
			if (NewScale.X >= MinScale && NewScale.Y >= MinScale && NewScale.Z >= MinScale)
			{
				// Apply the new scale to the actor
				GrabbedActor->SetActorScale3D(NewScale);
				UE_LOG(LogTemp, Warning, TEXT("Shrinking object %s to scale %s"), *GrabbedActor->GetName(), *NewScale.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Object %s has reached its minimum scale."), *GrabbedActor->GetName());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Shrink button pressed, but no object is grabbed."));
	}
}


//// --- Gravity Gun Implementation (Multi-Grab Version) ---
//
//void AUfoPawn::StartGravityGun()
//{
//	bIsGravityGunActive = true;
//
//	// Clear any previously grabbed objects
//	GrabbedComponents.Empty();
//
//	FVector Start = GetActorLocation();
//	FVector End = Start - FVector(0, 0, GravityGunRange);
//	TArray<FHitResult> OutHits;
//	TArray<AActor*> ActorsToIgnore;
//	ActorsToIgnore.Add(this);
//
//	// CORRECTED: This now correctly looks for the PhysicsBody object type, which will match your cube's settings.
//	// This also uses the original SphereTraceMultiForObjects function to avoid the red underline error.
//	bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
//		GetWorld(),
//		Start,
//		End,
//		GravityGunRadius,
//		{ UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_PhysicsBody) }, // This is the fix!
//		false,
//		ActorsToIgnore,
//		EDrawDebugTrace::ForDuration,
//		OutHits,
//		true
//	);
//
//	if (bHit)
//	{
//		for (const FHitResult& Hit : OutHits)
//		{
//			UPrimitiveComponent* HitComponent = Hit.GetComponent();
//			if (HitComponent && HitComponent->IsSimulatingPhysics())
//			{
//				// Add every valid physics object to our array
//				GrabbedComponents.Add(HitComponent);
//			}
//		}
//	}
//}
//
//void AUfoPawn::StopGravityGun()
//{
//	bIsGravityGunActive = false;
//	// Clear the array of grabbed objects
//	GrabbedComponents.Empty();
//}
//
//void AUfoPawn::HandleGravityGun(float DeltaTime)
//{
//	// Check if we have any components in our array
//	if (GrabbedComponents.Num() > 0)
//	{
//		FVector TargetLocation = GetActorLocation() - FVector(0, 0, GravityGunRange * 0.5f);
//
//		// Loop through all the components we've grabbed
//		for (UPrimitiveComponent* GrabbedComp : GrabbedComponents)
//		{
//			if (GrabbedComp)
//			{
//				// Calculate the direction from the object to the target location
//				FVector PullDirection = (TargetLocation - GrabbedComp->GetComponentLocation()).GetSafeNormal();
//				// Apply a force to the object, ignoring its mass
//				// The 'true' at the end makes this an acceleration change, which is mass-independent.
//				GrabbedComp->AddForce(PullDirection * GravityGunPullForce, NAME_None, true);
//			}
//		}
//	}
//}



//// ========================= NO ROLL FORCE METHOD ===========================
//// ========================= SIMPLIFY HOVER LOGIC ===========================
//// UfoPawn.cpp
//
//#include "UfoPawn.h"
//#include "Components/StaticMeshComponent.h"
//#include "GameFramework/SpringArmComponent.h"
//#include "Camera/CameraComponent.h"
//// REMOVED: No longer need PhysicsHandle or KismetSystemLibrary for the hover
//// #include "PhysicsEngine/PhysicsHandleComponent.h"
//// #include "Kismet/KismetSystemLibrary.h"
//
//AUfoPawn::AUfoPawn()
//{
//	PrimaryActorTick.bCanEverTick = true;
//
//	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
//	SetRootComponent(ShipMesh);
//	ShipMesh->SetSimulatePhysics(true);
//	ShipMesh->SetEnableGravity(true);
//
//	LeftThruster = CreateDefaultSubobject<USceneComponent>(TEXT("LeftThruster"));
//	LeftThruster->SetupAttachment(ShipMesh);
//
//	RightThruster = CreateDefaultSubobject<USceneComponent>(TEXT("RightThruster"));
//	RightThruster->SetupAttachment(ShipMesh);
//
//	FBodyInstance* BodyInstance = ShipMesh->GetBodyInstance();
//	BodyInstance->bLockYTranslation = true;
//	BodyInstance->bLockYRotation = true;
//	BodyInstance->bLockXRotation = true;
//
//	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
//	SpringArm->SetupAttachment(RootComponent);
//	SpringArm->TargetArmLength = 1500.0f;
//	SpringArm->bEnableCameraLag = false;
//	SpringArm->bDoCollisionTest = false;
//	SpringArm->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
//
//	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
//	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
//	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
//	Camera->SetOrthoWidth(400.0f);
//
//	// REMOVED: PhysicsHandle is no longer created.
//	// PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
//}
//
//void AUfoPawn::BeginPlay()
//{
//	Super::BeginPlay();
//}
//
//void AUfoPawn::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//	// UPDATED: Check if hover is active, otherwise apply thruster forces.
//	if (bIsHoverActive)
//	{
//		HandleHovering(DeltaTime);
//	}
//	else
//	{
//		ApplyThrusterForces();
//	}
//}
//
//void AUfoPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//	// Thruster actions (no change)
//	PlayerInputComponent->BindAction("ThrustLeft", IE_Pressed, this, &AUfoPawn::ThrustLeftPressed);
//	PlayerInputComponent->BindAction("ThrustLeft", IE_Released, this, &AUfoPawn::ThrustLeftReleased);
//	PlayerInputComponent->BindAction("ThrustRight", IE_Pressed, this, &AUfoPawn::ThrustRightPressed);
//	PlayerInputComponent->BindAction("ThrustRight", IE_Released, this, &AUfoPawn::ThrustRightReleased);
//
//	// UPDATED: Bind a new "Hover" action instead of "TractorBeam"
//	PlayerInputComponent->BindAction("Hover", IE_Pressed, this, &AUfoPawn::HoverPressed);
//	PlayerInputComponent->BindAction("Hover", IE_Released, this, &AUfoPawn::HoverReleased);
//}
//
//// --- Thruster Input Functions (no change) ---
//void AUfoPawn::ThrustLeftPressed()
//{
//	bIsLeftThrusterActive = true;
//}
//
//void AUfoPawn::ThrustLeftReleased()
//{
//	bIsLeftThrusterActive = false;
//}
//
//void AUfoPawn::ThrustRightPressed()
//{
//	bIsRightThrusterActive = true;
//}
//
//void AUfoPawn::ThrustRightReleased()
//{
//	bIsRightThrusterActive = false;
//}
//
//// --- NEW: Hover Input Functions ---
//void AUfoPawn::HoverPressed()
//{
//	bIsHoverActive = true;
//}
//
//void AUfoPawn::HoverReleased()
//{
//	bIsHoverActive = false;
//}
//
//
//// --- Thruster Logic (no change) ---
//void AUfoPawn::ApplyThrusterForces()
//{
//	const FVector ForceDirection = GetActorUpVector() * MainThrustForce;
//
//	if (bIsLeftThrusterActive)
//	{
//		ShipMesh->AddForceAtLocation(ForceDirection, LeftThruster->GetComponentLocation());
//	}
//
//	if (bIsRightThrusterActive)
//	{
//		ShipMesh->AddForceAtLocation(ForceDirection, RightThruster->GetComponentLocation());
//	}
//}
//
//
//// --- NEW: Hovering Implementation ---
//void AUfoPawn::HandleHovering(float DeltaTime)
//{
//	// 1. Hover: Counteract any existing vertical velocity to stop vertical movement.
//	// This does NOT affect horizontal momentum.
//	const float CurrentZVelocity = ShipMesh->GetPhysicsLinearVelocity().Z;
//	const float HoverForce = -CurrentZVelocity / DeltaTime; // Damping force
//	ShipMesh->AddForce(FVector(0, 0, HoverForce), NAME_None, true); // Use Accel Change for mass-independent force
//
//	// 2. Level Out: Smoothly rotate the ship to be flat
//	const FRotator CurrentRotation = GetActorRotation();
//	const FRotator TargetRotation = FRotator(0, CurrentRotation.Yaw, 0); // Keep yaw, but zero pitch and roll
//	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, LevelingTurnSpeed);
//	ShipMesh->SetWorldRotation(NewRotation);
//}


//====================== ROLL FORCE METHOD =========================
// // UfoPawn.cpp
//
//#include "UfoPawn.h"
//#include "Components/StaticMeshComponent.h"
//#include "GameFramework/SpringArmComponent.h"
//#include "Camera/CameraComponent.h"
//#include "PhysicsEngine/PhysicsHandleComponent.h"
//#include "Kismet/KismetSystemLibrary.h" // Needed for Sphere Trace
//
//// --- CONSTRUCTOR: Create and attach all components ---
//AUfoPawn::AUfoPawn()
//{
//	PrimaryActorTick.bCanEverTick = true;
//
//	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
//	SetRootComponent(ShipMesh);
//	ShipMesh->SetSimulatePhysics(true);
//	ShipMesh->SetEnableGravity(true);
//
//	// --- Add these back ---
//	LeftThruster = CreateDefaultSubobject<USceneComponent>(TEXT("LeftThruster"));
//	LeftThruster->SetupAttachment(ShipMesh);
//
//	RightThruster = CreateDefaultSubobject<USceneComponent>(TEXT("RightThruster"));
//	RightThruster->SetupAttachment(ShipMesh);
//	// --------------------
//
//	FBodyInstance* BodyInstance = ShipMesh->GetBodyInstance();
//	BodyInstance->bLockYTranslation = true;
//	BodyInstance->bLockYRotation = true;
//	// --- UNLOCK ROLL ROTATION ---
//	BodyInstance->bLockXRotation = true; // Set to false to allow the ship to roll
//
//	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
//	SpringArm->SetupAttachment(RootComponent);
//	SpringArm->TargetArmLength = 1500.0f;
//	SpringArm->bEnableCameraLag = false;
//	SpringArm->bDoCollisionTest = false;
//	SpringArm->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
//
//	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
//	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
//	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
//	Camera->SetOrthoWidth(400.0f);
//
//	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
//}
//
//void AUfoPawn::BeginPlay()
//{
//	Super::BeginPlay();
//}
//
//// --- TICK: Called every frame ---
//void AUfoPawn::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//	// If the tractor beam is active, run its special hover logic.
//	if (bIsTractorBeamActive)
//	{
//		HandleTractorBeam(DeltaTime);
//	}
//
//}
//
//// --- INPUT BINDING ---
//void AUfoPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//	// Bind new 2D movement axes
//	PlayerInputComponent->BindAxis("MoveHorizontal", this, &AUfoPawn::MoveHorizontal);
//	PlayerInputComponent->BindAxis("ThrustUp", this, &AUfoPawn::ThrustUp);
//
//	// Bind tractor beam actions
//	PlayerInputComponent->BindAction("TractorBeam", IE_Pressed, this, &AUfoPawn::StartTractorBeam);
//	PlayerInputComponent->BindAction("TractorBeam", IE_Released, this, &AUfoPawn::StopTractorBeam);
//}
//
//void AUfoPawn::MoveHorizontal(float Value)
//{
//	// --- FIX: PREVENT ROLLING IF TRACTOR BEAM IS ACTIVE ---
//	// If the tractor beam is on, its auto-leveling will fight the roll.
//	// So, we just exit the function and don't apply any roll force.
//	if (bIsTractorBeamActive)
//	{
//		return;
//	}
//
//	if (FMath::Abs(Value) > 0.1f)
//	{
//		// Use FMath::Abs(Value) to ensure the force is always upward.
//		// The direction of the roll is handled by which thruster we apply the force to.
//		const FVector ForceDirection = FVector::UpVector * FMath::Abs(Value) * RollForce;
//
//		if (Value > 0) // Pressing 'D' to roll right
//		{
//			// Apply upward force on the left thruster to cause a roll to the right
//			ShipMesh->AddForceAtLocation(ForceDirection, LeftThruster->GetComponentLocation());
//		}
//		else // Pressing 'A' to roll left
//		{
//			// Apply upward force on the right thruster to cause a roll to the left
//			ShipMesh->AddForceAtLocation(ForceDirection, RightThruster->GetComponentLocation());
//		}
//	}
//}
//
////// --- COMPLETELY REVISED MoveHorizontal FUNCTION ---
////void AUfoPawn::MoveHorizontal(float Value)
////{
////	// Prevent rolling if the tractor beam is active
////	if (bIsTractorBeamActive)
////	{
////		return;
////	}
////
////	if (FMath::Abs(Value) > 0.1f)
////	{
////		// Create a torque vector. For a side-scroller, rolling happens around the X-axis.
////		// The 'Value' from the input (A/D keys) determines the direction (positive or negative).
////		const FVector TorqueToApply = FVector(Value * RollTorque, 0.0f, 0.0f);
////
////		// Add the torque to the ship's mesh.
////		// The 'true' at the end makes it an acceleration change, ignoring the ship's mass,
////		// which makes it feel more responsive.
////		ShipMesh->AddTorqueInDegrees(TorqueToApply, NAME_None, true);
////	}
////}
//
//void AUfoPawn::ThrustUp(float Value)
//{
//	// Don't allow vertical thrust if the tractor beam is active and hovering
//	if (bIsTractorBeamActive)
//	{
//		return;
//	}
//	if (FMath::Abs(Value) > 0.1f)
//	{
//		// Apply force along the SHIP'S up-vector.
//		// When rolled, this will push the ship sideways.
//		const FVector ForceDirection = GetActorUpVector() * Value * MainThrustForce;
//		ShipMesh->AddForce(ForceDirection);
//	}
//}
//
//
//// --- TRACTOR BEAM IMPLEMENTATION ---
//
//void AUfoPawn::StartTractorBeam()
//{
//	bIsTractorBeamActive = true;
//
//	// Define the cone trace for objects
//	FVector Start = GetActorLocation();
//	FVector End = Start - FVector(0, 0, TractorBeamRange);
//	TArray<FHitResult> OutHits;
//	TArray<AActor*> ActorsToIgnore;
//	ActorsToIgnore.Add(this);
//
//	// Perform the sphere trace to find physics objects
//	bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
//		GetWorld(),
//		Start,
//		End,
//		TractorBeamRadius, // Use radius for sphere trace
//		{ EObjectTypeQuery::ObjectTypeQuery3 }, // Look for PhysicsBody objects
//		false,
//		ActorsToIgnore,
//		EDrawDebugTrace::ForDuration,
//		OutHits,
//		true
//	);
//
//
//	if (bHit)
//	{
//		for (const FHitResult& Hit : OutHits)
//		{
//			UPrimitiveComponent* HitComponent = Hit.GetComponent();
//			if (HitComponent && HitComponent->IsSimulatingPhysics())
//			{
//				// Grab the first physics object we find
//				PhysicsHandle->GrabComponentAtLocation(HitComponent, NAME_None, Hit.ImpactPoint);
//				return; // Exit after grabbing one object
//			}
//		}
//	}
//}
//
//void AUfoPawn::StopTractorBeam()
//{
//	bIsTractorBeamActive = false;
//	PhysicsHandle->ReleaseComponent();
//}
//
//void AUfoPawn::HandleTractorBeam(float DeltaTime)
//{
//	// --- Full Stop Hover and Leveling Logic ---
//	FVector CurrentVelocity = ShipMesh->GetPhysicsLinearVelocity();
//
//	// Calculate the required acceleration to counteract velocity (damping) and gravity.
//	// This is applied to all axes to bring the ship to a full stop.
//	FVector DampingAccel = -CurrentVelocity / DeltaTime;
//	FVector GravityCounterAccel = FVector(0, 0, -GetWorld()->GetGravityZ());
//
//	// Apply the combined counter-forces as an acceleration, which is mass-independent.
//	ShipMesh->AddForce(DampingAccel + GravityCounterAccel, NAME_None, true);
//
//	// --- Level Out Logic (remains the same) ---
//	FRotator CurrentRotation = GetActorRotation();
//	FRotator TargetRotation = FRotator(0, CurrentRotation.Yaw, 0); // Keep yaw, but zero pitch and roll
//	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, LevelingTurnSpeed);
//	ShipMesh->SetWorldRotation(NewRotation);
//
//	// --- Update Grabbed Object Logic (remains the same) ---
//	if (PhysicsHandle->GetGrabbedComponent())
//	{
//		FVector TargetLocation = GetActorLocation() - FVector(0, 0, TractorBeamRange * 0.5f);
//		PhysicsHandle->SetTargetLocation(TargetLocation);
//	}
//}





















//// UfoPawn.cpp
//
//#include "UfoPawn.h"
//#include "Components/StaticMeshComponent.h"
//#include "GameFramework/SpringArmComponent.h"
//#include "Camera/CameraComponent.h"
//#include "PhysicsEngine/PhysicsHandleComponent.h"
//#include "Kismet/KismetSystemLibrary.h" // Needed for Cone Trace
//
//// --- CONSTRUCTOR: Create and attach all components ---
//AUfoPawn::AUfoPawn()
//{
//    PrimaryActorTick.bCanEverTick = true;
//
//    ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
//    SetRootComponent(ShipMesh);
//    ShipMesh->SetSimulatePhysics(true);
//    ShipMesh->SetEnableGravity(true);
//
//    // --- Add these back ---
//    LeftThruster = CreateDefaultSubobject<USceneComponent>(TEXT("LeftThruster"));
//    LeftThruster->SetupAttachment(ShipMesh);
//
//    RightThruster = CreateDefaultSubobject<USceneComponent>(TEXT("RightThruster"));
//    RightThruster->SetupAttachment(ShipMesh);
//    // --------------------
//
//    FBodyInstance* BodyInstance = ShipMesh->GetBodyInstance();
//    BodyInstance->bLockYTranslation = true;
//    BodyInstance->bLockYRotation = true;
//    // --- UNLOCK ROLL ROTATION ---
//    BodyInstance->bLockXRotation = false; // Set to false to allow the ship to roll
//
//    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
//    SpringArm->SetupAttachment(RootComponent);
//    SpringArm->TargetArmLength = 1500.0f;
//    SpringArm->bEnableCameraLag = false;
//    SpringArm->bDoCollisionTest = false;
//    SpringArm->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
//
//    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
//    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
//    Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
//    Camera->SetOrthoWidth(400.0f);
//
//    PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
//
//    /* working code before new thruster fucntionality */
////    PrimaryActorTick.bCanEverTick = true;
////
////    // Create the ship mesh, make it the root, and enable physics
////    ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
////    SetRootComponent(ShipMesh);
////    ShipMesh->SetSimulatePhysics(true);
////    ShipMesh->SetEnableGravity(true);
////    // NEW SIDE-VIEW CONSTRAINTS
////    FBodyInstance* BodyInstance = ShipMesh->GetBodyInstance();
////    // Lock movement on the Y-axis (prevents moving toward/away from the camera)
////    BodyInstance->bLockYTranslation = true;
////    // Lock rotation on the X and Y axes to keep the ship oriented correctly
////    BodyInstance->bLockXRotation = true;
////    BodyInstance->bLockYRotation = true;
////
////    /*
////    // Important for 2.5D: Constrain movement to the X-Y plane for rotation
////    ShipMesh->GetBodyInstance()->bLockZRotation = true;
////    ShipMesh->GetBodyInstance()->bLockYRotation = true;
////    */
////
////    // Create thruster points and attach them to the mesh
////    LeftThruster = CreateDefaultSubobject<USceneComponent>(TEXT("LeftThruster"));
////    LeftThruster->SetupAttachment(ShipMesh);
////
////    RightThruster = CreateDefaultSubobject<USceneComponent>(TEXT("RightThruster"));
////    RightThruster->SetupAttachment(ShipMesh);
////
////    // ---new 2D camera mode---
////// Create the camera spring arm. For a 2D game, can disable lag for a tighter feel.
////    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
////    SpringArm->SetupAttachment(RootComponent);
////    SpringArm->TargetArmLength = 1500.0f; // How far away the camera is
////    SpringArm->bEnableCameraLag = true; // Optional: A true 2D feel often has no camera lag
////    SpringArm->bDoCollisionTest = false; // Don't try to move camera around obstacles
////	SpringArm->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f)); //Side view camera angle
////
////    // Create and attach the camera
////    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
////    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
////
////	// Set the camera to Orthographic for 2D projection or Perspective for 2.5D
////    Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
////    // Set the size of the viewing area. Adjust this value to zoom in or out.
////    Camera->SetOrthoWidth(400.0f);
////	
////    /* ---old 2.5D 3D camera code---
////    // Create the camera spring arm (for smooth camera movement)
////    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
////    SpringArm->SetupAttachment(RootComponent);
////    SpringArm->TargetArmLength = 1500.0f;
////    SpringArm->bEnableCameraLag = true;
////    SpringArm->bDoCollisionTest = false; // Don't try to move camera around obstacles
////    SpringArm->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f)); // Angled top-down view
////
////    // Create and attach the camera
////    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
////    Camera->SetupAttachment(SpringArm);
////    */
////
////    // Create the physics handle
////    PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
//}
//
//void AUfoPawn::BeginPlay()
//{
//    Super::BeginPlay();
//}
//
//// --- TICK: Called every frame ---
//void AUfoPawn::Tick(float DeltaTime)
//{
//    Super::Tick(DeltaTime);
//
//    // If the tractor beam is active, run its special hover logic.
//    if (bIsTractorBeamActive)
//    {
//        HandleTractorBeam(DeltaTime);
//    }
//
//}
//
//// --- INPUT BINDING ---
//void AUfoPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//    Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//    // Bind new 2D movement axes
//    PlayerInputComponent->BindAxis("MoveHorizontal", this, &AUfoPawn::MoveHorizontal);
//    PlayerInputComponent->BindAxis("ThrustUp", this, &AUfoPawn::ThrustUp);
//
//    // Bind tractor beam actions
//    PlayerInputComponent->BindAction("TractorBeam", IE_Pressed, this, &AUfoPawn::StartTractorBeam);
//    PlayerInputComponent->BindAction("TractorBeam", IE_Released, this, &AUfoPawn::StopTractorBeam);
//}
//
//void AUfoPawn::MoveHorizontal(float Value)
//{
//    if (FMath::Abs(Value) > 0.1f)
//    {
//        // THE FIX: Use FMath::Abs(Value) to ensure the force is always upward.
//        // The direction of the roll is handled by which thruster we apply the force to.
//        const FVector ForceDirection = FVector::UpVector * FMath::Abs(Value) * RollForce;
//
//        if (Value > 0) // Pressing 'D' to roll right
//        {
//            // Apply upward force on the left thruster
//            ShipMesh->AddForceAtLocation(ForceDirection, LeftThruster->GetComponentLocation());
//        }
//        else // Pressing 'A' to roll left
//        {
//            // Apply upward force on the right thruster
//            ShipMesh->AddForceAtLocation(ForceDirection, RightThruster->GetComponentLocation());
//        }
//    }
//    /* old working code no multi thruster functionality */
//    //if (FMath::Abs(Value) > 0.1f)
//    //{
//    //    // Apply force along the world's X-axis for left/right movement
//    //    const FVector ForceDirection = FVector::RightVector * Value * ThrustForce;
//    //    ShipMesh->AddForce(ForceDirection);
//    //}
//}
//
//void AUfoPawn::ThrustUp(float Value)
//{
//    // Don't allow vertical thrust if the tractor beam is active and hovering
//    if (bIsTractorBeamActive)
//    {
//        return;
//    }
//    if (FMath::Abs(Value) > 0.1f)
//    {
//        // Apply force along the SHIP'S up-vector.
//        // When rolled, this will push the ship sideways.
//        const FVector ForceDirection = GetActorUpVector() * Value * MainThrustForce;
//        ShipMesh->AddForce(ForceDirection);
//    }
//
//    /* old working code basic up thrust with no multi thrusters*/
//    //if (FMath::Abs(Value) > 0.1f)
//    //{
//    //    // Apply force along the world's Z-axis for up/down movement
//    //    const FVector ForceDirection = FVector::UpVector * Value * ThrustForce;
//    //    ShipMesh->AddForce(ForceDirection);
//    //}
//}
//
//
//// --- TRACTOR BEAM IMPLEMENTATION ---
//
//void AUfoPawn::StartTractorBeam()
//{
//    bIsTractorBeamActive = true;
//   /*not needed*/ //ShipMesh->SetEnableGravity(false); // Disable gravity while hovering
//
//    // Define the cone trace for objects
//    FVector Start = GetActorLocation();
//    FVector End = Start - FVector(0, 0, TractorBeamRange);
//    TArray<FHitResult> OutHits;
//    TArray<AActor*> ActorsToIgnore;
//    ActorsToIgnore.Add(this);
//
//    // Perform the cone trace to find physics objects
//    // Replace the ConeTraceMultiForObjects call with SphereTraceMultiForObjects, as ConeTraceMultiForObjects does not exist in UKismetSystemLibrary.
//
//    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
//        GetWorld(),
//        Start,
//        End,
//        TractorBeamRadius, // Use radius for sphere trace
//		{ EObjectTypeQuery::ObjectTypeQuery3 }, // Look for PhysicsBody objects
//        false,
//        ActorsToIgnore,
//        EDrawDebugTrace::ForDuration,
//        OutHits,
//        true
//    );
//
//
//    if (bHit)
//    {
//        for (const FHitResult& Hit : OutHits)
//        {
//            UPrimitiveComponent* HitComponent = Hit.GetComponent();
//            if (HitComponent && HitComponent->IsSimulatingPhysics())
//            {
//                // Grab the first physics object we find
//                PhysicsHandle->GrabComponentAtLocation(HitComponent, NAME_None, Hit.ImpactPoint);
//                return; // Exit after grabbing one object
//            }
//        }
//    }
//}
//
//void AUfoPawn::StopTractorBeam()
//{
//    bIsTractorBeamActive = false;
//    /*not needed*/ //ShipMesh->SetEnableGravity(true); // Re-enable gravity
//    PhysicsHandle->ReleaseComponent();
//}
//
//void AUfoPawn::HandleTractorBeam(float DeltaTime)
//{
//    // --- Full Stop Hover and Leveling Logic ---
//    FVector CurrentVelocity = ShipMesh->GetPhysicsLinearVelocity();
//
//    // Calculate the required acceleration to counteract velocity (damping) and gravity.
//    // This is applied to all axes to bring the ship to a full stop.
//    FVector DampingAccel = -CurrentVelocity / DeltaTime;
//    FVector GravityCounterAccel = FVector(0, 0, -GetWorld()->GetGravityZ());
//
//    // Apply the combined counter-forces as an acceleration, which is mass-independent.
//    ShipMesh->AddForce(DampingAccel + GravityCounterAccel, NAME_None, true);
//
//    // --- Level Out Logic (remains the same) ---
//    FRotator CurrentRotation = GetActorRotation();
//    FRotator TargetRotation = FRotator(0, CurrentRotation.Yaw, 0); // Keep yaw, but zero pitch and roll
//    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, LevelingTurnSpeed);
//    ShipMesh->SetWorldRotation(NewRotation);
//
//    // --- Update Grabbed Object Logic (remains the same) ---
//    if (PhysicsHandle->GetGrabbedComponent())
//    {
//        FVector TargetLocation = GetActorLocation() - FVector(0, 0, TractorBeamRange * 0.5f);
//        PhysicsHandle->SetTargetLocation(TargetLocation);
//    }
//
//
//    /* old tractor beam logic */
//    //// --- Hover and Leveling Logic ---
//    //// 1. Hover: Counteract any existing vertical velocity to hover in place
//    //float CurrentZVelocity = ShipMesh->GetPhysicsLinearVelocity().Z;
//    //float HoverForce = -CurrentZVelocity / DeltaTime; // Damping force
//    //ShipMesh->AddForce(FVector(0, 0, HoverForce), NAME_None, true); // Use Accel Change for mass-independent force
//
//    //// 2. Level Out: Smoothly rotate the ship to be flat
//    //FRotator CurrentRotation = GetActorRotation();
//    //FRotator TargetRotation = FRotator(0, CurrentRotation.Yaw, 0); // Keep yaw, but zero pitch and roll
//    //FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, LevelingTurnSpeed);
//    //ShipMesh->SetWorldRotation(NewRotation);
//
//    //// --- Update Grabbed Object ---
//    //if (PhysicsHandle->GetGrabbedComponent())
//    //{
//    //    FVector TargetLocation = GetActorLocation() - FVector(0, 0, TractorBeamRange * 0.5f);
//    //    PhysicsHandle->SetTargetLocation(TargetLocation);
//    //}
//}
//
