// Copyright CrystalVapor 2026, All rights reserved.

#pragma once

#include "Components/CRRecoilComponent.h"
#include "Components/CRRecoilSpreadComponent.h"
#include "CRRecoilScaledTestComponent.generated.h"

UCLASS(Transient, NotBlueprintable, HideDropdown)
class UCRRecoilScaledTestComponent : public UCRRecoilComponent
{
	GENERATED_BODY()

protected:
	virtual bool ProcessDeltaRecoilRotation(FRotator& DeltaRecoilRotation) override
	{
		DeltaRecoilRotation *= 0.5;
		return true;
	}
};

UCLASS(Transient, NotBlueprintable, HideDropdown)
class UCRRecoilTestSpreadComponent : public UCRRecoilSpreadComponent
{
	GENERATED_BODY()

public:
	void InitializeCurves()
	{
		ShotToHeatCurve.GetRichCurve()->AddKey(0.f, 1.f);
		HeatToSpreadAngleCurve.GetRichCurve()->AddKey(0.f, 1.f);
		HeatToCooldownPerSecondCurve.GetRichCurve()->AddKey(0.f, 10.f);
	}

	bool IsTrackingInput() const
	{
		return bTrackingInputDuringFire;
	}
};
