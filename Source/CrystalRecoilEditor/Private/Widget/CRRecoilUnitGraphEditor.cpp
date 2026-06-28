// Copyright CrystalVapor 2026, All rights reserved.

#include "Widget/CRRecoilUnitGraphEditor.h"
#include "ScopedTransaction.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Data/CRRecoilPattern.h"
#include "Data/CRRecoilUnitGraph.h"
#include "Editor/CRRecoilPatternEditor.h"
#include "Fonts/FontMeasure.h"
#include "Widget/CRRecoilUnitGraphBackgroundWidget.h"
#include "Misc/StringOutputDevice.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

void SCRRecoilUnitGraphWidget::Construct(const FArguments& InArgs)
{
    check(InArgs._RecoilPatternEditor)
    RecoilPatternEditor = InArgs._RecoilPatternEditor;

    static const FSlateRoundedBoxBrush KeyBorderBrush(FLinearColor(1.f, 1.f, 1.f, 0.07f), 4.f);
    static const FSlateRoundedBoxBrush PanelBackgroundBrush(FLinearColor(0.025f, 0.025f, 0.025f, 0.85f), 6.f);

    const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 9);
    const FSlateFontInfo TitleFontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 9);

    auto MakeShortcutRow = [&](const TSharedRef<SWidget>& KeyWidget, const FString& Description) -> TSharedRef<SWidget>
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .MinWidth(60.f)
            .MaxWidth(60.f)
            .VAlign(VAlign_Center)
            [
                SNew(SBorder)
                .BorderImage(&KeyBorderBrush)
                .Padding(FMargin(4.f, 2.f))
                .HAlign(HAlign_Center)
                [
                    KeyWidget
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(8.f, 0.f, 0.f, 0.f)
            [
                SNew(STextBlock)
                .Font(FontInfo)
                .Text(FText::FromString(Description))
            ];
    };

    auto MakeKeyText = [&](const FString& Key) -> TSharedRef<SWidget>
    {
        return SNew(STextBlock).Font(TitleFontInfo).Text(FText::FromString(Key));
    };

    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        [
            SAssignNew(BackgroundWidget, SCRRecoilUnitGraphBackgroundWidget)
            .Visibility(EVisibility::HitTestInvisible)
        ]
        + SOverlay::Slot()
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Bottom)
        [
            SNew(SOverlay)
            + SOverlay::Slot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            [
                SNew(SBorder)
                .BorderImage(&PanelBackgroundBrush)
                .Visibility_Lambda([this]()
                {
                    return RecoilPatternEditor->bShowShortcuts ? EVisibility::Visible : EVisibility::Collapsed;
                })
            ]
            + SOverlay::Slot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            .Padding(8.f)
            [
                SAssignNew(ShortcutsContainer, SVerticalBox)
                .Visibility_Lambda([this]()
                {
                    return RecoilPatternEditor->bShowShortcuts ? EVisibility::Visible : EVisibility::Collapsed;
                })
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 6.f)
                [
                    SNew(STextBlock)
                    .Font(TitleFontInfo)
                    .Text(FText::FromString(TEXT("Graph Shortcuts")))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 2.f)
                [
                    MakeShortcutRow(
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Font(TitleFontInfo)
                            .Text(FText::FromString(TEXT("Shift + ")))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(SImage)
                            .Image(FAppStyle::Get().GetBrush("GraphEditor.MouseEvent_16x"))
                            .DesiredSizeOverride(FVector2D(12.f, 12.f))
                        ],
                        TEXT("Add Unit"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 2.f)
                [
                    MakeShortcutRow(MakeKeyText(TEXT("Shift + S")), TEXT("Toggle Snapping"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 2.f)
                [
                    MakeShortcutRow(MakeKeyText(TEXT("S")), TEXT("Scale"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 2.f)
                [
                    MakeShortcutRow(MakeKeyText(TEXT("R")), TEXT("Auto Rearrange"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 2.f)
                [
                    MakeShortcutRow(MakeKeyText(TEXT("F")), TEXT("Zoom to Fit"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 2.f)
                [
                    MakeShortcutRow(MakeKeyText(TEXT("H")), TEXT("Toggle Shortcuts"))
                ]
            ]
        ]
    ];
}

void SCRRecoilUnitGraphWidget::SetRecoilUnitGraph(UCRRecoilUnitGraph* InRecoilUnitGraph)
{
	RecoilUnitGraph = InRecoilUnitGraph;

	// Trigger zoom to fit when opening
	bNeedZoomToFit = true;
}

void SCRRecoilUnitGraphWidget::ZoomToFitAllUnits() const
{
	UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	if (!CurrentRecoilUnitGraph || CurrentRecoilUnitGraph->GetUnitCount() == 0)
	{
		BackgroundWidget->SetViewOffset(BackgroundWidget->GetTickSpaceGeometry().GetLocalSize() * -0.5f / BackgroundWidget->GetZoomAmount());
		return;
	}

	// Find bounds of all units in recoil space, always including origin (0,0)
	FVector2f MinBounds = FVector2f::ZeroVector;
	FVector2f MaxBounds = FVector2f::ZeroVector;

	for (int32 Index = 0; Index < CurrentRecoilUnitGraph->GetUnitCount(); ++Index)
	{
		const FCRRecoilUnit& Unit = CurrentRecoilUnitGraph->GetUnitAt(Index);
		MinBounds.X = FMath::Min(MinBounds.X, Unit.Position.X);
		MinBounds.Y = FMath::Min(MinBounds.Y, Unit.Position.Y);
		MaxBounds.X = FMath::Max(MaxBounds.X, Unit.Position.X);
		MaxBounds.Y = FMath::Max(MaxBounds.Y, Unit.Position.Y);
	}

	// Convert to graph space and resolve min/max since Y is flipped
	const FVector2f MinGraphCandidate = RecoilCoordsToGraphCoords(MinBounds);
	const FVector2f MaxGraphCandidate = RecoilCoordsToGraphCoords(MaxBounds);
	const FVector2f MinGraph = FVector2f(FMath::Min(MinGraphCandidate.X, MaxGraphCandidate.X), FMath::Min(MinGraphCandidate.Y, MaxGraphCandidate.Y));
	const FVector2f MaxGraph = FVector2f(FMath::Max(MinGraphCandidate.X, MaxGraphCandidate.X), FMath::Max(MinGraphCandidate.Y, MaxGraphCandidate.Y));

	// Add padding for visual clearance
	const FVector2f SizeGraph = MaxGraph - MinGraph;
	const float UnitDrawSize = CurrentRecoilUnitGraph->UnitDrawSize;
	const FVector2f Padding = FVector2f(UnitDrawSize * 3.f + SizeGraph.X * 0.04f, UnitDrawSize * 3.f + SizeGraph.Y * 0.04f);
	const FVector2f PaddedSizeGraph = SizeGraph + Padding * 2.f;
	const FVector2f CenterGraph = (MinGraph + MaxGraph) * 0.5f;

	// Find closest zoom level to fit all units
	const FVector2f WidgetSize = BackgroundWidget->GetTickSpaceGeometry().GetLocalSize();
	const float TargetZoom = FMath::Min(WidgetSize.X / PaddedSizeGraph.X, WidgetSize.Y / PaddedSizeGraph.Y);

	for (int32 Index = 0; Index < 40; ++Index)
	{
		BackgroundWidget->ChangeZoomLevel(-1, FVector2f::ZeroVector, false);
	}

	float SmallestDifference = FMath::Abs(BackgroundWidget->GetZoomAmount() - TargetZoom);
	for (int32 Index = 0; Index < 40; ++Index)
	{
		BackgroundWidget->ChangeZoomLevel(1, FVector2f::ZeroVector, false);
		const float Difference = FMath::Abs(BackgroundWidget->GetZoomAmount() - TargetZoom);
		if (Difference <= SmallestDifference)
		{
			SmallestDifference = Difference;
		}
		else
		{
			BackgroundWidget->ChangeZoomLevel(-1, FVector2f::ZeroVector, false);
			break;
		}
	}

	if (BackgroundWidget->GetZoomAmount() > TargetZoom)
	{
		BackgroundWidget->ChangeZoomLevel(-1, FVector2f::ZeroVector, false);
	}

	const float ActualZoom = BackgroundWidget->GetZoomAmount();
	const FVector2f ViewportCenter = WidgetSize * 0.5f;
	BackgroundWidget->SetViewOffset(FVector2f(CenterGraph.X - ViewportCenter.X / ActualZoom, CenterGraph.Y - ViewportCenter.Y / ActualZoom));
}

int32 SCRRecoilUnitGraphWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (bNeedZoomToFit)
	{
		// Check if widget geometry is ready
		const FVector2f WidgetSize = BackgroundWidget->GetTickSpaceGeometry().GetLocalSize();
		if (WidgetSize.X > 0 && WidgetSize.Y > 0)
		{
			ZoomToFitAllUnits();
			bNeedZoomToFit = false;
		}
		// else: keep bNeedZoomToFit = true and try again next frame
	}

	SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	DrawOrigin(OutDrawElements, AllottedGeometry, LayerId);
	DrawRecoilUnits(OutDrawElements, AllottedGeometry, LayerId);
	DrawGridAxisNumbers(OutDrawElements, AllottedGeometry, LayerId);
	DrawSelectionBox(OutDrawElements, AllottedGeometry, LayerId);
	return LayerId;
}

FReply SCRRecoilUnitGraphWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SelectionDrag.Reset();

	const FVector2f MousePanelLocation = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		ViewDrag = FCRUnitGraphViewDelayedDrag(MousePanelLocation, EKeys::RightMouseButton);
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (MouseEvent.IsShiftDown())
		{
			AddUnitUnderCursor();
			return FReply::Handled();
		}

		LastLeftMouseDownFoundUnitID = FindUnitByScreenLocation(MousePanelLocation);
		FCRRecoilUnitSelection& CurrentRecoilUnitSelection = GetRecoilUnitSelection();
		if (LastLeftMouseDownFoundUnitID == INDEX_NONE)
		{
			CurrentRecoilUnitSelection.ClearSelection();
			SelectionDrag = FCRUnitGraphSelectionDelayedDrag(MousePanelLocation, EKeys::LeftMouseButton);
			MoveUnitsDrag.Reset();
		}
		else
		{
			if (!MouseEvent.IsControlDown() && !CurrentRecoilUnitSelection.IsUnitSelected(LastLeftMouseDownFoundUnitID))
			{
				CurrentRecoilUnitSelection.ClearSelection();
			}

			CurrentRecoilUnitSelection.AddSelection(LastLeftMouseDownFoundUnitID);
			MoveUnitsDrag = FCRUnitGraphMoveUnitsDelayedDrag(GetUnitGraph(), CurrentRecoilUnitSelection, GetUnitGraph()->GetUnitByID(LastLeftMouseDownFoundUnitID)->Position, MousePanelLocation, EKeys::LeftMouseButton);
		}
	}

	return FReply::Handled();
}

FReply SCRRecoilUnitGraphWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (ViewDrag.IsSet() && ViewDrag->IsDragging())
		{
			ViewDrag.Reset();
		}
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bool bLeftMouseUpWithNoDrag = true;

		if (SelectionDrag.IsSet() && SelectionDrag->IsDragging())
		{
			SelectionDrag.Reset();
			bLeftMouseUpWithNoDrag = false;
		}

		if (MoveUnitsDrag.IsSet() && MoveUnitsDrag->IsDragging())
		{
			MoveUnitsDrag.Reset();
			bLeftMouseUpWithNoDrag = false;
		}

		if (bLeftMouseUpWithNoDrag && LastLeftMouseDownFoundUnitID == INDEX_NONE)
		{
			if (!MouseEvent.IsControlDown())
			{
				FCRRecoilUnitSelection& CurrentRecoilUnitSelection = GetRecoilUnitSelection();
				CurrentRecoilUnitSelection.ClearSelection();
			}
		}
	}

	return FReply::Handled();
}

FReply SCRRecoilUnitGraphWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	CurrentMousePanelPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if (ViewDrag.IsSet() && (ViewDrag->IsDragging() || ViewDrag->AttemptDragStart(MouseEvent)))
	{
		BackgroundWidget->SetViewOffset(BackgroundWidget->GetViewOffset() - MouseEvent.GetCursorDelta() / BackgroundWidget->GetZoomAmount());
	}

	if (SelectionDrag.IsSet() && (SelectionDrag->IsDragging() || SelectionDrag->AttemptDragStart(MouseEvent)))
	{
		SelectionDrag->EndPosition = CurrentMousePanelPosition;
		const FSlateRect SelectionRect = SelectionDrag->GetSelectionRect();
		SelectUnitsInPanelCoordsRect(SelectionRect);
	}

	if (MoveUnitsDrag.IsSet() && (MoveUnitsDrag->IsDragging() || MoveUnitsDrag->AttemptDragStart(MouseEvent)))
	{
		const FVector2f NewPanelLocation = CurrentMousePanelPosition;
		const FVector2f NewGraphLocation = BackgroundWidget->PanelCoordToGraphCoord(NewPanelLocation);
		const FVector2f NewRecoilLocation = GraphCoordsToRecoilCoords(NewGraphLocation);
		const FVector2f LastRecoilLocation = MoveUnitsDrag->LastRecoilCoordsLocation;

		const bool bEnableGridSnapping = RecoilPatternEditor->bEnableGridSnapping;
		const float GridSnapSize = RecoilPatternEditor->GridSnappingValue;
		const FCRRecoilUnitSelection& RecoilUnitSelection = GetRecoilUnitSelection();

		if (!bEnableGridSnapping)
		{
			const FVector2f DeltaRecoilLocation = NewRecoilLocation - LastRecoilLocation;
			MoveUnitsDrag->ApplyMovement(RecoilUnitSelection, DeltaRecoilLocation);
			MoveUnitsDrag->LastRecoilCoordsLocation = NewRecoilLocation;
		}
		else
		{
			UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
			const FCRRecoilUnit& LastRecoilUnit = *(CurrentRecoilUnitGraph->GetUnitByID(LastLeftMouseDownFoundUnitID));
			const FVector2f SnappedNewRecoilLocation = FVector2f(FMath::RoundToInt(NewRecoilLocation.X / GridSnapSize) * GridSnapSize, FMath::RoundToInt(NewRecoilLocation.Y / GridSnapSize) * GridSnapSize);
			const FVector2f DeltaRecoilLocation = SnappedNewRecoilLocation - LastRecoilUnit.Position;
			MoveUnitsDrag->ApplyMovement(RecoilUnitSelection, DeltaRecoilLocation);
			MoveUnitsDrag->LastRecoilCoordsLocation = SnappedNewRecoilLocation;
		}

		TryAutoRearrangeUnits();
		RecoilPatternEditor->RefreshUnitPosition();
	}

	if (ScaleUnitsDrag.IsSet())
	{
		const FVector2f NewPanelLocation = CurrentMousePanelPosition;
		const FVector2f InitialPanelLocation = ScaleUnitsDrag->InitialPanelLocation;
		const FVector2f ScaleVector = NewPanelLocation - InitialPanelLocation;
		const float NewScale = ScaleVector.Size() / ScaleUnitsDrag->NormalVectorSizePanel;
		const FCRRecoilUnitSelection& RecoilUnitSelection = GetRecoilUnitSelection();
		ScaleUnitsDrag->ApplyScaling(RecoilUnitSelection, NewScale);
	}

	return SCompoundWidget::OnMouseMove(MyGeometry, MouseEvent);
}

FReply SCRRecoilUnitGraphWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return BackgroundWidget->OnMouseWheel(MyGeometry, MouseEvent);
}

void SCRRecoilUnitGraphWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	if (ViewDrag.IsSet() && ViewDrag->IsDragging())
	{
		ViewDrag.Reset();
	}

	if (SelectionDrag.IsSet() && SelectionDrag->IsDragging())
	{
		SelectionDrag.Reset();
	}

	SCompoundWidget::OnMouseLeave(MouseEvent);
}

FReply SCRRecoilUnitGraphWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	return SCompoundWidget::OnKeyUp(MyGeometry, InKeyEvent);
}

void SCRRecoilUnitGraphWidget::DrawOrigin(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, const int32 BaseLayerID) const
{
	const UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	const FVector2f OriginPanelCoords = BackgroundWidget->GraphCoordToPanelCoord(FVector2f::ZeroVector);
	const FVector2f OriginDrawSize = FVector2f(CurrentRecoilUnitGraph->OriginDrawSize);
	const FVector2f OriginDrawOffset = OriginDrawSize * 0.5f;
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayerID + CrystalRecoilEditor::EEditorLayerOffset::OriginLayer, AllottedGeometry.ToPaintGeometry(OriginDrawSize, FSlateLayoutTransform(OriginPanelCoords - OriginDrawOffset)), FAppStyle::GetBrush("Plus"), ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(CurrentRecoilUnitGraph->OriginOpacity));
}

void SCRRecoilUnitGraphWidget::DrawRecoilUnits(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, int32 BaseLayerID) const
{
	const UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	const int32 UnitCount = CurrentRecoilUnitGraph->GetUnitCount();
	const FVector2f UnitDrawSize = FVector2f(CurrentRecoilUnitGraph->UnitDrawSize);
	const FCRRecoilUnitSelection& UnitSelection = GetRecoilUnitSelection();
	const int32 UnitLinesLayerID = BaseLayerID + CrystalRecoilEditor::EEditorLayerOffset::RecoilUnitsLayer;
	const int32 UnitLayerID = BaseLayerID + CrystalRecoilEditor::EEditorLayerOffset::RecoilUnitsLayer + 1;
	const int32 UnitNumberLayerID = BaseLayerID + CrystalRecoilEditor::EEditorLayerOffset::RecoilUnitNumbersLayer;
	const SCRRecoilUnitGraphBackgroundWidget* CurrentBackgroundWidget = this->BackgroundWidget.Get();
	const FSlateFontInfo NumberFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", CurrentRecoilUnitGraph->UnitNumbersFontSize);
	const bool bDrawUnitNumbers = CurrentRecoilUnitGraph->bDrawUnitNumbers && CurrentBackgroundWidget->GetCurrentLOD() >= EGraphRenderingLOD::MediumDetail;

	// First pass: Draw all lines
	for (int32 Index = 1; Index < UnitCount; ++Index)
	{
		const FCRRecoilUnit& RecoilUnit = CurrentRecoilUnitGraph->GetUnitAt(Index);
		const FCRRecoilUnit& PreviousRecoilUnit = CurrentRecoilUnitGraph->GetUnitAt(Index - 1);

		const FVector2f RecoilUnitCenterGraphLocation = RecoilCoordsToGraphCoords(RecoilUnit.Position);
		const FVector2f PreviousRecoilUnitCenterGraphLocation = RecoilCoordsToGraphCoords(PreviousRecoilUnit.Position);

		const FVector2f PreviousRecoilUnitCenterPanelLocation = CurrentBackgroundWidget->GraphCoordToPanelCoord(PreviousRecoilUnitCenterGraphLocation);
		const FVector2f RecoilUnitCenterPanelLocation = CurrentBackgroundWidget->GraphCoordToPanelCoord(RecoilUnitCenterGraphLocation);

		FSlateDrawElement::MakeLines(OutDrawElements, UnitLinesLayerID, AllottedGeometry.ToPaintGeometry(), {PreviousRecoilUnitCenterPanelLocation, RecoilUnitCenterPanelLocation}, ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(CurrentRecoilUnitGraph->UnitLinesOpacity), true, 1.f);
	}

	// Second pass: Draw all units
	for (int32 Index = 0; Index < UnitCount; ++Index)
	{
		const FCRRecoilUnit& RecoilUnit = CurrentRecoilUnitGraph->GetUnitAt(Index);
		const FVector2f RecoilUnitCenterGraphLocation = RecoilCoordsToGraphCoords(RecoilUnit.Position);
		const FVector2f RecoilUnitCenterPanelLocation = CurrentBackgroundWidget->GraphCoordToPanelCoord(RecoilUnitCenterGraphLocation);
		const FVector2f RecoilUnitDrawPanelLocation = RecoilUnitCenterPanelLocation - UnitDrawSize * 0.5f;
		const bool bIsSelected = UnitSelection.IsUnitSelected(RecoilUnit.ID);
		const FLinearColor& UnitColor = bIsSelected ? FLinearColor(0.004f, 0.238f, 1.f) : FLinearColor::White;

		// Draw Unit
		FSlateDrawElement::MakeBox(OutDrawElements, UnitLayerID, AllottedGeometry.ToPaintGeometry(UnitDrawSize, FSlateLayoutTransform(RecoilUnitDrawPanelLocation)), FAppStyle::GetBrush("Icons.FilledCircle"), ESlateDrawEffect::None, UnitColor);

		// Draw Numbers centered inside units
		if (bDrawUnitNumbers)
		{
			const FString NumberString = FString::FromInt(Index);
			const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const FVector2f TextSize = FontMeasure->Measure(NumberString, NumberFontInfo);
			const FVector2f TextOffset = (UnitDrawSize - TextSize) * 0.5f;
			const FLinearColor NumberColor = bIsSelected ? FLinearColor::White : FLinearColor::Black;

			FSlateDrawElement::MakeText(OutDrawElements, UnitNumberLayerID, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(RecoilUnitDrawPanelLocation + TextOffset)), NumberString, NumberFontInfo, ESlateDrawEffect::None, NumberColor);
		}
	}

	// Draw position labels next to selected units while dragging
	if (CurrentRecoilUnitGraph->bDrawDragPositionLabels && MoveUnitsDrag.IsSet() && MoveUnitsDrag->IsDragging())
	{
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FSlateFontInfo LabelFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 9);
		const int32 DragLabelLayerID = BaseLayerID + CrystalRecoilEditor::EEditorLayerOffset::DragPositionLabelLayer;
		const float LabelOffsetX = UnitDrawSize.X * 0.5f + 6.f;

		for (int32 Index = 0; Index < UnitCount; ++Index)
		{
			const FCRRecoilUnit& RecoilUnit = CurrentRecoilUnitGraph->GetUnitAt(Index);
			if (!UnitSelection.IsUnitSelected(RecoilUnit.ID))
			{
				continue;
			}

			const FVector2f UnitCenterPanelLocation = CurrentBackgroundWidget->GraphCoordToPanelCoord(RecoilCoordsToGraphCoords(RecoilUnit.Position));
			const FString LabelText = FString::Printf(TEXT("%.2f, %.2f"), RecoilUnit.Position.X, RecoilUnit.Position.Y);
			const FVector2f LabelSize = FontMeasure->Measure(LabelText, LabelFontInfo);
			const FVector2f LabelPosition(UnitCenterPanelLocation.X + LabelOffsetX, UnitCenterPanelLocation.Y - LabelSize.Y * 0.5f);

			FSlateDrawElement::MakeText(OutDrawElements, DragLabelLayerID, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(LabelPosition)), LabelText, LabelFontInfo, ESlateDrawEffect::None, FLinearColor::Gray);
		}
	}
}

void SCRRecoilUnitGraphWidget::DrawGridAxisNumbers(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, const int32 BaseLayerID) const
{
	const FVector2D LocalSize = GetTickSpaceGeometry().GetLocalSize();
	const int32 GridAxisStep = BackgroundWidget->GetGridAxisStep();
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo NumberFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	const float Zoom = BackgroundWidget->GetZoomAmount();
	const FVector2D ViewOffset = BackgroundWidget->GetViewOffset();

	// Compute the exact visible grid number range from the viewport bounds - panel X = [0, W] maps to graph X = [ViewOffset.X, ViewOffset.X + W / Zoom]
	const int32 XGridStart = FMath::FloorToInt(ViewOffset.X / ScaleFromRecoilCoordsToGraphCoords / GridAxisStep) * GridAxisStep;
	const int32 XGridEnd = FMath::CeilToInt((ViewOffset.X + LocalSize.X / Zoom) / ScaleFromRecoilCoordsToGraphCoords / GridAxisStep) * GridAxisStep;
	const int32 YGridStart = FMath::FloorToInt(ViewOffset.Y / ScaleFromRecoilCoordsToGraphCoords / GridAxisStep) * GridAxisStep;
	const int32 YGridEnd = FMath::CeilToInt((ViewOffset.Y + LocalSize.Y / Zoom) / ScaleFromRecoilCoordsToGraphCoords / GridAxisStep) * GridAxisStep;

	for (int32 GridNum = XGridStart; GridNum <= XGridEnd; GridNum += GridAxisStep)
	{
		DrawSingleGridAxisNumber(GridNum, true, NumberFontInfo, FontMeasure, AllottedGeometry, OutDrawElements, BaseLayerID);
	}

	for (int32 GridNum = YGridStart; GridNum <= YGridEnd; GridNum += GridAxisStep)
	{
		DrawSingleGridAxisNumber(GridNum, false, NumberFontInfo, FontMeasure, AllottedGeometry, OutDrawElements, BaseLayerID);
	}
}

void SCRRecoilUnitGraphWidget::DrawSingleGridAxisNumber(const int32 GridLineNumber, const bool bXAxis, const FSlateFontInfo& NumberFontInfo, const TSharedRef<FSlateFontMeasure>& FontMeasure, const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, const int32 BaseLayerID) const
{
	const FVector2d GridLineNumberGraphCoords = bXAxis ? FVector2d(GridLineNumber * ScaleFromRecoilCoordsToGraphCoords, 0.f) : FVector2d(0.f, GridLineNumber * ScaleFromRecoilCoordsToGraphCoords);
	FVector2f GridLineNumberPanelCoords = BackgroundWidget->GraphCoordToPanelCoord(GridLineNumberGraphCoords);

	if (bXAxis)
	{
		GridLineNumberPanelCoords.Y = 0.f;
	}
	else
	{
		GridLineNumberPanelCoords.X = 0.f;
	}

	const FSlateRect SafeZoneRect(-50.f, -50.f, 10.f, 10.f);

	// The tick may be overlapped, skip them.
	if (SafeZoneRect.ContainsPoint(GridLineNumberPanelCoords))
	{
		return;
	}

	// Negate Y-axis numbers so positive values appear at the top (like a math graph),
	// making upward recoil display as positive values instead of using screen coordinates
	const FString GridTickText = bXAxis ? FString::FromInt(GridLineNumber) : FString::FromInt(-GridLineNumber);
	const FLinearColor GridTickTextColor = GridLineNumber == 0 ? FLinearColor::White : FLinearColor::White.CopyWithNewOpacity(0.65f);
	const FVector2d GridTickTextSize = FontMeasure->Measure(GridTickText, NumberFontInfo);
	FVector2f GridTickDrawnPosition = GridLineNumberPanelCoords;

	if (bXAxis)
	{
		GridTickDrawnPosition.X -= GridTickTextSize.X * 0.5f;
	}
	else
	{
		GridTickDrawnPosition.Y -= GridTickTextSize.Y * 0.5f;
	}

	const FSlateLayoutTransform TextTransform = FSlateLayoutTransform(GridTickDrawnPosition);
	const FPaintGeometry TextPaintGeometry = AllottedGeometry.ToPaintGeometry(GridTickTextSize, TextTransform);
	FSlateDrawElement::MakeText(OutDrawElements, BaseLayerID + CrystalRecoilEditor::EEditorLayerOffset::GridCoordinateLayer, TextPaintGeometry, GridTickText, NumberFontInfo, ESlateDrawEffect::None, GridTickTextColor);
}

void SCRRecoilUnitGraphWidget::DrawSelectionBox(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, const int32 BaseLayerID) const
{
	if (!SelectionDrag.IsSet() || !SelectionDrag->IsDragging())
	{
		return;
	}

	const FSlateRect SelectionBoxRect = SelectionDrag->GetSelectionRect();
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayerID + CrystalRecoilEditor::EEditorLayerOffset::RecoilUnitSelectionBoxLayer, AllottedGeometry.ToPaintGeometry(SelectionBoxRect.GetSize(), FSlateLayoutTransform(SelectionBoxRect.GetTopLeft())), FAppStyle::GetBrush(TEXT("MarqueeSelection")));
}

void SCRRecoilUnitGraphWidget::AddUnitUnderCursor() const
{
	UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	FScopedTransaction Transaction(NSLOCTEXT("SCRRecoilUnitGraphWidget", "AddUnitUnderCursor", "Add Unit Under Cursor"));
	CurrentRecoilUnitGraph->Modify();

	const FVector2f CurrentMouseRecoilLocation = GraphCoordsToRecoilCoords(BackgroundWidget->PanelCoordToGraphCoord(CurrentMousePanelPosition));
	CurrentRecoilUnitGraph->AddUnit(CurrentMouseRecoilLocation);

	TryAutoRearrangeUnits();
}

void SCRRecoilUnitGraphWidget::AddUnit(const FVector2f& RecoilLocation) const
{
	UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	FScopedTransaction Transaction(NSLOCTEXT("SCRRecoilUnitGraphWidget", "AddUnit", "Add Unit"));
	CurrentRecoilUnitGraph->Modify();
	CurrentRecoilUnitGraph->AddUnit(RecoilLocation);
}

void SCRRecoilUnitGraphWidget::CopySelectedUnits() const
{
	const FCRRecoilUnitSelection& CurrentUnitSelection = GetRecoilUnitSelection();
	UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	FCRRecoilUnitClipboardData CopiedData;

	for (const FCRRecoilUnit* Unit : CurrentUnitSelection.GetSelectedRecoilUnits(CurrentRecoilUnitGraph))
	{
		CopiedData.RecoilUnitLocations.Add(Unit->Position);
	}

	FString ExportedString;
	FCRRecoilUnitClipboardData::StaticStruct()->ExportText(ExportedString, &CopiedData, nullptr, nullptr, PPF_None, nullptr);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedString);
}

void SCRRecoilUnitGraphWidget::PasteUnits() const
{
	FCRRecoilUnitClipboardData CopiedData;
	FString CopiedString;
	FPlatformApplicationMisc::ClipboardPaste(CopiedString);
	if (!CopiedData.ImportFromString(CopiedString))
	{
		return;
	}

	UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	const FVector2f CurrentMouseRecoilLocation = GraphCoordsToRecoilCoords(BackgroundWidget->PanelCoordToGraphCoord(CurrentMousePanelPosition));
	FScopedTransaction Transaction(NSLOCTEXT("SCRRecoilUnitGraphWidget", "PasteUnits", "Paste Units"));
	CurrentRecoilUnitGraph->Modify();

	for (const FVector2f& Location : CopiedData.RecoilUnitLocations)
	{
		CurrentRecoilUnitGraph->AddUnit(Location + CurrentMouseRecoilLocation);
	}

	TryAutoRearrangeUnits();
}

void SCRRecoilUnitGraphWidget::DeleteUnits() const
{
	FCRRecoilUnitSelection& CurrentUnitSelection = GetRecoilUnitSelection();
	UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	FScopedTransaction Transaction(NSLOCTEXT("SCRRecoilUnitGraphWidget", "DeleteUnits", "Delete Units"));
	CurrentRecoilUnitGraph->Modify();

	TArray<int32> SelectedUnitIDs = CurrentUnitSelection.GetSelection();
	const int32 UnitSelectionCount = SelectedUnitIDs.Num();

	for (int32 Index = 0; Index < UnitSelectionCount; ++Index)
	{
		CurrentRecoilUnitGraph->RemoveUnit(SelectedUnitIDs[Index]);
	}

	CurrentUnitSelection.ClearSelection();
}

void SCRRecoilUnitGraphWidget::StartUnitScaling()
{
	TArray<FCRRecoilUnit*> SelectedUnits = GetRecoilUnitSelection().GetSelectedRecoilUnits(GetUnitGraph());
	FVector2f AverageLocation = FVector2f::ZeroVector;
	const int32 Num = SelectedUnits.Num();

	for (int32 Index = 0; Index < Num; ++Index)
	{
		AverageLocation += SelectedUnits[Index]->Position;
	}

	AverageLocation /= Num;

	const FVector2f CurrentMouseRecoilPosition = GraphCoordsToRecoilCoords(BackgroundWidget->PanelCoordToGraphCoord(CurrentMousePanelPosition));
	const FVector2f MidRecoilLocation = (AverageLocation + CurrentMouseRecoilPosition) * 0.5f;
	const FVector2f MidPanelLocation = BackgroundWidget->GraphCoordToPanelCoord(RecoilCoordsToGraphCoords(MidRecoilLocation));
	ScaleUnitsDrag = FCRUnitGraphScaleUnitsDelayedDrag(GetUnitGraph(), GetRecoilUnitSelection(), MidRecoilLocation, MidPanelLocation, EKeys::LeftMouseButton);
	ScaleUnitsDrag->NormalVectorSizePanel = (MidPanelLocation - CurrentMousePanelPosition).Size();
}

void SCRRecoilUnitGraphWidget::StopUnitScaling()
{
	if (ScaleUnitsDrag.IsSet())
	{
		ScaleUnitsDrag.Reset();
	}
}

UCRRecoilUnitGraph* SCRRecoilUnitGraphWidget::GetUnitGraph() const
{
	return GetRecoilPattern()->GetUnitGraph();
}

UCRRecoilPattern* SCRRecoilUnitGraphWidget::GetRecoilPattern() const
{
	return RecoilPatternEditor->GetRecoilPattern();
}

FCRRecoilUnitSelection& SCRRecoilUnitGraphWidget::GetRecoilUnitSelection() const
{
	return RecoilPatternEditor->GetRecoilUnitSelection();
}

FVector2f SCRRecoilUnitGraphWidget::RecoilCoordsToGraphCoords(FVector2f RecoilCoords) const
{
	RecoilCoords.Y *= -1.f;
	RecoilCoords *= ScaleFromRecoilCoordsToGraphCoords;
	return RecoilCoords;
}

FVector2f SCRRecoilUnitGraphWidget::GraphCoordsToRecoilCoords(FVector2f GraphCoords) const
{
	GraphCoords.Y *= -1.f;
	GraphCoords /= ScaleFromRecoilCoordsToGraphCoords;
	return GraphCoords;
}

int32 SCRRecoilUnitGraphWidget::FindUnitByScreenLocation(const FVector2f& ScreenLocation) const
{
	const UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	const int32 UnitCount = CurrentRecoilUnitGraph->GetUnitCount();
	const FVector2f UnitDrawSize = FVector2f(CurrentRecoilUnitGraph->UnitDrawSize);

	for (int32 Index = 0; Index < UnitCount; ++Index)
	{
		const FCRRecoilUnit& RecoilUnit = CurrentRecoilUnitGraph->GetUnitAt(Index);
		const FVector2f RecoilUnitCenterGraphLocation = RecoilCoordsToGraphCoords(RecoilUnit.Position);
		const FVector2f RecoilUnitCenterPanelLocation = BackgroundWidget->GraphCoordToPanelCoord(RecoilUnitCenterGraphLocation);
		const FSlateRect UnitRect(RecoilUnitCenterPanelLocation - UnitDrawSize * 0.5f, RecoilUnitCenterPanelLocation + UnitDrawSize * 0.5f);

		if (UnitRect.ContainsPoint(ScreenLocation))
		{
			return RecoilUnit.ID;
		}
	}
	return INDEX_NONE;
}

void SCRRecoilUnitGraphWidget::SelectUnitsInPanelCoordsRect(const FSlateRect& SelectionRect) const
{
	const UCRRecoilUnitGraph* CurrentRecoilUnitGraph = GetUnitGraph();
	FCRRecoilUnitSelection& UnitSelection = GetRecoilUnitSelection();

	for (int32 Index = 0; Index < CurrentRecoilUnitGraph->GetUnitCount(); ++Index)
	{
		const FCRRecoilUnit& RecoilUnit = CurrentRecoilUnitGraph->GetUnitAt(Index);
		const FVector2f RecoilUnitCenterGraphLocation = RecoilCoordsToGraphCoords(RecoilUnit.Position);
		const FVector2f RecoilUnitCenterPanelLocation = BackgroundWidget->GraphCoordToPanelCoord(RecoilUnitCenterGraphLocation);

		if (SelectionRect.ContainsPoint(RecoilUnitCenterPanelLocation))
		{
			UnitSelection.AddSelection(RecoilUnit.ID);
		}
		else
		{
			UnitSelection.RemoveSelection(RecoilUnit.ID);
		}
	}
}

void SCRRecoilUnitGraphWidget::TryAutoRearrangeUnits() const
{
	if (RecoilPatternEditor && RecoilUnitGraph.IsValid() && RecoilPatternEditor->bEnableAutoRearrangeUnits)
	{
		RecoilUnitGraph->RearrangeUnits();
	}
}

bool FCRRecoilUnitClipboardData::ImportFromString(const FString& ImportString)
{
	FStringOutputDevice Errors;
	StaticStruct()->ImportText(*ImportString, this, nullptr, 0, &Errors, StaticStruct()->GetName(), true);
	return Errors.IsEmpty();
}
