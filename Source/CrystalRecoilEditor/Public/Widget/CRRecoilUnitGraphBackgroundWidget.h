// Copyright CrystalVapor 2026, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SNodePanel.h"

struct FCRZoomLevelEntry
{
	explicit FCRZoomLevelEntry(const float InZoomAmount, const FText& InDisplayText, const EGraphRenderingLOD::Type InLOD)
		: DisplayText(FText::Format(NSLOCTEXT("GraphEditor", "Zoom", "Zoom {0}"), InDisplayText))
		, ZoomAmount(InZoomAmount)
		, LOD(InLOD)
	{
	}

	FText DisplayText;

	float ZoomAmount;

	EGraphRenderingLOD::Type LOD;
};

struct FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer : public FZoomLevelsContainer
{
	FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer();

	virtual float GetZoomAmount(int32 InZoomLevel) const override;

	virtual int32 GetNearestZoomLevel(float ZoomAmount) const override;

	virtual FText GetZoomText(int32 InZoomLevel) const override;

	virtual int32 GetNumZoomLevels() const override;

	virtual int32 GetDefaultZoomLevel() const override;

	virtual EGraphRenderingLOD::Type GetLOD(int32 InZoomLevel) const override;

private:
	TArray<FCRZoomLevelEntry> ZoomLevels;
};

class CRYSTALRECOILEDITOR_API SCRRecoilUnitGraphBackgroundWidget : public SNodePanel
{
public:
	SLATE_BEGIN_ARGS(SCRRecoilUnitGraphBackgroundWidget){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	using SNodePanel::GraphCoordToPanelCoord;

	using SNodePanel::PanelCoordToGraphCoord;

	using SNodePanel::ScrollToLocation;

	using SNodePanel::ChangeZoomLevel;

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// Returns the view offset that is zoomed with zoom amount, and based in the center of the view
	FVector2D GetZoomedAndCenterBasedViewOffset() const;

	int32 GetGridAxisStep() const;
};
