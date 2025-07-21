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

    // Create the ship mesh, make it the root, and enable physics
    ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
    SetRootComponent(ShipMesh);
    ShipMesh->SetSimulatePhysics(true);
    ShipMesh->SetEnableGravity(true);
    // Important for 2.5D: Constrain movement to the X-Y plane for rotation
    ShipMesh->GetBodyInstance()->bLockZRotation = true;
    ShipMesh->GetBodyInstance()->bLockYRotation = true;


    // Create thruster points and attach them to the mesh
    LeftThruster = CreateDefaultSubobject<USceneComponent>(TEXT("LeftThruster"));
    LeftThruster->SetupAttachment(ShipMesh);

    RightThruster = CreateDefaultSubobject<USceneComponent>(TEXT("RightThruster"));
    RightThruster->SetupAttachment(ShipMesh);

    // Create the camera spring arm (for smooth camera movement)
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 1200.0f;
    SpringArm->bEnableCameraLag = true;
    SpringArm->SetRelativeRotation(FRotator(-50.0f, 0.0f, 0.0f)); // Angled top-down view

    // Create and attach the camera
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);

    // Create the physics handle
    PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));
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

    // Bind movement axes
    PlayerInputComponent->BindAxis("MoveUp", this, &AUfoPawn::MoveUp);
    PlayerInputComponent->BindAxis("Turn", this, &AUfoPawn::Turn);

    // Bind tractor beam actions
    PlayerInputComponent->BindAction("TractorBeam", IE_Pressed, this, &AUfoPawn::StartTractorBeam);
    PlayerInputComponent->BindAction("TractorBeam", IE_Released, this, &AUfoPawn::StopTractorBeam);
}

// --- MOVEMENT IMPLEMENTATION ---
void AUfoPawn::MoveUp(float Value)
{
    if (FMath::Abs(Value) > 0.1f)
    {
        // Apply force from both thrusters for forward movement
        FVector ForceDirection = ShipMesh->GetForwardVector() * Value * ThrustForce;
        ShipMesh->AddForce(ForceDirection);
    }
}

void AUfoPawn::Turn(float Value)
{
    if (FMath::Abs(Value) > 0.1f)
    {
        // Apply force at a specific thruster's location to create torque and turn the ship
        FVector ForceDirection = ShipMesh->GetForwardVector() * TurnForce;
        if (Value > 0) // Turning Right
        {
            ShipMesh->AddForceAtLocation(ForceDirection, LeftThruster->GetComponentLocation());
        }
        else // Turning Left
        {
            ShipMesh->AddForceAtLocation(ForceDirection, RightThruster->GetComponentLocation());
        }
    }
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