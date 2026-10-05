// Copyright CrystalVapor 2026, All rights reserved.

#include "Editor/CRRecoilPatternEditor.h"
#include "Data/CRRecoilPattern.h"
#include "Data/CRRecoilUnitGraph.h"
#include "Editor/CRRecoilPatternEditorCommands.h"
#include "IStructureDetailsView.h"
#include "ScopedTransaction.h"
#include "Widget/CRRecoilUnitGraphEditor.h"
#include "Editor.h"

void FCRRecoilUnitSelection::AddSelection(const int32 UnitID)
{
	SelectedUnits.AddUnique(UnitID);
	OnSelectionChanged.Broadcast();
}

void FCRRecoilUnitSelection::AddSelection(const TArray<int32>& UnitsArray)
{
	for (const int32 Unit : UnitsArray)
	{
		SelectedUnits.AddUnique(Unit);
	}

	OnSelectionChanged.Broadcast();
}

void FCRRecoilUnitSelection::AddSelection(const TArray<FCRRecoilUnit>& RecoilUnitsArray)
{
	for (const FCRRecoilUnit& RecoilUnit : RecoilUnitsArray)
	{
		SelectedUnits.AddUnique(RecoilUnit.ID);
	}

	OnSelectionChanged.Broadcast();
}

void FCRRecoilUnitSelection::RemoveSelection(const int32 UnitID)
{
	SelectedUnits.Remove(UnitID);
	OnSelectionChanged.Broadcast();
}

void FCRRecoilUnitSelection::ClearSelection()
{
	SelectedUnits.Empty();
	OnSelectionChanged.Broadcast();
}

const TArray<int32>& FCRRecoilUnitSelection::GetSelection() const
{
	return SelectedUnits;
}

TArray<FCRRecoilUnit*> FCRRecoilUnitSelection::GetSelectedRecoilUnits(UCRRecoilUnitGraph* UnitGraph) const
{
	TArray<FCRRecoilUnit*> SelectedRecoilUnits;
	for (const int32 UnitID : SelectedUnits)
	{
		if (FCRRecoilUnit* RecoilUnit = UnitGraph->GetUnitByID(UnitID))
		{
			SelectedRecoilUnits.Add(RecoilUnit);
		}
	}
	return SelectedRecoilUnits;
}

bool FCRRecoilUnitSelection::IsEmpty() const
{
	return SelectedUnits.IsEmpty();
}

bool FCRRecoilUnitSelection::IsUnitSelected(const int32 UnitID) const
{
	return SelectedUnits.Contains(UnitID);
}

int32 FCRRecoilUnitSelection::GetNum() const
{
	return SelectedUnits.Num();
}

FCRRecoilPatternEditor::FCRRecoilPatternEditor()
{
	RecoilUnitSelection.OnSelectionChanged.AddRaw<FCRRecoilPatternEditor>(this, &FCRRecoilPatternEditor::OnSelectionChanged);
	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
}

void FCRRecoilPatternEditor::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshUnitSelection();
	}
}

void FCRRecoilPatternEditor::RefreshUnitSelection()
{
	UCRRecoilUnitGraph* UnitGraph = GetRecoilUnitGraph();
	if (!UnitGraph)
	{
		return;
	}

	const TArray<int32> SelectedIDs = RecoilUnitSelection.GetSelection();
	for (const int32 UnitID : SelectedIDs)
	{
		if (!UnitGraph->GetUnitByID(UnitID))
		{
			RecoilUnitSelection.RemoveSelection(UnitID);
		}
	}
	OnSelectionChanged();
}

void FCRRecoilPatternEditor::OnGraphDetailsChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (bEnableAutoRearrangeUnits && PropertyChangedEvent.GetMemberPropertyName() == FName(TEXT("RecoilUnits")))
	{
		if (UCRRecoilUnitGraph* UnitGraph = GetRecoilUnitGraph())
		{
			UnitGraph->RearrangeUnits();
		}
	}
	RefreshUnitSelection();
}

void FCRRecoilPatternEditor::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

TSharedRef<FCRRecoilPatternEditor> FCRRecoilPatternEditor::CreateRecoilPatternEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UCRRecoilPattern* RecoilPattern)
{
	TSharedRef<FCRRecoilPatternEditor> RecoilPatternEditor = MakeShareable(new FCRRecoilPatternEditor());
	RecoilPatternEditor->InitializeEditor(Mode, InitToolkitHost, RecoilPattern);
	return RecoilPatternEditor;
}

FName FCRRecoilPatternEditor::GetToolkitFName() const
{
	return FName("CRRecoilPatternEditor");
}

FText FCRRecoilPatternEditor::GetBaseToolkitName() const
{
	return NSLOCTEXT("CrystalRecoil", "RecoilPatternEditorAppLabel", "Recoil Pattern Editor");
}

FString FCRRecoilPatternEditor::GetWorldCentricTabPrefix() const
{
	return FString("Recoil Pattern Editor");
}

FLinearColor FCRRecoilPatternEditor::GetWorldCentricTabColorScale() const
{
	return FColor::Cyan;
}

void FCRRecoilPatternEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(NSLOCTEXT("CRRecoilPatternEditor", "WorkspaceMenu_RecoilPatternEditor", "Recoil Pattern Editor"));

	InTabManager->RegisterTabSpawner(UnitGraphTabFName,
				 FOnSpawnTab::CreateSP(this, &FCRRecoilPatternEditor::GetTab_UnitGraph))
				.SetDisplayName(NSLOCTEXT("CRRecoilPatternEditor", "UnitGraphTab", "Graph"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(UnitGraphDetailsTabFName,
				 FOnSpawnTab::CreateSP(this, &FCRRecoilPatternEditor::GetTab_UnitGraphDetails))
				.SetDisplayName(NSLOCTEXT("CRRecoilPatternEditor", "UnitGraphDetailsTab", "Graph Settings"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(UnitDetailsTabFName,
				 FOnSpawnTab::CreateSP(this, &FCRRecoilPatternEditor::GetTab_UnitDetails))
				.SetDisplayName(NSLOCTEXT("CRRecoilPatternEditor", "UnitDetailsTab", "Selection"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(RecoilDetailsTabFName,
				 FOnSpawnTab::CreateSP(this, &FCRRecoilPatternEditor::GetTab_RecoilDetails))
				.SetDisplayName(NSLOCTEXT("CRRecoilPatternEditor", "RecoilDetailsTab", "Recoil Settings"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FCRRecoilPatternEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

UCRRecoilPattern* FCRRecoilPatternEditor::GetRecoilPattern() const
{
	return CastChecked<UCRRecoilPattern>(GetEditingObject());
}

UCRRecoilUnitGraph* FCRRecoilPatternEditor::GetRecoilUnitGraph() const
{
	const UCRRecoilPattern* RecoilPattern = GetRecoilPattern();
	return RecoilPattern ? RecoilPattern->GetUnitGraph() : nullptr;
}

FCRRecoilUnitSelection& FCRRecoilPatternEditor::GetRecoilUnitSelection()
{
	return RecoilUnitSelection;
}

const FCRRecoilUnitSelection& FCRRecoilPatternEditor::GetRecoilUnitSelection() const
{
	return RecoilUnitSelection;
}

void FCRRecoilPatternEditor::MapCommands()
{
	const FCRRecoilPatternEditorCommands& Commands = FCRRecoilPatternEditorCommands::Get();
	const TSharedRef<FUICommandList> EditorCommands = GetToolkitCommands();

	EditorCommands->MapAction
	(
		Commands.RemoveUnit,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_RemoveUnit),
		FCanExecuteAction::CreateLambda([this]() { return !RecoilUnitSelection.IsEmpty(); })
	);

	EditorCommands->MapAction
	(
		Commands.SelectAll,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_SelectAll),
		FCanExecuteAction::CreateLambda([]() { return true; })
	);

	EditorCommands->MapAction
	(
		Commands.CopyUnits,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_CopyUnits),
		FCanExecuteAction::CreateLambda([this]() { return !RecoilUnitSelection.IsEmpty(); })
	);

	EditorCommands->MapAction
	(
		Commands.PasteUnits,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_PasteUnits),
		FCanExecuteAction::CreateLambda([]() { return true; })
	);

	EditorCommands->MapAction
	(
		Commands.UnitsSnapping,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_SwitchUnitSnapping),
		FCanExecuteAction::CreateLambda([]() { return true; }),
		FIsActionChecked::CreateLambda([this]() { return bEnableGridSnapping; })
	);

	EditorCommands->MapAction
	(
		Commands.AutoRearrangeUnits,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_SwitchAutoRearrangeUnits),
		FCanExecuteAction::CreateLambda([]() { return true; }),
		FIsActionChecked::CreateLambda([this]() { return bEnableAutoRearrangeUnits; })
	);

	EditorCommands->MapAction
	(
		Commands.UnitScaling,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_SwitchUnitScaling),
		FCanExecuteAction::CreateLambda([this]() { return RecoilUnitSelection.GetNum() > 1; }),
		FIsActionChecked::CreateLambda([this]() { return bEnableUnitScaling; })
	);

	EditorCommands->MapAction
	(
		Commands.ZoomViewToFit,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_ZoomToFitAllUnits)
	);

	EditorCommands->MapAction
	(
		Commands.ToggleShortcutsVisibility,
		FExecuteAction::CreateRaw(this, &FCRRecoilPatternEditor::Command_ToggleShortcutsVisibility),
		FCanExecuteAction::CreateLambda([]() { return true; }),
		FIsActionChecked::CreateLambda([this]() { return bShowShortcuts; })
	);
}

void FCRRecoilPatternEditor::ExtendToolBar()
{
	auto RegisterToolBarExtensions = [&](FToolBarBuilder& Builder)
	{
		const FCRRecoilPatternEditorCommands& Commands = FCRRecoilPatternEditorCommands::Get();
		Builder.PushCommandList(ToolkitCommands);
		Builder.BeginSection("RecoilPatternToolBarExtHook");

		Builder.AddToolBarButton(
			Commands.UnitScaling,
			"RecoilPatternToolBarExtHook",
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon("EditorStyle", "ViewportToolbar.TransformScale"));

		Builder.AddWidget(SNew(SSpacer).Size(FVector2D(8.f, 0.f)));

		Builder.BeginBlockGroup();
		Builder.AddToolBarButton(
			Commands.UnitsSnapping,
			"RecoilPatternToolBarExtHook",
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon("EditorStyle", "GenericCurveEditor.ToggleInputSnapping"));

		Builder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP(this, &FCRRecoilPatternEditor::GetMenuContent_UnitsSnapping),
			FText(),
			Commands.UnitsSnapping->GetDescription(),
			FSlateIcon(),
			true);
		Builder.EndBlockGroup();

		Builder.AddWidget(SNew(SSpacer).Size(FVector2D(8.f, 0.f)));

		Builder.AddToolBarButton(
			Commands.AutoRearrangeUnits,
			"RecoilPatternToolBarExtHook",
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon("EditorStyle", "GenericCurveEditor.StraightenTangents"));

		Builder.AddWidget(SNew(SSpacer).Size(FVector2D(4.f, 0.f)));

		Builder.AddToolBarButton(
			Commands.ZoomViewToFit,
			"RecoilPatternToolBarExtHook",
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon("EditorStyle", "Curve.ZoomToFit"));

		Builder.EndSection();
		Builder.PopCommandList();
	};

	const TSharedPtr<FExtender> ToolBarExtender = MakeShared<FExtender>();
	ToolBarExtender->AddToolBarExtension("Asset", EExtensionHook::After, ToolkitCommands, FToolBarExtensionDelegate::CreateLambda(RegisterToolBarExtensions));
	AddToolbarExtender(ToolBarExtender);
}

void FCRRecoilPatternEditor::InitializeEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InAsset)
{
	MapCommands();
	InitAssetEditor(Mode, InitToolkitHost, ToolkitFName, CreateEditorLayout(), true, true, InAsset);
	if (!UnitGraphWidget.IsValid())
	{
		BuildTab_UnitGraph();
	}
	ExtendToolBar();
	RegenerateMenusAndToolbars();
}

TSharedRef<FTabManager::FLayout> FCRRecoilPatternEditor::CreateEditorLayout()
{
	return FTabManager::NewLayout(ToolkitFName)
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.75f)
					->SetHideTabWell(true)
					->AddTab(UnitGraphTabFName, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.25f)
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.5f)
						->SetForegroundTab(UnitDetailsTabFName)
						->AddTab(UnitDetailsTabFName, ETabState::OpenedTab)
						->AddTab(UnitGraphDetailsTabFName, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.5f)
						->AddTab(RecoilDetailsTabFName, ETabState::OpenedTab)
					)
				)
			)
		);
}

void FCRRecoilPatternEditor::BuildTab_UnitGraph()
{
	UnitGraphWidget = SNew(SCRRecoilUnitGraphWidget)
		.RecoilPatternEditor(this);
	UnitGraphWidget->SetRecoilUnitGraph(GetRecoilUnitGraph());
}

void FCRRecoilPatternEditor::BuildTab_UnitGraphDetails()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;

	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	UnitGraphDetailsWidget = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	UnitGraphDetailsWidget->OnFinishedChangingProperties().AddSP(this, &FCRRecoilPatternEditor::OnGraphDetailsChanged);
}

void FCRRecoilPatternEditor::BuildTab_UnitDetails()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;

	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	UnitDetailsWidget = PropertyEditorModule.CreateStructureDetailView(DetailsViewArgs, FStructureDetailsViewArgs(),
	                                                                   nullptr);
	OnSelectionChanged();
}

void FCRRecoilPatternEditor::BuildTab_RecoilDetails()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	RecoilDetailsWidget = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
}

TSharedRef<SDockTab> FCRRecoilPatternEditor::GetTab_UnitGraph(const FSpawnTabArgs& Args)
{
	if (!UnitGraphWidget.IsValid())
	{
		BuildTab_UnitGraph();
	}

	UnitGraphWidget->SetRecoilUnitGraph(GetRecoilUnitGraph());

	return SNew(SDockTab)
		.Label(NSLOCTEXT("CRRecoilPatternEditor", "UnitGraphTab", "Graph"))
		[
			UnitGraphWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FCRRecoilPatternEditor::GetTab_UnitGraphDetails(const FSpawnTabArgs& Args)
{
	if (!UnitGraphDetailsWidget.IsValid())
	{
		BuildTab_UnitGraphDetails();
	}

	UnitGraphDetailsWidget->SetObject(GetRecoilUnitGraph());

	return SNew(SDockTab)
		.Label(NSLOCTEXT("CRRecoilPatternEditor", "UnitGraphDetailsTab", "Graph Settings"))
		[
			UnitGraphDetailsWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FCRRecoilPatternEditor::GetTab_UnitDetails(const FSpawnTabArgs& Args)
{
	if (!UnitDetailsWidget.IsValid())
	{
		BuildTab_UnitDetails();
	}

	return SNew(SDockTab)
		.Label(NSLOCTEXT("CRRecoilPatternEditor", "UnitDetailsTab", "Selection"))
		[
			UnitDetailsWidget->GetWidget().ToSharedRef()
		];
}

TSharedRef<SDockTab> FCRRecoilPatternEditor::GetTab_RecoilDetails(const FSpawnTabArgs& Args)
{
	if (!RecoilDetailsWidget.IsValid())
	{
		BuildTab_RecoilDetails();
	}

	RecoilDetailsWidget->SetObject(GetRecoilPattern());

	return SNew(SDockTab)
		.Label(NSLOCTEXT("CRRecoilPatternEditor", "RecoilDetailsTab", "Recoil Settings"))
		[
			RecoilDetailsWidget.ToSharedRef()
		];
}

void FCRRecoilPatternEditor::Command_RemoveUnit() const
{
	UnitGraphWidget->DeleteUnits();
}

void FCRRecoilPatternEditor::Command_SelectAll()
{
	UCRRecoilUnitGraph* RecoilUnitGraph = GetRecoilUnitGraph();
	check(RecoilUnitGraph);
	RecoilUnitSelection.ClearSelection();
	RecoilUnitSelection.AddSelection(RecoilUnitGraph->GetRecoilUnits());
}

void FCRRecoilPatternEditor::Command_CopyUnits() const
{
	UnitGraphWidget->CopySelectedUnits();
}

void FCRRecoilPatternEditor::Command_PasteUnits() const
{
	UnitGraphWidget->PasteUnits();
}

void FCRRecoilPatternEditor::Command_SwitchUnitSnapping()
{
	bEnableGridSnapping = !bEnableGridSnapping;
}

void FCRRecoilPatternEditor::Command_SwitchAutoRearrangeUnits()
{
	bEnableAutoRearrangeUnits = !bEnableAutoRearrangeUnits;
}

void FCRRecoilPatternEditor::Command_SwitchUnitScaling()
{
	if (bEnableUnitScaling)
	{
		UnitGraphWidget->StopUnitScaling();
		return;
	}

	if (const TSharedPtr<FTabManager> EditorTabManager = GetTabManager())
	{
		EditorTabManager->TryInvokeTab(UnitGraphTabFName);
	}
	bEnableUnitScaling = true;
	UnitGraphWidget->StartUnitScaling();
}

void FCRRecoilPatternEditor::Command_ZoomToFitAllUnits() const
{
	UnitGraphWidget->ZoomToFitAllUnits();
}

void FCRRecoilPatternEditor::Command_ToggleShortcutsVisibility()
{
	bShowShortcuts = !bShowShortcuts;
}

void FCRRecoilPatternEditor::RefreshUnitPosition() const
{
	if (RecoilUnitSelection.GetNum() != 1 || !SelectedUnitScope.IsValid() || !UnitDetailsWidget.IsValid())
	{
		return;
	}

	const int32 UnitID = RecoilUnitSelection.GetSelection()[0];
	const FCRRecoilUnit* ActualUnit = GetRecoilUnitGraph()->GetUnitByID(UnitID);
	if (!ActualUnit)
	{
		return;
	}

	FCRRecoilUnit* ScopedUnit = reinterpret_cast<FCRRecoilUnit*>(SelectedUnitScope->GetStructMemory());
	ScopedUnit->Position = ActualUnit->Position;
	UnitDetailsWidget->SetStructureData(SelectedUnitScope);
}

TSharedRef<SWidget> FCRRecoilPatternEditor::GetMenuContent_UnitsSnapping()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection("GridSnappingSize", NSLOCTEXT("CrystalRecoil", "GridSnappingSizeSection", "Snap Sizes"));

	FNumberFormattingOptions NumberFormattingOptions;
	NumberFormattingOptions.MaximumFractionalDigits = 2;

	const TArray<float> SnapSizes = { 0.1f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f, 5.0f, 10.0f };

	for (const float SnapSize : SnapSizes)
	{
		const FText Label = FText::AsNumber(SnapSize, &NumberFormattingOptions);
		const FText ToolTip = FText::Format(
			NSLOCTEXT("CrystalRecoil", "GridSnappingTooltip", "Snaps units to increments of {0}"),
			Label
		);

		MenuBuilder.AddMenuEntry(
			Label,
			ToolTip,
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, SnapSize]()
				{
					GridSnappingValue = SnapSize;
				}),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, SnapSize]()
				{
					return FMath::IsNearlyEqual(GridSnappingValue, SnapSize);
				})
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
	}

	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void FCRRecoilPatternEditor::OnSelectionChanged() const
{
	if (RecoilUnitSelection.GetNum() == 1)
	{
		const int32 UnitID = RecoilUnitSelection.GetSelection()[0];
		UCRRecoilUnitGraph* RecoilUnitGraph = GetRecoilUnitGraph();
		check(RecoilUnitGraph);
		const FCRRecoilUnit* RecoilUnit = RecoilUnitGraph->GetUnitByID(UnitID);

		if (RecoilUnit && UnitDetailsWidget.IsValid())
		{
			// Copy unit into a safe scope - avoids dangling pointer if TArray reallocates
			SelectedUnitScope = MakeShared<FStructOnScope>(FCRRecoilUnit::StaticStruct());
			FCRRecoilUnit* ScopedUnit = reinterpret_cast<FCRRecoilUnit*>(SelectedUnitScope->GetStructMemory());
			*ScopedUnit = *RecoilUnit;

			UnitDetailsWidget->GetOnFinishedChangingPropertiesDelegate().Clear();
			UnitDetailsWidget->GetOnFinishedChangingPropertiesDelegate().AddLambda([this](const FPropertyChangedEvent&)
			{
				if (RecoilUnitSelection.GetNum() != 1 || !SelectedUnitScope.IsValid())
				{
					return;
				}

				UCRRecoilUnitGraph* RecoilUnitGraph = GetRecoilUnitGraph();
				const int32 CurrentUnitID = RecoilUnitSelection.GetSelection()[0];
				FCRRecoilUnit* ActualUnit = RecoilUnitGraph->GetUnitByID(CurrentUnitID);
				if (!ActualUnit)
				{
					return;
				}

				// Write edited copy back into the actual array
				const FCRRecoilUnit* EditedUnit = reinterpret_cast<const FCRRecoilUnit*>(SelectedUnitScope->GetStructMemory());
				FScopedTransaction Transaction(NSLOCTEXT("CRRecoilPatternEditor", "EditRecoilUnit", "Edit Recoil Unit"));
				RecoilUnitGraph->Modify();
				*ActualUnit = *EditedUnit;

				if (bEnableAutoRearrangeUnits)
				{
					RecoilUnitGraph->RearrangeUnits();
				}

				RecoilUnitGraph->MarkPackageDirty();
			});

			UnitDetailsWidget->SetStructureData(SelectedUnitScope);
		}
		else if (UnitDetailsWidget.IsValid())
		{
			UnitDetailsWidget->SetStructureData(nullptr);
		}
	}
	else
	{
		// Clear the details panel when nothing or multiple units are selected
		if (UnitDetailsWidget.IsValid())
		{
			UnitDetailsWidget->SetStructureData(nullptr);
		}
	}

	if (RecoilUnitSelection.GetNum() <= 1 && bEnableUnitScaling && UnitGraphWidget.IsValid())
	{
		UnitGraphWidget->StopUnitScaling();
	}
}
