// Copyright CrystalVapor 2026, All rights reserved.

#include "Data/CRRecoilUnitGraph.h"

const FCRRecoilUnit& UCRRecoilUnitGraph::GetUnitAt(const int32 Index) const
{
	return RecoilUnits[Index];
}

int32 UCRRecoilUnitGraph::GetUnitCount() const
{
	return RecoilUnits.Num();
}

#if WITH_EDITOR
int32 UCRRecoilUnitGraph::AddUnit(const FVector2f& RecoilUnitLocation)
{
	RecoilUnits.Add(FCRRecoilUnit(NextID++, RecoilUnitLocation));
	return NextID - 1;
}

void UCRRecoilUnitGraph::RemoveUnit(const uint32 ID)
{
	RecoilUnits.RemoveAll([ID](const FCRRecoilUnit& Unit) { return Unit.ID == ID; });

	if (RecoilUnits.Num() == 0)
	{
		NextID = 0;
	}
}

FCRRecoilUnit* UCRRecoilUnitGraph::GetUnitByID(uint32 ID)
{
	return RecoilUnits.FindByPredicate([ID](const FCRRecoilUnit& Unit) { return Unit.ID == ID; });
}

TArray<FCRRecoilUnit>& UCRRecoilUnitGraph::GetRecoilUnits()
{
	return RecoilUnits;
}

void UCRRecoilUnitGraph::RearrangeUnits()
{
	const bool bByY = RearrangePolicy == ECRRecoilUnitGraphRearrangePolicy::AscendByY || RearrangePolicy == ECRRecoilUnitGraphRearrangePolicy::DescendByY;
	const bool bAscend = RearrangePolicy == ECRRecoilUnitGraphRearrangePolicy::AscendByY || RearrangePolicy == ECRRecoilUnitGraphRearrangePolicy::AscendByX;

	Algo::Sort(RecoilUnits, [bByY, bAscend](const FCRRecoilUnit& A, const FCRRecoilUnit& B)
	{
		const float ValueA = bByY ? A.Position.Y : A.Position.X;
		const float ValueB = bByY ? B.Position.Y : B.Position.X;

		if (ValueA != ValueB)
		{
			return bAscend ? ValueA < ValueB : ValueA > ValueB;
		}

		return A.ID < B.ID;
	});
}

void UCRRecoilUnitGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_STRING_VIEW_CHECKED(UCRRecoilUnitGraph, RecoilUnits))
	{
		if (RecoilUnits.Num() == 0)
		{
			NextID = 0;
			return;
		}

		// Process in reverse order to fix duplicate IDs
		// Example: [0, 1, 2, 3, 3] becomes [0, 1, 2, 3, 4]
		for (int32 i = RecoilUnits.Num() - 1; i >= 0; --i)
		{
			FCRRecoilUnit& Unit = RecoilUnits[i];

			// Check if this ID is a duplicate (appears earlier in the array)
			bool bIsDuplicate = false;
			for (int32 j = 0; j < i; ++j)
			{
				if (RecoilUnits[j].ID == Unit.ID)
				{
					bIsDuplicate = true;
					break;
				}
			}

			// If duplicate, assign a new unique ID
			if (bIsDuplicate)
			{
				Unit.ID = NextID++;
			}
		}

		// Ensure NextID is higher than all existing IDs
		for (const FCRRecoilUnit& Unit : RecoilUnits)
		{
			if (Unit.ID >= NextID)
			{
				NextID = Unit.ID + 1;
			}
		}
	}
}
#endif
