// Copyright Ben Tristem 2016.

#include "BuildingEscape.h"
#include "Grabber.h"

#define OUT

// Sets default values for this component's properties
UGrabber::UGrabber()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UGrabber::BeginPlay()
{
	Super::BeginPlay();
	FindPhysicsHandleComponent();
	SetupInputComponent();
}

/// Look for attached Physics Handle
void UGrabber::FindPhysicsHandleComponent()
{
	PhysicsHandle = GetOwner()->FindComponentByClass<UPhysicsHandleComponent>();
}

/// Look for attached Input Component (only appears at run time)
void UGrabber::SetupInputComponent()
{
	InputComponent = GetOwner()->FindComponentByClass<UInputComponent>();

	if (!ensure(InputComponent)) { return; }
	InputComponent->BindAction("Grab", IE_Pressed, this, &UGrabber::Grab);
	InputComponent->BindAction("Grab", IE_Released, this, &UGrabber::Release);
	InputComponent->BindAction("Throw", IE_Pressed, this, &UGrabber::Throw);
}

// Called every frame
void UGrabber::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ensure(PhysicsHandle)) { return; }

	if (PhysicsHandle->GrabbedComponent)
	{
		PhysicsHandle->SetTargetLocation(GetReachLineEnd()); //TODO Unreal engine doesn't seem to need this
	}
}

void UGrabber::Grab() {
	/// LINE TRACE and see if we reach any actors with physics body collision channel set
	auto HitResult = GetFirstPhysicsBodyInReach();
	auto ActorHit = HitResult.GetActor();

	if (!ActorHit) { return;}
	auto GrabbedComponent = HitResult.GetComponent(); // gets the mesh in our case

	/// If we hit something then grab
	if (!PhysicsHandle) { return; }
	PhysicsHandle->GrabComponent(
		GrabbedComponent,
		NAME_None, // no bones needed
		GrabbedComponent->GetOwner()->GetActorLocation(),
		true // allow rotation
	);
	GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
}

void UGrabber::Release()
{
	if (!ensure(PhysicsHandle)) { return; }
	//Need to reference this before releasing, to find it after
	auto GrabbedComponent = PhysicsHandle->GrabbedComponent;

	if (!(GrabbedComponent)) { return; }
	PhysicsHandle->ReleaseComponent();

	auto PropLinearVelocityVector = GrabbedComponent->GetPhysicsLinearVelocity();
	UE_LOG(LogTemp, Warning, TEXT("Forward Vector: %s"), *PropLinearVelocityVector.ToString());
	auto DampedVector = PropLinearVelocityVector / 5; //This is hardcoded not good TODO remove magic number and allow it to be editable on the pawn bp
	UE_LOG(LogTemp, Warning, TEXT("Damped Vector: %s"), *DampedVector.ToString());
	GrabbedComponent->SetPhysicsLinearVelocity(DampedVector, false, NAME_None);
}

void UGrabber::Throw() 
{

	if (!ensure(PhysicsHandle)) { return; }
	auto GrabbedComponent = PhysicsHandle->GrabbedComponent;

	if (!(GrabbedComponent)) { return; }
	PhysicsHandle->ReleaseComponent();
	GrabbedComponent->WakeRigidBody();

	//Get forward Vector based on player view
	FVector PlayerViewPointLocation;
	FRotator PlayerViewPointRotation;
	GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(
		OUT PlayerViewPointLocation,
		OUT PlayerViewPointRotation
	);
	auto ForwardVector = PlayerViewPointRotation.Vector().GetSafeNormal();

	//Throw the object
	GrabbedComponent->AddImpulse(ImpulsePower*ForwardVector, NAME_None, true);
}

const FHitResult UGrabber::GetFirstPhysicsBodyInReach()
{
	/// Line-trace (AKA ray-cast) out to reach distance
	FHitResult HitResult;
	FCollisionQueryParams TraceParameters(FName(TEXT("")), false, GetOwner());
	GetWorld()->LineTraceSingleByObjectType(
		OUT HitResult,
		GetReachLineStart(),
		GetReachLineEnd(),
		FCollisionObjectQueryParams(ECollisionChannel::ECC_PhysicsBody),
		TraceParameters
	);
	return HitResult;
}

FVector UGrabber::GetReachLineStart()
{
	FVector PlayerViewPointLocation;
	FRotator PlayerViewPointRotation;
	GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(
		OUT PlayerViewPointLocation,
		OUT PlayerViewPointRotation
	);
	return PlayerViewPointLocation;
}

FVector UGrabber::GetReachLineEnd()
{
	FVector PlayerViewPointLocation;
	FRotator PlayerViewPointRotation;
	GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(
		OUT PlayerViewPointLocation,
		OUT PlayerViewPointRotation
	);
	return PlayerViewPointLocation + PlayerViewPointRotation.Vector() * Reach;
}
