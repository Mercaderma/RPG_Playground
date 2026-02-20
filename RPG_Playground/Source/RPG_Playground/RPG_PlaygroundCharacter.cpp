// Copyright Epic Games, Inc. All Rights Reserved.

#include "RPG_PlaygroundCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "RPG_Playground.h"
#include "MotionWarpingComponent.h"

#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

ARPG_PlaygroundCharacter::ARPG_PlaygroundCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Motion Warping Component
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	//Player starts NOT Crouching
	bCrouched = false;
	fDefaultArmLength = 400.0f;
	fCrouchArmLength = 550.0f;
	fDefaultWalkSpeed = 500.0f;
	fCrouchWalkSpeed = 350.0f;

	bCanWarp = false;

	CrouchTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("CrouchTimeline"));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ARPG_PlaygroundCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (CrouchCurve)
	{
		FOnTimelineFloat ProgressFunction;
		ProgressFunction.BindUFunction(this, FName("HandleCrouchProgress"));

		CrouchTimeline->AddInterpFloat(CrouchCurve, ProgressFunction);
		CrouchTimeline->SetLooping(false);
	}
}

void ARPG_PlaygroundCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ARPG_PlaygroundCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ARPG_PlaygroundCharacter::Crouch);

		// Vault
		EnhancedInputComponent->BindAction(VaultAction, ETriggerEvent::Started, this, &ARPG_PlaygroundCharacter::Vault);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARPG_PlaygroundCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ARPG_PlaygroundCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARPG_PlaygroundCharacter::Look);
	}
	else
	{
		UE_LOG(LogRPG_Playground, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ARPG_PlaygroundCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ARPG_PlaygroundCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ARPG_PlaygroundCharacter::HandleCrouchProgress(float Value)
{
	float NewLength = FMath::Lerp(
		fDefaultArmLength,
		fCrouchArmLength,
		Value
	);

	GetCameraBoom()->TargetArmLength = NewLength;
}

void ARPG_PlaygroundCharacter::Crouch(const FInputActionValue& Value)
{
	if (bCrouched) 
	{
		bCrouched = false;
		//Normal Speed and Camera distance
		GetCharacterMovement()->MaxWalkSpeed = fDefaultWalkSpeed;
		CrouchTimeline->ReverseFromEnd();
	}
	else
	{
		bCrouched = true;
		//Lower Speed and Camera Far Away
		GetCharacterMovement()->MaxWalkSpeed = fCrouchWalkSpeed;
		CrouchTimeline->PlayFromStart();

	}
}

void ARPG_PlaygroundCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ARPG_PlaygroundCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ARPG_PlaygroundCharacter::DoJumpStart()
{
	// If Jump while crouching, player stops crouching
	if (bCrouched) 
	{
			bCrouched = false;
			//Normal Speed and Camera distance
			GetCharacterMovement()->MaxWalkSpeed = fDefaultWalkSpeed;
			CrouchTimeline->ReverseFromEnd();
	}
	// signal the character to jump
	Jump();
}

void ARPG_PlaygroundCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ARPG_PlaygroundCharacter::Vault()
{
	// Hit result used to store obstacle detection data
	FHitResult ObstacleHit;

	// Check if there is an obstacle in front of the character
	if (CheckObstacle(ObstacleHit))
	{	
		float Height = 0.0f;
		// Check if there is enough clearance to perform a vault
		if (CheckClearance(ObstacleHit, Height))
		{
			// If everything is valid, start the vault motion warp
			VaultMotionWarp();
		}
	}
}

bool ARPG_PlaygroundCharacter::CheckObstacle(FHitResult& OutHit)
{
	// Get current character location and forward direction
	FVector ActorLocation = GetActorLocation();
	FVector ForwardVector = GetActorForwardVector();

	// Horizontal sweep configuration
	const float HorizontalDistance = 180.0f;
	const float HorizontalZStep = 30.0f;
	const float SphereRadius = 5.0f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);// Ignore self during trace

	// Perform multiple horizontal sweeps at different heights
	for (int32 Index = 0; Index <= 2; Index++)
	{
		FVector Start = ActorLocation + FVector(0.0f, 0.0f, Index * HorizontalZStep);
		FVector End = Start + ForwardVector * HorizontalDistance;

		bool bHit = GetWorld()->SweepSingleByChannel(
			OutHit,
			Start,
			End,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(SphereRadius),
			Params
		);
		// Debug line to visualize sweep
		//DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, 2.0f);

		// If any sweep hits, obstacle detected
		if (bHit)
		{
			return true;
		}
	}

	return false;
}
bool ARPG_PlaygroundCharacter::CheckClearance(const FHitResult& ObstacleHit, float& OutHeight)
{
	FVector ForwardVector = GetActorForwardVector();

	// Forward stepping and vertical trace configuration
	const float StepForwardDistance = 30.0f;
	const float VerticalTraceDistance = 150.0f;
	const float SphereRadius = 5.0f;
	const int32 VerticalChecks = 6;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// Base point from obstacle impact
	FVector BasePoint = ObstacleHit.ImpactPoint;

	bool bFoundLanding = false;

	// Step forward and check vertical clearance
	for (int32 Index = 0; Index < VerticalChecks; Index++)
	{
		FVector ForwardOffset = ForwardVector * (Index * StepForwardDistance);

		// Start trace slightly above obstacle
		FVector Start = BasePoint + ForwardOffset + FVector(0.0f, 0.0f, 100.0f);
		FVector End = Start - FVector(0, 0, VerticalTraceDistance);

		FHitResult VerticalHit;

		bool bHit = GetWorld()->SweepSingleByChannel(
			VerticalHit,
			Start,
			End,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(SphereRadius),
			Params
		);

		// Debug Red = blocked, Green = clear
		//FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		//DrawDebugLine(GetWorld(), Start, End, DebugColor, false, 2.0f);

		if (bHit)
		{
			// First hit defines the vault start position
			if (Index == 0)
			{
				VaultStartPos = VerticalHit.ImpactPoint;
				/* Debug
				DrawDebugSphere(
					GetWorld(),
					VaultStartPos,
					10.0f,
					12,
					FColor::Magenta,
					false,
					5.0f
				);*/
			}

			// Store middle position for motion warp alignment
			VaultMiddlePos = VerticalHit.ImpactPoint;
			/* Debug
			DrawDebugSphere(
				GetWorld(),
				VaultMiddlePos,
				8.0f,
				8,
				FColor::Yellow,
				false,
				5.0f
			);*/
		}
		else
		{
			// No obstacle above -> check for ground to land on
			FVector GroundStart = Start;
			FVector GroundEnd = Start - FVector(0.0f, 0.0f, 500.0f);

			FHitResult GroundHit;
			
			bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
				GroundHit,
				GroundStart,
				GroundEnd,
				ECC_Visibility,
				Params
			);
			// Debug
			//DrawDebugLine(GetWorld(), GroundStart, GroundEnd, FColor::Cyan, false, 5.0f);

			if (bGroundHit)
			{
				// Store landing position
				VaultLandPos = GroundHit.ImpactPoint;

				// Allow warp execution
				bCanWarp = true;
				/*Debug
				DrawDebugSphere(
					GetWorld(),
					VaultLandPos,
					12.0f,
					12,
					FColor::Cyan,
					false,
					10.0f
				);
				*/
				bFoundLanding = true;
			}
			// Stop checking further forward positions
			break; 
		}
	}

	return bFoundLanding;
}

bool ARPG_PlaygroundCharacter::FindForwardLanding(FVector& OutLandingPoint)
{
	// Trace forward and down to find valid landing surface
	FVector ActorLocation = GetActorLocation();
	FVector Forward = GetActorForwardVector();

	const float ForwardDistance = 80.0f;
	const float TraceUp = 200.0f;
	const float TraceDown = 1000.0f;

	FVector BasePoint = ActorLocation + (Forward * ForwardDistance);

	FVector TraceStart = BasePoint + FVector(0.0f, 0.0f, TraceUp);
	FVector TraceEnd = BasePoint - FVector(0.0f, 0.0f, TraceDown);

	FHitResult Hit;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		Params
	);

	/*Debug
	FColor DebugColor = bHit ? FColor::Green : FColor::Red;

	DrawDebugLine(
		GetWorld(),
		TraceStart,
		TraceEnd,
		DebugColor,
		false,
		5.0f,
		0,
		2.0f
	);
	*/
	if (bHit)
	{

		OutLandingPoint = Hit.ImpactPoint;
		/*Debug
		DrawDebugSphere(
			GetWorld(),
			OutLandingPoint,
			10.0f,
			12,
			FColor::Cyan,
			false,
			5.0f
		);
		*/
		return true;
	}

	return false;
}

void ARPG_PlaygroundCharacter::VaultMotionWarp()
{
	// Ensure required components and montage are valid
	if (!MotionWarping || !VaultMontage) return;

	// Ensure mesh height is within acceptable landing range
	const float MeshZ = GetMesh()->GetComponentLocation().Z;
	const float MinZ = VaultLandPos.Z - 50.0f;
	const float MaxZ = VaultLandPos.Z + 50.0f;

	const bool bInRange = FMath::IsWithinInclusive(MeshZ, MinZ, MaxZ);

	// Abort if warp conditions are not valid
	if (!bCanWarp || !bInRange)
	{
		return;
	}

	// Temporarily disable walking physics and collision
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	SetActorEnableCollision(false);

	// Disable camera collision test to prevent camera snapping
	CameraBoom->bDoCollisionTest = false;

	// Update motion warp targets
	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(
		FName("VaultStart"),
		VaultStartPos,
		GetActorRotation()
	);

	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(
		FName("VaultMiddle"),
		VaultMiddlePos,
		GetActorRotation()
	);

	MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(
		FName("VaultLand"),
		VaultLandPos,
		GetActorRotation()
	);

	// Play vault montage
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &ARPG_PlaygroundCharacter::OnVaultMontageEnded);

		AnimInstance->Montage_Play(VaultMontage, 1.5f);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, VaultMontage);
	}
}

void ARPG_PlaygroundCharacter::OnVaultMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Ensure this callback corresponds to the correct montage
	if (Montage != VaultMontage) return;

	// Restore movement mode and collision
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	SetActorEnableCollision(true);

	// Reset warp state
	bCanWarp = false;

	// Reset landing position to avoid stale data usage
	VaultLandPos = FVector(0.0f, 0.0f, 20000.0f);

	// Re-enable camera collision
	CameraBoom->bDoCollisionTest = true;

	// Clean up warp targets
	if (MotionWarping)
	{
		MotionWarping->RemoveWarpTarget(FName("VaultStart"));
		MotionWarping->RemoveWarpTarget(FName("VaultMiddle"));
		MotionWarping->RemoveWarpTarget(FName("VaultLand"));
	}
}