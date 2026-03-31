// Copyright CrystalVapor 2026, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CRRecoilPattern.generated.h"

class UCRRecoilUnitGraph;

UENUM()
enum class ERecoilPatternEndBehavior : uint8
{
	// Continues applying the last shot's recoil delta (Infinite climb)
	RepeatLast,

	// Recoil stops completely (Gun stabilizes at the last position)
	Stop,

	// Loops the pattern back to a specific shot index
	RestartFromCustomIndex,

	// Switches to procedural random recoil defined in RandomizedRecoil
	Random
};

USTRUCT()
struct FRecoilPatternRandomizedRecoil
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Pattern")
	FVector2D RandomXRange = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Pattern")
	FVector2D RandomYRange = FVector2D::ZeroVector;
};

UCLASS()
class CRYSTALRECOIL_API UCRRecoilPattern : public UDataAsset
{
	GENERATED_BODY()

public:
	UCRRecoilPattern();

	UCRRecoilUnitGraph* GetUnitGraph() const;

	/**
	* Returns the incremental recoil delta for the current shot and advances ShotIndex to the next one
	* PatternEndBehavior controls what happens once ShotIndex exceeds the pattern length:
	*   Stop                   - returns zero; index stays put
	*   RepeatLast             - returns the last delta forever; index stays put
	*   RestartFromCustomIndex - resets index to the loop point, then advances normally
	*   Random                 - returns a random delta; index stays put
	*/
	FVector2f ConsumeShot(int32& ShotIndex) const;

	int32 GetMaxShotIndex() const;

	UPROPERTY()
	TObjectPtr<UCRRecoilUnitGraph> RecoilUnitGraph;

	/**
	* Controls how fast the recoil kick reaches its peak
	* 0.0 = Slow, smooth rise (floaty, heavy weapon feel)
	* 1.0 = Instant violent snap (sharp, aggressive kick)
	*/
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.f, ClampMax = 1.f), Category = "Uplift")
	float UpliftSpeed = 0.7f;

	/**
	* Time to wait after the last shot before recovery begins
	* Set to 0 for recovery to begin immediately on the next frame after the last shot
	*/
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.f, ForceUnits = "s"), Category = "Recovery")
	float RecoveryDelay = 0.1f;

	/**
	* The starting speed of the return movement
	* High Value: Returns immediately at high speed (Snappy start)
	* Low Value: Eases in slowly (Smooth start)
	*/
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.01f, ForceUnits = "deg/s"), Category = "Recovery")
	float InitialRecoverySpeed = 2.f;

	/**
	* The maximum speed the camera can move while returning to its pre-shot position
	*/
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.01f, ForceUnits = "deg/s"), Category = "Recovery")
	float MaxRecoverySpeed = 10.f;

	/**
	* How fast the recovery speed ramps up (in deg/s²) from Initial to Max
	* High Value: Reaches max speed almost instantly
	* Low Value: Slowly accelerates the return motion (Spongey feel)
	*/
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.01f), Category = "Recovery")
	float RecoveryAcceleration = 40.f;

	/**
	* If accumulated player input exceeds this threshold during the burst, cancel recovery entirely
	* Measured from StartShooting until recovery would begin (after RecoveryDelay)
	* Set to 0 to disable (recovery always completes regardless of player aiming)
	*/
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.f, ClampMax = 90.f, ForceUnits = "deg"), Category = "Recovery")
	float RecoveryCancelThreshold = 0.f;

	/**
	* Defines behavior when the player shoots beyond the defined pattern length
	* RepeatLast: Good for high-recoil weapons (AK-47 style infinite climb)
	* Stop: Good for low recoil weapons that stabilize (Laser beams)
	* Random: Good for heavy weapons (LMGs) with chaotic spray at the end
	*/
	UPROPERTY(EditAnywhere, Category = "Pattern")
	ERecoilPatternEndBehavior PatternEndBehavior = ERecoilPatternEndBehavior::RepeatLast;

	/**
	* The shot index to loop back to when the pattern ends
	* 0: Loops the entire pattern from the start
	* High Value: Loops only the later part of the pattern (The "Sustained Fire" phase)
	*/
	UPROPERTY(EditAnywhere, Meta = (EditCondition = "PatternEndBehavior == ERecoilPatternEndBehavior::RestartFromCustomIndex", EditConditionHides = true, ClampMin = 0), Category = "Pattern")
	int32 CustomRecoilRestartIndex = 0;

	/**
	* Configuration for procedural random recoil
	* Defines the min/max X and Y kick applied after the pattern finishes
	* Used to create unpredictable noise for sustained fire
	*/
	UPROPERTY(EditAnywhere, Meta = (EditCondition = "PatternEndBehavior == ERecoilPatternEndBehavior::Random", EditConditionHides = true), Category = "Pattern")
	FRecoilPatternRandomizedRecoil RandomizedRecoil;
};
