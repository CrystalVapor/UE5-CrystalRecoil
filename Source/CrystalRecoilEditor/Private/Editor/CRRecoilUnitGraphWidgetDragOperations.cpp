// Copyright CrystalVapor 2026, All rights reserved.

#include "Editor/CRRecoilUnitGraphWidgetDragOperations.h"
#include "ScopedTransaction.h"

namespace
{
	void CommitDraggedUnits(UCRRecoilUnitGraph* UnitGraph, const TArray<FCRRecoilUnit>& InitialUnits, const FText& Description)
	{
		if (!UnitGraph)
		{
			return;
		}

		TArray<FCRRecoilUnit>& Units = UnitGraph->GetRecoilUnits();
		bool bChanged = Units.Num() != InitialUnits.Num();
		for (int32 Index = 0; !bChanged && Index < Units.Num(); ++Index)
		{
			bChanged = Units[Index].ID != InitialUnits[Index].ID || Units[Index].Position != InitialUnits[Index].Position;
		}

		if (bChanged)
		{
			// Undo must restore shot order as well as positions.
			TArray<FCRRecoilUnit> FinalUnits = MoveTemp(Units);
			Units = InitialUnits;
			FScopedTransaction Transaction(Description);
			UnitGraph->Modify();
			Units = MoveTemp(FinalUnits);
		}
	}
}

FCRUnitGraphScaleUnitsDelayedDrag::FCRUnitGraphScaleUnitsDelayedDrag(UCRRecoilUnitGraph* UnitGraph, const FCRRecoilUnitSelection& UnitSelection, const FVector2f InInitialRecoilLocation, const FVector2f InInitialPosition, const FKey& InEffectiveKey)
{
	CachedUnitGraph = UnitGraph;
	CachedRecoilUnits = UnitGraph->GetRecoilUnits();
	InitialRecoilLocation = InInitialRecoilLocation;
	InitialPanelLocation = InInitialPosition;
}

FCRUnitGraphScaleUnitsDelayedDrag::~FCRUnitGraphScaleUnitsDelayedDrag()
{
	CommitDraggedUnits(CachedUnitGraph.Get(), CachedRecoilUnits, NSLOCTEXT("CRUnitGraphScaleUnitsDelayedDrag", "DragOperation", "Scale recoil units"));
}

void FCRUnitGraphScaleUnitsDelayedDrag::Cancel()
{
	if (UCRRecoilUnitGraph* UnitGraph = CachedUnitGraph.Get())
	{
		UnitGraph->GetRecoilUnits() = CachedRecoilUnits;
	}
	CachedUnitGraph.Reset();
}

void FCRUnitGraphScaleUnitsDelayedDrag::ApplyScaling(const FCRRecoilUnitSelection& RecoilUnitSelection, float NewScale)
{
	UCRRecoilUnitGraph* UnitGraph = CachedUnitGraph.Get();
	if (!UnitGraph || !FMath::IsFinite(NewScale))
	{
		return;
	}

	NewScale = FMath::Max(0.05f, NewScale);
	const float ScaleFactor = NewScale / CurrentScale;
	if (!FMath::IsFinite(ScaleFactor))
	{
		return;
	}

	TArray<FCRRecoilUnit*> SelectedRecoilUnitPtrs = RecoilUnitSelection.GetSelectedRecoilUnits(UnitGraph);
	for (int32 Index = 1; Index < SelectedRecoilUnitPtrs.Num(); ++Index)
	{
		const FVector2f NewPosition = InitialRecoilLocation + (SelectedRecoilUnitPtrs[Index]->Position - InitialRecoilLocation) * ScaleFactor;
		if (!FMath::IsFinite(NewPosition.X) || !FMath::IsFinite(NewPosition.Y))
		{
			return;
		}
	}

	for (int32 Index = 1; Index < SelectedRecoilUnitPtrs.Num(); ++Index)
	{
		FVector2f VectorFromInitialToUnit = SelectedRecoilUnitPtrs[Index]->Position - InitialRecoilLocation;
		VectorFromInitialToUnit *= ScaleFactor;
		SelectedRecoilUnitPtrs[Index]->Position = InitialRecoilLocation + VectorFromInitialToUnit;
	}

	CurrentScale = NewScale;
}

FCRUnitGraphMoveUnitsDelayedDrag::FCRUnitGraphMoveUnitsDelayedDrag(UCRRecoilUnitGraph* UnitGraph, const FCRRecoilUnitSelection& UnitSelection, const FVector2f InInitialRecoilLocation, const FVector2f InInitialPosition, const FKey& InEffectiveKey)
	: FDelayedDrag(static_cast<FVector2D>(InInitialPosition), InEffectiveKey)
{
	TriggerDistance = 0.f;
	CachedUnitGraph = UnitGraph;
	CachedRecoilUnits = UnitGraph->GetRecoilUnits();
	LastRecoilCoordsLocation = InInitialRecoilLocation;
}

FCRUnitGraphMoveUnitsDelayedDrag::~FCRUnitGraphMoveUnitsDelayedDrag()
{
	CommitDraggedUnits(CachedUnitGraph.Get(), CachedRecoilUnits, NSLOCTEXT("CRUnitGraphMoveUnitsDelayedDrag", "DragOperation", "Move recoil units"));
}

void FCRUnitGraphMoveUnitsDelayedDrag::Cancel()
{
	if (UCRRecoilUnitGraph* UnitGraph = CachedUnitGraph.Get())
	{
		UnitGraph->GetRecoilUnits() = CachedRecoilUnits;
	}
	CachedUnitGraph.Reset();
}

void FCRUnitGraphMoveUnitsDelayedDrag::ApplyMovement(const FCRRecoilUnitSelection& UnitSelection, const FVector2f& Movement) const
{
	UCRRecoilUnitGraph* UnitGraph = CachedUnitGraph.Get();
	if (!UnitGraph)
	{
		return;
	}

	for (FCRRecoilUnit* RecoilUnit : UnitSelection.GetSelectedRecoilUnits(UnitGraph))
	{
		if (RecoilUnit)
		{
			RecoilUnit->Position += Movement;
		}
	}
}
