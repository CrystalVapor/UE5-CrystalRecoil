// Copyright CrystalVapor 2026, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "CRRecoilComponent.h"
#include "CRRecoilSpreadComponent.generated.h"

/**
* Extends UCRRecoilComponent with a heat-based spread system.
*
* Each shot adds heat via ShotToHeatCurve. Heat drives spread angle via HeatToSpreadAngleCurve.
* Heat cools down over time via HeatToCooldownPerSecondCurve after RecoilHeatCooldownDelay.
*
* Usage:
* - Assign the three curves in the editor or via Blueprint
* - Call GetCurrentSpreadAngle() before each shot to get the current spread angle
* - Bind OnHeatChanged to react to heat changes (e.g. crosshair scaling)
*/

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCRSpreadRecoilHeatChangedDelegate, const float, NewHeat, const float, OldHeat);

UCLASS(ClassGroup = (CrystalRecoil), Meta = (BlueprintSpawnableComponent), DisplayName = "Recoil Component (with Spread)")
class CRYSTALRECOIL_API UCRRecoilSpreadComponent : public UCRRecoilComponent
{
	GENERATED_BODY()

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* Call before each shot to get the current spread angle for projectile direction calculation
	* Returns 0 if any of the three curves are not set
	*/
	UFUNCTION(BlueprintCallable, Category = "Spread Recoil Component")
	float GetCurrentSpreadAngle() const;

	/**
	* Called automatically on each ApplyShot()
	* Exposed for ability to add extra heat from external sources, e.g. melee hits, abilities, debug cheats
	*/
	UFUNCTION(BlueprintCallable, Category = "Spread Recoil Component")
	void AddRecoilHeat(const float InHeat);

	/**
	* Returns the current heat value in the range [0, MaxRecoilHeat]
	*/
	UFUNCTION(BlueprintCallable, Category = "Spread Recoil Component")
	float GetRecoilHeat() const;

	/**
	* Overrides the maximum heat cap at runtime
	* Useful for abilities or buffs that temporarily increase or decrease the weapon's heat capacity
	*/
	UFUNCTION(BlueprintCallable, Category = "Spread Recoil Component")
	void SetMaxRecoilHeat(const float InMaxHeat);

	/**
	* Overrides the cooldown delay at runtime
	* Time after the last shot before heat starts cooling down
	*/
	UFUNCTION(BlueprintCallable, Category = "Spread Recoilponent")
	void SetRecoilHeatCoolDownDelay(const float InDelay);

	/**
	* Fired whenever heat changes, e.g. to drive crosshair scaling or VFX
	* Passes the new and old heat values
	*/
	UPROPERTY(BlueprintAssignable, Category = "Spread Recoil Component")
	FCRSpreadRecoilHeatChangedDelegate OnHeatChanged;

protected:
	virtual void ApplyShot() override;

	void SetRecoilHeat(const float InHeat);

	void DoHeatCooldown(const float DeltaTime);

	bool ReadyToCalculateRecoil() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0.f), Category = "Recoil Spread|Heat")
	float MaxRecoilHeat = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0.f, ForceUnits = "s"), Category = "Recoil Spread|Heat")
	float RecoilHeatCooldownDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil Spread|Curves")
	FRuntimeFloatCurve ShotToHeatCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil Spread|Curves")
	FRuntimeFloatCurve HeatToSpreadAngleCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil Spread|Curves")
	FRuntimeFloatCurve HeatToCooldownPerSecondCurve;

	float CurrentRecoilHeat = 0.f;
};
