// Copyright CrystalVapor 2026, All rights reserved.

#include "Widget/CRRecoilUnitGraphBackgroundWidget.h"

FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer::FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer()
{
	ZoomLevels.Add(FCRZoomLevelEntry(0.250f, FText::FromString(TEXT("-6")), EGraphRenderingLOD::LowestDetail));
	ZoomLevels.Add(FCRZoomLevelEntry(0.500f, FText::FromString(TEXT("-4")), EGraphRenderingLOD::LowDetail));
	ZoomLevels.Add(FCRZoomLevelEntry(0.750f, FText::FromString(TEXT("-2")), EGraphRenderingLOD::MediumDetail));
	ZoomLevels.Add(FCRZoomLevelEntry(1.000f, FText::FromString(TEXT("1:1")), EGraphRenderingLOD::DefaultDetail));
	ZoomLevels.Add(FCRZoomLevelEntry(1.250f, FText::FromString(TEXT("+2")), EGraphRenderingLOD::DefaultDetail));
	ZoomLevels.Add(FCRZoomLevelEntry(1.500f, FText::FromString(TEXT("+4")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(1.750f, FText::FromString(TEXT("+6")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(2.000f, FText::FromString(TEXT("+8")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(2.250f, FText::FromString(TEXT("+10")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(2.500f, FText::FromString(TEXT("+12")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(2.750f, FText::FromString(TEXT("+14")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(3.000f, FText::FromString(TEXT("+16")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(3.250f, FText::FromString(TEXT("+18")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(3.500f, FText::FromString(TEXT("+20")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(3.750f, FText::FromString(TEXT("+22")), EGraphRenderingLOD::FullyZoomedIn));
	ZoomLevels.Add(FCRZoomLevelEntry(4.000f, FText::FromString(TEXT("+24")), EGraphRenderingLOD::FullyZoomedIn));
}

float FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer::GetZoomAmount(int32 InZoomLevel) const
{
	checkSlow(ZoomLevels.IsValidIndex(InZoomLevel));
	return ZoomLevels[InZoomLevel].ZoomAmount;
}

int32 FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer::GetNearestZoomLevel(float ZoomAmount) const
{
	for (int32 ZoomLevelIndex = 0; ZoomLevelIndex < GetNumZoomLevels(); ++ZoomLevelIndex)
	{
		if (ZoomAmount <= GetZoomAmount(ZoomLevelIndex))
		{
			return ZoomLevelIndex;
		}
	}

	return GetDefaultZoomLevel();
}

FText FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer::GetZoomText(int32 InZoomLevel) const
{
	checkSlow(ZoomLevels.IsValidIndex(InZoomLevel));
	return ZoomLevels[InZoomLevel].DisplayText;
}

int32 FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer::GetNumZoomLevels() const
{
	return ZoomLevels.Num();
}

int32 FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer::GetDefaultZoomLevel() const
{
	return 13;
}

EGraphRenderingLOD::Type FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer::GetLOD(int32 InZoomLevel) const
{
	checkSlow(ZoomLevels.IsValidIndex(InZoomLevel));
	return ZoomLevels[InZoomLevel].LOD;
}

void SCRRecoilUnitGraphBackgroundWidget::Construct(const FArguments& InArgs)
{
	// Use our custom zoom levels container
	ZoomLevels = MakeShared<FCRRecoilUnitGraphBackgroundWidgetZoomLevelsContainer>();
	SNodePanel::Construct();
}

int32 SCRRecoilUnitGraphBackgroundWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 ResultLayer = SNodePanel::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FSlateBrush* BackgroundBrush = FAppStyle::GetBrush(TEXT("Graph.Panel.SolidBackground"));
	PaintBackgroundAsLines(BackgroundBrush, AllottedGeometry, MyCullingRect, OutDrawElements, ResultLayer);
	return ResultLayer;
}

FVector2D SCRRecoilUnitGraphBackgroundWidget::GetZoomedAndCenterBasedViewOffset() const
{
	return GetViewOffset() * GetZoomAmount() + GetTickSpaceGeometry().GetLocalSize() * 0.5f;
}

int32 SCRRecoilUnitGraphBackgroundWidget::GetGridAxisStep() const
{
	return FMath::Max(1, FMath::CeilToInt(2.f / GetZoomAmount()));
}
