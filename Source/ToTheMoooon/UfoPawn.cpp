// UfoPawn.cpp

#include "UfoPawn.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Kismet/KismetSystemLibrary.h" // Needed for Cone Trace

// --- CONSTRUCTOR: Create and attach all components ---
AUfoPawn::AUfoPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
    SetRootComponent(ShipMesh);
    ShipMesh->SetSimulatePhysics(true);
    ShipMesh->SetEnableGravity(true);

    // --- Add these back ---
    LeftThruster = CreateDefaultSubobject<USceneComponent>(TEXT("LeftThruster"));
    LeftThruster->SetupAttachment(ShipMesh);

    RightThruster = CreateDefaultSubobject<USceneComponent>(TEXT("RightThruster"));
    RightThruster->SetupAttachment(ShipMesh);
    // --------------------

    FBodyInstance* BodyInstance = ShipMesh->GetBodyInstance();
    BodyInstance->bLockYTranslation = true;
    BodyInstance->bLockYRotation = true;
    // --- UNLOCK ROLL ROTATION ---
    BodyInstance->bLockXRotation = false; // Set to false to allow the ship to roll

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

    PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

    /* working code before new thruster fucntionality */
//    PrimaryActorTick.bCanEverTick = true;
//
//    // Create the ship mesh, make it the root, and enable physics
//    ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
//    SetRootComponent(ShipMesh);
//    ShipMesh->SetSimulatePhysics(true);
//    ShipMesh->SetEnableGravity(true);
//    // NEW SIDE-VIEW CONSTRAINTS
//    FBodyInstance* BodyInstance = ShipMesh->GetBodyInstance();
//    // Lock movement on the Y-axis (prevents moving toward/away from the camera)
//    BodyInstance->bLockYTranslation = true;
//    // Lock rotation on the X and Y axes to keep the ship oriented correctly
//    BodyInstance->bLockXRotation = true;
//    BodyInstance->bLockYRotation = true;
//
//    /*
//    // Important for 2.5D: Constrain movement to the X-Y plane for rotation
//    ShipMesh->GetBodyInstance()->bLockZRotation = true;
//    ShipMesh->GetBodyInstance()->bLockYRotation = true;
//    */
//
//    // Create thruster points and attach them to the mesh
//    LeftThruster = CreateDefaultSubobject<USceneComponent>(TEXT("LeftThruster"));
//    LeftThruster->SetupAttachment(ShipMesh);
//
//    RightThruster = CreateDefaultSubobject<USceneComponent>(TEXT("RightThruster"));
//    RightThruster->SetupAttachment(ShipMesh);
//
//    // ---new 2D camera mode---
//// Create the camera spring arm. For a 2D game, can disable lag for a tighter feel.
//    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
//    SpringArm->SetupAttachment(RootComponent);
//    SpringArm->TargetArmLength = 1500.0f; // How far away the camera is
//    SpringArm->bEnableCameraLag = true; // Optional: A true 2D feel often has no camera lag
//    SpringArm->bDoCollisionTest = false; // Don't try to move camera around obstacles
//	SpringArm->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f)); //Side view camera angle
//
//    // Create and attach the camera
//    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
//    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
//
//	// Set the camera to Orthographic for 2D projection or Perspective for 2.5D
//    Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
//    // Set the size of the viewing area. Adjust this value to zoom in or out.
//    Camera->SetOrthoWidth(400.0f);
//	
//    /* ---old 2.5D 3D camera code---
//    // Create the camera spring arm (for smooth camera movement)
//    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
//    SpringArm->SetupAttachment(RootComponent);
//    SpringArm->TargetArmLength = 1500.0f;
//    SpringArm->bEnableCameraLag = true;
//    SpringArm->bDoCollisionTest = false; // Don't try to move camera around obstacles
//    SpringArm->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f)); // Angled top-down view
//
//    // Create and attach the camera
//    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
//    Camera->SetupAttachment(SpringArm);
//    */
//
//    // Create the physics handle
//    PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
}

void AUfoPawn::BeginPlay()
{
    Super::BeginPlay();
}

// --- TICK: Called every frame ---
void AUfoPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // If the tractor beam is active, run its logic
    if (bIsTractorBeamActive)
    {
        HandleTractorBeam(DeltaTime);
    }
}

// --- INPUT BINDING ---
void AUfoPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Bind new 2D movement axes
    PlayerInputComponent->BindAxis("MoveHorizontal", this, &AUfoPawn::MoveHorizontal);
    PlayerInputComponent->BindAxis("ThrustUp", this, &AUfoPawn::ThrustUp);

    // Bind tractor beam actions
    PlayerInputComponent->BindAction("TractorBeam", IE_Pressed, this, &AUfoPawn::StartTractorBeam);
    PlayerInputComponent->BindAction("TractorBeam", IE_Released, this, &AUfoPawn::StopTractorBeam);
}

void AUfoPawn::MoveHorizontal(float Value)
{
    if (FMath::Abs(Value) > 0.1f)
    {
        // THE FIX: Use FMath::Abs(Value) to ensure the force is always upward.
        // The direction of the roll is handled by which thruster we apply the force to.
        const FVector ForceDirection = FVector::UpVector * FMath::Abs(Value) * RollForce;

        if (Value > 0) // Pressing 'D' to roll right
        {
            // Apply upward force on the left thruster
            ShipMesh->AddForceAtLocation(ForceDirection, LeftThruster->GetComponentLocation());
        }
        else // Pressing 'A' to roll left
        {
            // Apply upward force on the right thruster
            ShipMesh->AddForceAtLocation(ForceDirection, RightThruster->GetComponentLocation());
        }
    }
    /* old working code no multi thruster functionality */
    //if (FMath::Abs(Value) > 0.1f)
    //{
    //    // Apply force along the world's X-axis for left/right movement
    //    const FVector ForceDirection = FVector::RightVector * Value * ThrustForce;
    //    ShipMesh->AddForce(ForceDirection);
    //}
}

void AUfoPawn::ThrustUp(float Value)
{
    // Don't allow vertical thrust if the tractor beam is active and hovering
    if (bIsTractorBeamActive)
    {
        return;
    }
    if (FMath::Abs(Value) > 0.1f)
    {
        // Apply force along the SHIP'S up-vector.
        // When rolled, this will push the ship sideways.
        const FVector ForceDirection = GetActorUpVector() * Value * MainThrustForce;
        ShipMesh->AddForce(ForceDirection);
    }

    /* old working code basic up thrust*/
    //if (FMath::Abs(Value) > 0.1f)
    //{
    //    // Apply force along the world's Z-axis for up/down movement
    //    const FVector ForceDirection = FVector::UpVector * Value * ThrustForce;
    //    ShipMesh->AddForce(ForceDirection);
    //}
}


// --- TRACTOR BEAM IMPLEMENTATION ---

void AUfoPawn::StartTractorBeam()
{
    bIsTractorBeamActive = true;
    ShipMesh->SetEnableGravity(false); // Disable gravity while hovering

    // Define the cone trace for objects
    FVector Start = GetActorLocation();
    FVector End = Start - FVector(0, 0, TractorBeamRange);
    TArray<FHitResult> OutHits;
    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(this);

    // Perform the cone trace to find physics objects
    // Replace the ConeTraceMultiForObjects call with SphereTraceMultiForObjects, as ConeTraceMultiForObjects does not exist in UKismetSystemLibrary.

    bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(),
        Start,
        End,
        TractorBeamRadius, // Use radius for sphere trace
		{ EObjectTypeQuery::ObjectTypeQuery3 }, // Look for PhysicsBody objects
        false,
        ActorsToIgnore,
        EDrawDebugTrace::ForDuration,
        OutHits,
        true
    );


    if (bHit)
    {
        for (const FHitResult& Hit : OutHits)
        {
            UPrimitiveComponent* HitComponent = Hit.GetComponent();
            if (HitComponent && HitComponent->IsSimulatingPhysics())
            {
                // Grab the first physics object we find
                PhysicsHandle->GrabComponentAtLocation(HitComponent, NAME_None, Hit.ImpactPoint);
                return; // Exit after grabbing one object
            }
        }
    }
}

void AUfoPawn::StopTractorBeam()
{
    bIsTractorBeamActive = false;
    ShipMesh->SetEnableGravity(true); // Re-enable gravity
    PhysicsHandle->ReleaseComponent();
}

void AUfoPawn::HandleTractorBeam(float DeltaTime)
{
    // --- Hover and Leveling Logic ---
    // 1. Hover: Counteract any existing vertical velocity to hover in place
    float CurrentZVelocity = ShipMesh->GetPhysicsLinearVelocity().Z;
    float HoverForce = -CurrentZVelocity / DeltaTime; // Damping force
    ShipMesh->AddForce(FVector(0, 0, HoverForce), NAME_None, true); // Use Accel Change for mass-independent force

    // 2. Level Out: Smoothly rotate the ship to be flat
    FRotator CurrentRotation = GetActorRotation();
    FRotator TargetRotation = FRotator(0, CurrentRotation.Yaw, 0); // Keep yaw, but zero pitch and roll
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, LevelingTurnSpeed);
    ShipMesh->SetWorldRotation(NewRotation);

    // --- Update Grabbed Object ---
    if (PhysicsHandle->GetGrabbedComponent())
    {
        FVector TargetLocation = GetActorLocation() - FVector(0, 0, TractorBeamRange * 0.5f);
        PhysicsHandle->SetTargetLocation(TargetLocation);
    }
}