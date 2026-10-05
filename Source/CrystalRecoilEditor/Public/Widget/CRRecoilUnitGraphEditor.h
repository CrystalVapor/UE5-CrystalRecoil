// Copyright CrystalVapor 2026, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/CRRecoilUnitGraphWidgetDragOperations.h"
#include "CRRecoilUnitGraphEditor.generated.h"

class FCRRecoilUnitSelection;
class UCRRecoilPattern;
class SCRRecoilUnitGraphBackgroundWidget;
class FCRRecoilPatternEditor;
class UTransBuffer;
struct FTransactionContext;

namespace CrystalRecoilEditor
{
	enum EEditorLayerOffset
	{
		Base = 10000,
		OriginLayer,
		RecoilUnitsLayer,
		SelectedRecoilUnitsLayer,
		RecoilUnitSelectionBoxLayer,
		RecoilUnitNumbersLayer,
		GridCoordinateLayer,
		DragPositionLabelLayer,
	};
}

USTRUCT()
struct FCRRecoilUnitClipboardData
{
	GENERATED_BODY()

	bool ImportFromString(const FString& ImportString);

	UPROPERTY()
	TArray<FVector2f> RecoilUnitLocations;
};

class CRYSTALRECOILEDITOR_API SCRRecoilUnitGraphWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCRRecoilUnitGraphWidget){}
	SLATE_ARGUMENT(FCRRecoilPatternEditor*, RecoilPatternEditor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual ~SCRRecoilUnitGraphWidget() override;

	void SetRecoilUnitGraph(UCRRecoilUnitGraph* InRecoilUnitGraph);

	// SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual bool SupportsKeyboardFocus() const override { return true; }

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;

	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	// End of SWidget interface

	void DrawOrigin(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, const int32 BaseLayerID) const;

	void DrawRecoilUnits(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, int32 BaseLayerID) const;

	void DrawGridAxisNumbers(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, const int32 BaseLayerID) const;

	void DrawSingleGridAxisNumber(const int32 GridLineNumber, const bool bXAxis, const FSlateFontInfo& NumberFontInfo, const TSharedRef<FSlateFontMeasure>& FontMeasure, const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, const int32 BaseLayerID) const;

	void DrawSelectionBox(FSlateWindowElementList& OutDrawElements, const FGeometry& AllottedGeometry, const int32 BaseLayerID) const;

	void AddUnitUnderCursor();

	void AddUnit(const FVector2f& RecoilLocation);

	void ZoomToFitAllUnits() const;

	void CopySelectedUnits() const;

	void PasteUnits();

	void DeleteUnits();

	void FinishDragging();

	void CancelDragging();

	void StartUnitScaling();

	void StopUnitScaling();

protected:
	void OnBeforeUndoRedo(const FTransactionContext& TransactionContext);

	UCRRecoilUnitGraph* GetUnitGraph() const;

	UCRRecoilPattern* GetRecoilPattern() const;

	FCRRecoilUnitSelection& GetRecoilUnitSelection() const;

	FVector2f RecoilCoordsToGraphCoords(FVector2f RecoilCoords) const;

	FVector2f GraphCoordsToRecoilCoords(FVector2f GraphCoords) const;

	int32 FindUnitByScreenLocation(const FVector2f& ScreenLocation) const;

	void SelectUnitsInPanelCoordsRect(const FSlateRect& SelectionRect) const;

	void TryAutoRearrangeUnits() const;

	int32 ScaleFromRecoilCoordsToGraphCoords = 16;

	TWeakObjectPtr<UCRRecoilUnitGraph> RecoilUnitGraph;

	FCRRecoilPatternEditor* RecoilPatternEditor = nullptr;

	TWeakObjectPtr<UTransBuffer> TransactionBuffer;

	FDelegateHandle BeforeUndoRedoHandle;

	TSharedPtr<SCRRecoilUnitGraphBackgroundWidget> BackgroundWidget;

	TSharedPtr<SVerticalBox> ShortcutsContainer;

	TOptional<FCRUnitGraphViewDelayedDrag> ViewDrag;

	TOptional<FCRUnitGraphSelectionDelayedDrag> SelectionDrag;

	TOptional<FCRUnitGraphMoveUnitsDelayedDrag> MoveUnitsDrag;

	TOptional<FCRUnitGraphScaleUnitsDelayedDrag> ScaleUnitsDrag;

	int32 LastLeftMouseDownFoundUnitID = INDEX_NONE;

	FVector2f CurrentMousePanelPosition = FVector2f::ZeroVector;

	mutable bool bNeedZoomToFit = false;
};
