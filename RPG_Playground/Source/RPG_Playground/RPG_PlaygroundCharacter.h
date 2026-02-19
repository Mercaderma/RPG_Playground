// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/TimelineComponent.h"

#include "RPG_PlaygroundCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UMotionWarpingComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ARPG_PlaygroundCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Motion Warping */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* MotionWarping;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Vault Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* VaultAction;

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CrouchAction;

	/** Bool Is Crouching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCrouched;

	/** Camera values */
	float fDefaultArmLength;
	float fCrouchArmLength;

	/** Speed Values */
	float fDefaultWalkSpeed;
	float fCrouchWalkSpeed;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** TimeLine Component */
	UPROPERTY()
	UTimelineComponent* CrouchTimeline;

	/** CRoauchCurve */
	UPROPERTY(EditAnywhere, Category = "Camera")
	UCurveFloat* CrouchCurve;

	/** Bool Can Warp */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanWarp;

	/** Animation montage*/
	UPROPERTY(EditAnywhere, Category = "Vault")
	UAnimMontage* VaultMontage;

	/** Warp target position at the start of the vault */
	UPROPERTY(EditAnywhere, Category = "Vault")
	FVector VaultStartPos;

	/** Warp target position at the middle of the vault */
	UPROPERTY(EditAnywhere, Category = "Vault")
	FVector VaultMiddlePos;

	/** Warp target position at the landing point of the vault */
	UPROPERTY(EditAnywhere, Category = "Vault")
	FVector VaultLandPos;



public:

	/** Constructor */
	ARPG_PlaygroundCharacter();	

	virtual void BeginPlay() override;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for looking input */
	void Crouch(const FInputActionValue& Value);


	/** Called for Camera Handle */
	UFUNCTION()
	void HandleCrouchProgress(float Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Performs obstacle and clearance checks before triggering motion warp */
	UFUNCTION(BlueprintCallable, Category="Input")
	void Vault();

	/* Restores movement, collision and cleans up warp targets */
	UFUNCTION()
	void OnVaultMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Sets movement mode, updates warp targets and plays the montage */
	UFUNCTION(BlueprintCallable, Category = "Vault")
	void VaultMotionWarp();

	/** Performs forward horizontal sweeps to detect a vaultable obstacle */
	bool CheckObstacle(FHitResult& OutHit);

	/** Checks vertical clearance over the detected obstacle and determines landing position */
	bool CheckClearance(const FHitResult& ObstacleHit, float& OutHeight);

	/** Performs a forward downward trace to find a landing surface */
	bool FindForwardLanding(FVector& OutLandingPoint);

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

