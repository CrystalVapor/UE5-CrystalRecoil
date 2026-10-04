// Copyright CrystalVapor 2026, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"

class SCRRecoilUnitGraphWidget;
class UCRRecoilUnitGraph;
class UCRRecoilPattern;
struct FPropertyChangedEvent;

class FCRRecoilUnitSelection
{
public:
	void AddSelection(const int32 UnitID);

	void AddSelection(const TArray<int32>& UnitsArray);

	void AddSelection(const TArray<struct FCRRecoilUnit>& RecoilUnitsArray);

	void RemoveSelection(const int32 UnitID);

	void ClearSelection();

	const TArray<int32>& GetSelection() const;

	TArray<FCRRecoilUnit*> GetSelectedRecoilUnits(UCRRecoilUnitGraph* UnitGraph) const;

	bool IsEmpty() const;

	bool IsUnitSelected(const int32 UnitID) const;

	int32 GetNum() const;

	FSimpleMulticastDelegate OnSelectionChanged;

protected:
	TArray<int32> SelectedUnits;
};

class CRYSTALRECOILEDITOR_API FCRRecoilPatternEditor : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	FCRRecoilPatternEditor();

	virtual void PostUndo(bool bSuccess) override;

	virtual void PostRedo(bool bSuccess) override;

	static TSharedRef<FCRRecoilPatternEditor> CreateRecoilPatternEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UCRRecoilPattern* RecoilPattern);

	virtual FName GetToolkitFName() const override;

	virtual FText GetBaseToolkitName() const override;

	virtual FString GetWorldCentricTabPrefix() const override;

	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	UCRRecoilPattern* GetRecoilPattern() const;

	UCRRecoilUnitGraph* GetRecoilUnitGraph() const;

	FCRRecoilUnitSelection& GetRecoilUnitSelection();

	const FCRRecoilUnitSelection& GetRecoilUnitSelection() const;

	void MapCommands();

	void ExtendToolBar();

	void InitializeEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InAsset);

	static TSharedRef<FTabManager::FLayout> CreateEditorLayout();

	void BuildTab_UnitGraph();

	void BuildTab_UnitGraphDetails();

	void BuildTab_UnitDetails();

	void BuildTab_RecoilDetails();

	TSharedRef<SDockTab> GetTab_UnitGraph(const FSpawnTabArgs& Args);

	TSharedRef<SDockTab> GetTab_UnitGraphDetails(const FSpawnTabArgs& Args);

	TSharedRef<SDockTab> GetTab_UnitDetails(const FSpawnTabArgs& Args);

	TSharedRef<SDockTab> GetTab_RecoilDetails(const FSpawnTabArgs& Args);

	TSharedRef<SWidget> GetMenuContent_UnitsSnapping();

	void Command_RemoveUnit() const;

	void Command_SelectAll();

	void Command_CopyUnits() const;

	void Command_PasteUnits() const;

	void Command_SwitchUnitSnapping();

	void Command_SwitchAutoRearrangeUnits();

	void Command_SwitchUnitScaling();

	void Command_ZoomToFitAllUnits() const;

	void Command_ToggleShortcutsVisibility();

	void RefreshUnitPosition() const;

	bool bShowShortcuts = true;

	bool bEnableUnitScaling = false;

	bool bEnableAutoRearrangeUnits = true;

	bool bEnableGridSnapping = true;

	float GridSnappingValue = 0.1f;

protected:
	void RefreshUnitSelection();

	void OnGraphDetailsChanged(const FPropertyChangedEvent& PropertyChangedEvent);

	void OnSelectionChanged() const;

	inline static const FName ToolkitFName = FName("CRRecoilPatternEditor");

	inline static const FName UnitGraphTabFName = FName("CRRecoilPatternEditor_UnitGraph");

	inline static const FName UnitGraphDetailsTabFName = FName("CRRecoilPatternEditor_UnitGraphDetails");

	inline static const FName UnitDetailsTabFName = FName("CRRecoilPatternEditor_UnitDetails");

	inline static const FName RecoilDetailsTabFName = FName("CRRecoilPatternEditor_RecoilDetails");

	FCRRecoilUnitSelection RecoilUnitSelection;

	TSharedPtr<SCRRecoilUnitGraphWidget> UnitGraphWidget;

	TSharedPtr<IDetailsView> UnitGraphDetailsWidget;

	TSharedPtr<IStructureDetailsView> UnitDetailsWidget;

	TSharedPtr<IDetailsView> RecoilDetailsWidget;

	mutable TSharedPtr<FStructOnScope> SelectedUnitScope;
};
