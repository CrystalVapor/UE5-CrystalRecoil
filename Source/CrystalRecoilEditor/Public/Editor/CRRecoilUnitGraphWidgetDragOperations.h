// Copyright CrystalVapor 2026, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CRRecoilPatternEditor.h"
#include "Data/CRRecoilUnitGraph.h"
#include "Framework/DelayedDrag.h"

class FCRRecoilUnitSelection;
class UCRRecoilUnitGraph;

class FCRUnitGraphViewDelayedDrag : public FDelayedDrag
{
public:
	FCRUnitGraphViewDelayedDrag(const FVector2f InInitialPosition, const FKey& InEffectiveKey)
		: FDelayedDrag(static_cast<FVector2D>(InInitialPosition), InEffectiveKey)
	{
	}
};

class FCRUnitGraphSelectionDelayedDrag : public FDelayedDrag
{
public:
	FCRUnitGraphSelectionDelayedDrag(const FVector2f InInitialPosition, const FKey& InEffectiveKey)
		: FDelayedDrag(static_cast<FVector2D>(InInitialPosition), InEffectiveKey)
	{
		TriggerDistance = 0.f;
	}

	FSlateRect GetSelectionRect() const
	{
		const FVector2D LocalInitialPosition = GetInitialPosition();
		const FVector2f LocalEndPosition = EndPosition;
		return FSlateRect
		(
			FMath::Min(LocalInitialPosition.X, LocalEndPosition.X),
			FMath::Min(LocalInitialPosition.Y, LocalEndPosition.Y),
			FMath::Max(LocalInitialPosition.X, LocalEndPosition.X),
			FMath::Max(LocalInitialPosition.Y, LocalEndPosition.Y)
		);
	}

	FVector2f EndPosition = FVector2f::ZeroVector;
};

class FCRUnitGraphScaleUnitsDelayedDrag
{
public:
	FCRUnitGraphScaleUnitsDelayedDrag(UCRRecoilUnitGraph* UnitGraph, const FCRRecoilUnitSelection& UnitSelection, const FVector2f InInitialRecoilLocation, const FVector2f InInitialPosition, const FKey& InEffectiveKey);

	~FCRUnitGraphScaleUnitsDelayedDrag();

	void Cancel();

	void ApplyScaling(const FCRRecoilUnitSelection& RecoilUnitSelection, float NewScale);

	TArray<FCRRecoilUnit> CachedRecoilUnits;

	TWeakObjectPtr<UCRRecoilUnitGraph> CachedUnitGraph;

	float CurrentScale = 1.f;

	float NormalVectorSizePanel = 1.f;

	FVector2f EndPosition = FVector2f::ZeroVector;

	FVector2f InitialRecoilLocation = FVector2f::ZeroVector;

	FVector2f InitialPanelLocation = FVector2f::ZeroVector;

	int32 BaseUnitID = 0;
};

class FCRUnitGraphMoveUnitsDelayedDrag : public FDelayedDrag
{
public:
	FCRUnitGraphMoveUnitsDelayedDrag(UCRRecoilUnitGraph* UnitGraph, const FCRRecoilUnitSelection& UnitSelection, const FVector2f InInitialRecoilLocation, const FVector2f InInitialPosition, const FKey& InEffectiveKey);

	~FCRUnitGraphMoveUnitsDelayedDrag();

	void Cancel();

	void ApplyMovement(const FCRRecoilUnitSelection& UnitSelection, const FVector2f& Movement) const;

	TArray<FCRRecoilUnit> CachedRecoilUnits;

	TWeakObjectPtr<UCRRecoilUnitGraph> CachedUnitGraph;

	FVector2f LastRecoilCoordsLocation = FVector2f::ZeroVector;

	FVector2D CachedPannerVector = FVector2d::ZeroVector;
};
