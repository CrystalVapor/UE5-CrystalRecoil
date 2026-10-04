// Copyright CrystalVapor 2026, All rights reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/CRRecoilComponent.h"
#include "Components/CRRecoilSpreadComponent.h"
#include "CRRecoilScaledTestComponent.h"
#include "Data/CRRecoilPattern.h"
#include "Data/CRRecoilUnitGraph.h"
#include "Editor.h"
#include "Editor/CRRecoilPatternEditor.h"
#include "Editor/CRRecoilPatternEditorCommands.h"
#include "Editor/CRRecoilUnitGraphWidgetDragOperations.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "ScopedTransaction.h"
#include "UObject/Package.h"
#include "UObject/StructOnScope.h"
#include "Widget/CRRecoilUnitGraphEditor.h"
#include <limits>

namespace
{
	class FRecoilTestPatternEditor : public FCRRecoilPatternEditor
	{
	public:
		void SetPattern(UCRRecoilPattern* Pattern)
		{
			AddEditingObject(Pattern);
		}

		void NotifyGraphSettingsChanged(FProperty* ChangedProperty = nullptr)
		{
			FPropertyChangedEvent Event(ChangedProperty, EPropertyChangeType::ValueSet);
			UnitGraphDetailsWidget->OnFinishedChangingProperties().Broadcast(Event);
		}

		FVector2f GetSelectedPosition() const
		{
			return reinterpret_cast<const FCRRecoilUnit*>(SelectedUnitScope->GetStructMemory())->Position;
		}

		bool HasSelectedUnit() const
		{
			return SelectedUnitScope.IsValid();
		}

		void SetGraphWidget(const TSharedRef<SCRRecoilUnitGraphWidget>& Widget)
		{
			UnitGraphWidget = Widget;
		}
	};

	class FRecoilTestGraphWidget : public SCRRecoilUnitGraphWidget
	{
	public:
		void BeginDrag(UCRRecoilUnitGraph* Graph, const FCRRecoilUnitSelection& Selection, const bool bScaling)
		{
			if (bScaling)
			{
				ScaleUnitsDrag.Emplace(Graph, Selection, FVector2f::ZeroVector, FVector2f::ZeroVector, EKeys::LeftMouseButton);
				ScaleUnitsDrag->ApplyScaling(Selection, 2.f);
			}
			else
			{
				MoveUnitsDrag.Emplace(Graph, Selection, FVector2f::ZeroVector, FVector2f::ZeroVector, EKeys::LeftMouseButton);
				MoveUnitsDrag->ApplyMovement(Selection, FVector2f(0.f, 2.f));
			}
			Graph->RearrangeUnits();
		}

		bool HasActiveDrag() const
		{
			return MoveUnitsDrag.IsSet() || ScaleUnitsDrag.IsSet();
		}

		void BeginScaling(UCRRecoilUnitGraph* Graph, const FCRRecoilUnitSelection& Selection)
		{
			ScaleUnitsDrag.Emplace(Graph, Selection, FVector2f(0.f, 1.25f), FVector2f::ZeroVector, EKeys::LeftMouseButton);
			ScaleUnitsDrag->NormalVectorSizePanel = 1.f;
		}
	};

	struct FRecoilTestWorld
	{
		FRecoilTestWorld(TSubclassOf<UCRRecoilComponent> ComponentClass = UCRRecoilComponent::StaticClass())
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			Controller = World->SpawnActor<APlayerController>();
			Controller->SetAsLocalPlayerController();
			World->AddController(Controller);
			Component = NewObject<UCRRecoilComponent>(World->SpawnActor<AActor>(), ComponentClass);
			Pattern = NewObject<UCRRecoilPattern>(Component);
			Component->SetTargetController(Controller);
			Component->SetRecoilPattern(Pattern);
			Component->RegisterComponent();
		}

		~FRecoilTestWorld()
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}

		void Advance(const int32 Frames)
		{
			constexpr float DeltaTime = 1.f / 120.f;
			for (int32 Frame = 0; Frame < Frames; ++Frame)
			{
				World->Tick(LEVELTICK_TimeOnly, DeltaTime);
				if (Component->IsComponentTickEnabled())
				{
					Component->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
				}
			}
		}

		UWorld* World;
		APlayerController* Controller;
		UCRRecoilComponent* Component;
		UCRRecoilPattern* Pattern;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilScalingPanTest, "CrystalRecoil.Editor.ScalingPan", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilScalingPanTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>();
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 FirstID = Graph->AddUnit(FVector2f(0.f, 1.f));
	const int32 SecondID = Graph->AddUnit(FVector2f(0.f, 2.f));
	const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
	Editor->SetPattern(Pattern);
	Editor->GetRecoilUnitSelection().AddSelection(FirstID);
	Editor->GetRecoilUnitSelection().AddSelection(SecondID);
	const TSharedRef<FRecoilTestGraphWidget> Widget = SNew(FRecoilTestGraphWidget).RecoilPatternEditor(&Editor.Get());
	Widget->SetRecoilUnitGraph(Graph);
	Widget->BeginScaling(Graph, Editor->GetRecoilUnitSelection());
	Editor->bEnableUnitScaling = true;
	const FPointerEvent MouseDown(0, FVector2D::ZeroVector, FVector2D::ZeroVector, TSet<FKey>{ EKeys::RightMouseButton }, EKeys::RightMouseButton, 0.f, FModifierKeysState());
	Widget->OnMouseButtonDown(FGeometry(), MouseDown);
	const FPointerEvent MouseMove(0, FVector2D(30.0, 0.0), FVector2D::ZeroVector, TSet<FKey>{ EKeys::RightMouseButton }, EKeys::Invalid, 0.f, FModifierKeysState());
	Widget->OnMouseMove(FGeometry(), MouseMove);
	TestFalse(TEXT("Panning ends the scaling preview"), Widget->HasActiveDrag());
	TestFalse(TEXT("Panning disables scaling mode"), Editor->bEnableUnitScaling);
	TestEqual(TEXT("Panning preserves the first point"), Graph->GetUnitByID(FirstID)->Position, FVector2f(0.f, 1.f));
	TestEqual(TEXT("Panning preserves the second point"), Graph->GetUnitByID(SecondID)->Position, FVector2f(0.f, 2.f));
	Widget->FinishDragging();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilScalingFocusTest, "CrystalRecoil.Editor.ScalingFocus", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilScalingFocusTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>();
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 FirstID = Graph->AddUnit(FVector2f(0.f, 1.f));
	const int32 SecondID = Graph->AddUnit(FVector2f(0.f, 2.f));
	const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
	Editor->SetPattern(Pattern);
	Editor->GetRecoilUnitSelection().AddSelection(FirstID);
	Editor->GetRecoilUnitSelection().AddSelection(SecondID);
	const TSharedRef<FRecoilTestGraphWidget> Widget = SNew(FRecoilTestGraphWidget).RecoilPatternEditor(&Editor.Get());
	Widget->SetRecoilUnitGraph(Graph);
	Editor->SetGraphWidget(Widget);
	Editor->bEnableUnitScaling = true;
	Widget->StartUnitScaling();
	TestFalse(TEXT("Scaling cannot start in a graph that cannot receive focus"), Widget->HasActiveDrag());
	TestFalse(TEXT("A failed focus request disables scaling mode"), Editor->bEnableUnitScaling);
	Widget->StopUnitScaling();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilScalingSelectionTest, "CrystalRecoil.Editor.ScalingSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilScalingSelectionTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>(CreatePackage(TEXT("/Game/CrystalRecoilScalingSelectionTest")), NAME_None, RF_Transactional);
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 UnitID = Graph->AddUnit(FVector2f(0.f, 1.f));
	const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
	Editor->SetPattern(Pattern);
	Editor->MapCommands();
	Editor->BuildTab_UnitGraphDetails();
	Editor->GetRecoilUnitSelection().AddSelection(UnitID);
	const TSharedRef<FRecoilTestGraphWidget> Widget = SNew(FRecoilTestGraphWidget).RecoilPatternEditor(&Editor.Get());
	Widget->SetRecoilUnitGraph(Graph);
	Editor->SetGraphWidget(Widget);
	TestFalse(TEXT("The scaling command needs more than one selected point"), Editor->GetToolkitCommands()->CanExecuteAction(FCRRecoilPatternEditorCommands::Get().UnitScaling.ToSharedRef()));
	Widget->StartUnitScaling();
	TestFalse(TEXT("Scaling rejects a single selected point"), Widget->HasActiveDrag());

	// Exercise selection loss while a legacy single-point scaling preview is active.
	Widget->BeginScaling(Graph, Editor->GetRecoilUnitSelection());
	Editor->bEnableUnitScaling = true;
	FProperty* UnitArrayProperty = UCRRecoilUnitGraph::StaticClass()->FindPropertyByName(TEXT("RecoilUnits"));
	{
		FScopedTransaction Transaction(NSLOCTEXT("CrystalRecoilTests", "ClearUnits", "Clear recoil units"));
		Graph->Modify();
		Graph->GetRecoilUnits().Reset();
		FPropertyChangedEvent Event(UnitArrayProperty, EPropertyChangeType::ArrayClear);
		Graph->PostEditChangeProperty(Event);
		Editor->NotifyGraphSettingsChanged(UnitArrayProperty);
		TestFalse(TEXT("Losing the final selection ends the scaling preview"), Widget->HasActiveDrag());
		TestFalse(TEXT("Losing the final selection disables scaling mode"), Editor->bEnableUnitScaling);
	}
	Widget->FinishDragging();
	GEditor->UndoTransaction();
	TestEqual(TEXT("Undo restores the removed point"), Graph->GetUnitCount(), 1);
	const int32 AddedID = Graph->AddUnit(FVector2f(0.f, 2.f));
	TestTrue(TEXT("Undo preserves the next unique unit ID"), AddedID != UnitID);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilBlueprintHeatCapTest, "CrystalRecoil.Runtime.BlueprintHeatCap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilBlueprintHeatCapTest::RunTest(const FString& Parameters)
{
	UCRRecoilSpreadComponent* Component = NewObject<UCRRecoilSpreadComponent>();
	FProperty* HeatCapProperty = Component->GetClass()->FindPropertyByName(TEXT("MaxRecoilHeat"));
	if (!TestNotNull(TEXT("The heat cap is reflected"), HeatCapProperty))
	{
		return false;
	}
	TestEqual(TEXT("Blueprint property writes use the heat-cap setter"), HeatCapProperty->GetMetaData(TEXT("BlueprintSetter")), FString(TEXT("SetMaxRecoilHeat")));
	UFunction* Setter = Component->FindFunction(FName(*HeatCapProperty->GetMetaData(TEXT("BlueprintSetter"))));
	if (!TestNotNull(TEXT("The reflected heat-cap setter exists"), Setter))
	{
		return false;
	}
	TestTrue(TEXT("The setter remains Blueprint callable"), Setter->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	Component->AddRecoilHeat(80.f);
	struct FSetterParameters
	{
		float InMaxHeat = 10.f;
	} SetterParameters;
	Component->ProcessEvent(Setter, &SetterParameters);
	TestEqual(TEXT("The reflected setter immediately clamps existing heat"), Component->GetRecoilHeat(), 10.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilLateSelectionTest, "CrystalRecoil.Editor.LateSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilLateSelectionTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>();
	const int32 UnitID = Pattern->GetUnitGraph()->AddUnit(FVector2f(2.f, 3.f));
	const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
	Editor->SetPattern(Pattern);
	Editor->GetRecoilUnitSelection().AddSelection(UnitID);
	TestFalse(TEXT("An unopened details view has no selected structure"), Editor->HasSelectedUnit());
	Editor->BuildTab_UnitDetails();
	if (TestTrue(TEXT("Opening details populates the existing selection"), Editor->HasSelectedUnit()))
	{
		TestEqual(TEXT("The opened details view shows the selected unit"), Editor->GetSelectedPosition(), FVector2f(2.f, 3.f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilUnspawnedGraphTest, "CrystalRecoil.Editor.UnspawnedGraph", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilUnspawnedGraphTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>(CreatePackage(TEXT("/Game/CrystalRecoilUnspawnedGraphTest")), NAME_None, RF_Transactional);
	Pattern->GetUnitGraph()->AddUnit(FVector2f(0.f, 1.f));
	const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
	Editor->SetPattern(Pattern);
	Editor->BuildTab_UnitGraph();
	Editor->Command_ZoomToFitAllUnits();
	Editor->Command_SelectAll();
	Editor->Command_RemoveUnit();
	TestEqual(TEXT("Graph commands work before its tab is spawned"), Pattern->GetUnitGraph()->GetUnitCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilSpreadResumeTest, "CrystalRecoil.Runtime.SpreadResume", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilSpreadResumeTest::RunTest(const FString& Parameters)
{
	FRecoilTestWorld Fixture(UCRRecoilTestSpreadComponent::StaticClass());
	UCRRecoilTestSpreadComponent* Component = CastChecked<UCRRecoilTestSpreadComponent>(Fixture.Component);
	Component->InitializeCurves();
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(1.f, 0.f));
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(3.f, 0.f));
	Component->StartShooting();
	Component->ApplyShot();
	Fixture.Advance(240);
	TestFalse(TEXT("Recoil and heat stop ticking after cooldown"), Component->IsComponentTickEnabled());
	Fixture.Controller->SetControlRotation(FRotator(0.0, -10.0, 0.0));
	Fixture.Pattern->RecoveryDelay = 1.f;
	Component->AddRecoilHeat(10.f);
	Component->ApplyShot();
	Fixture.Advance(12);
	TestTrue(TEXT("The shot after external heat applies its full delta"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, -8.0, 0.001));
	Fixture.Advance(240);
	TestTrue(FString::Printf(TEXT("Idle aiming does not compensate the shot resumed by heat (yaw=%f, heat=%f)"), Fixture.Controller->GetControlRotation().Yaw, Component->GetRecoilHeat()), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, -10.0, 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilSpreadCancellationTest, "CrystalRecoil.Runtime.SpreadCancellation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilSpreadCancellationTest::RunTest(const FString& Parameters)
{
	FRecoilTestWorld Fixture(UCRRecoilTestSpreadComponent::StaticClass());
	UCRRecoilTestSpreadComponent* Component = CastChecked<UCRRecoilTestSpreadComponent>(Fixture.Component);
	Component->InitializeCurves();
	Component->SetRecoilHeatCoolDownDelay(10.f);
	Fixture.Pattern->RecoveryDelay = 0.f;
	Fixture.Pattern->RecoveryCancelThreshold = 1.f;
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(5.f, 0.f));
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f::ZeroVector);
	Component->StartShooting();
	Component->ApplyShot();
	Fixture.Advance(1);
	Fixture.Controller->SetControlRotation(Fixture.Controller->GetControlRotation() + FRotator(0.0, 2.0, 0.0));
	for (int32 Frame = 0; Frame < 60 && Component->IsTrackingInput(); ++Frame)
	{
		Fixture.Advance(1);
	}
	TestFalse(TEXT("Manual aiming cancels the first shot's recovery"), Component->IsTrackingInput());
	TestTrue(TEXT("Canceled uplift leaves the first shot and manual aiming"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, 7.0, 0.001));
	TestTrue(TEXT("Heat keeps ticking after recoil cancellation"), Component->IsComponentTickEnabled());
	Component->ApplyShot();
	Fixture.Advance(240);
	TestTrue(TEXT("An immediate opposite shot recovers without phantom compensation"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, 7.0, 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilSettingsOrderTest, "CrystalRecoil.Editor.SettingsOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilSettingsOrderTest::RunTest(const FString& Parameters)
{
	FProperty* UnitArrayProperty = UCRRecoilUnitGraph::StaticClass()->FindPropertyByName(TEXT("RecoilUnits"));
	if (!TestNotNull(TEXT("Recoil units have an editable array property"), UnitArrayProperty))
	{
		return false;
	}
	for (const bool bAutoRearrange : { false, true })
	{
		UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>();
		UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
		const int32 FirstID = Graph->AddUnit(FVector2f(0.f, 1.f));
		const int32 SecondID = Graph->AddUnit(FVector2f(0.f, 2.f));
		const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
		Editor->SetPattern(Pattern);
		Editor->BuildTab_UnitGraphDetails();
		Editor->bEnableAutoRearrangeUnits = bAutoRearrange;
		Graph->GetUnitByID(FirstID)->Position.Y = 3.f;
		FPropertyChangedEvent Event(UnitArrayProperty, EPropertyChangeType::ValueSet);
		Graph->PostEditChangeProperty(Event);
		Editor->NotifyGraphSettingsChanged(UnitArrayProperty);
		TestEqual(TEXT("Graph Settings respect the automatic ordering toggle"), Graph->GetUnitAt(0).ID, static_cast<uint32>(bAutoRearrange ? SecondID : FirstID));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilScalingOrderTest, "CrystalRecoil.Editor.ScalingOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilScalingOrderTest::RunTest(const FString& Parameters)
{
	for (const bool bAutoRearrange : { false, true })
	{
		UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>(CreatePackage(TEXT("/Game/CrystalRecoilScalingOrderTest")), NAME_None, RF_Transactional);
		UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
		const int32 FirstID = Graph->AddUnit(FVector2f::ZeroVector);
		const int32 SecondID = Graph->AddUnit(FVector2f(0.f, 1.f));
		const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
		Editor->SetPattern(Pattern);
		Editor->bEnableAutoRearrangeUnits = bAutoRearrange;
		Editor->GetRecoilUnitSelection().AddSelection(FirstID);
		Editor->GetRecoilUnitSelection().AddSelection(SecondID);
		const TSharedRef<FRecoilTestGraphWidget> Widget = SNew(FRecoilTestGraphWidget).RecoilPatternEditor(&Editor.Get());
		Widget->SetRecoilUnitGraph(Graph);
		Widget->BeginScaling(Graph, Editor->GetRecoilUnitSelection());
		const FPointerEvent MouseEvent(0, FVector2D(6.0, 0.0), FVector2D(1.0, 0.0), TSet<FKey>{ EKeys::LeftMouseButton }, EKeys::Invalid, 0.f, FModifierKeysState());
		Widget->OnMouseMove(FGeometry(), MouseEvent);
		TestEqual(TEXT("Scaling respects the automatic ordering toggle"), Graph->GetUnitAt(0).ID, static_cast<uint32>(bAutoRearrange ? SecondID : FirstID));
		TestEqual(TEXT("Scaling moved the second point across the first"), Graph->GetUnitByID(SecondID)->Position.Y, -0.25f);
		Widget->FinishDragging();
		GEditor->UndoTransaction();
		TestEqual(TEXT("Undo restores the original shot order"), Graph->GetUnitAt(0).ID, static_cast<uint32>(FirstID));
		TestEqual(TEXT("Undo restores the original position"), Graph->GetUnitByID(SecondID)->Position.Y, 1.f);
		GEditor->RedoTransaction();
		TestEqual(TEXT("Redo restores the scaled shot order"), Graph->GetUnitAt(0).ID, static_cast<uint32>(bAutoRearrange ? SecondID : FirstID));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilHeatCapTest, "CrystalRecoil.Runtime.HeatCap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilHeatCapTest::RunTest(const FString& Parameters)
{
	UCRRecoilSpreadComponent* Component = NewObject<UCRRecoilSpreadComponent>();
	Component->AddRecoilHeat(80.f);
	Component->SetMaxRecoilHeat(10.f);
	TestEqual(TEXT("Reducing the cap clamps existing heat immediately"), Component->GetRecoilHeat(), 10.f);
	Component->SetMaxRecoilHeat(100.f);
	TestEqual(TEXT("Increasing the cap preserves heat"), Component->GetRecoilHeat(), 10.f);
	Component->SetMaxRecoilHeat(-1.f);
	TestEqual(TEXT("A nonpositive cap clears heat"), Component->GetRecoilHeat(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilUpliftAimingTest, "CrystalRecoil.Runtime.UpliftAiming", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilUpliftAimingTest::RunTest(const FString& Parameters)
{
	for (const float RecoveryDelay : { 0.f, 0.1f })
	{
		FRecoilTestWorld Fixture;
		Fixture.Pattern->UpliftSpeed = 0.f;
		Fixture.Pattern->RecoveryDelay = RecoveryDelay;
		Fixture.Pattern->RecoveryCancelThreshold = 5.f;
		Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(5.f, 0.f));
		Fixture.Component->StartShooting();
		Fixture.Component->ApplyShot();
		Fixture.Advance(24);
		Fixture.Controller->SetControlRotation(Fixture.Controller->GetControlRotation() + FRotator(0.0, 10.0, 0.0));
		Fixture.Advance(180);
		TestTrue(FString::Printf(TEXT("Aiming during uplift cancels recovery (delay=%f)"), RecoveryDelay), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, 15.0, 0.001));
		TestFalse(TEXT("Canceled recovery stops ticking"), Fixture.Component->IsComponentTickEnabled());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilClipboardOrderTest, "CrystalRecoil.Editor.ClipboardOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilClipboardOrderTest::RunTest(const FString& Parameters)
{
#if PLATFORM_LINUX
	// The null RHI skips SDL initialization, which clipboard operations require.
	if (!TestTrue(TEXT("SDL initializes for the clipboard test"), FPlatformApplicationMisc::InitSDL()))
	{
		return false;
	}
#endif
	FString PreviousClipboard;
	FPlatformApplicationMisc::ClipboardPaste(PreviousClipboard);
	ON_SCOPE_EXIT
	{
		FPlatformApplicationMisc::ClipboardCopy(*PreviousClipboard);
	};
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>(CreatePackage(TEXT("/Game/CrystalRecoilClipboardOrderTest")), NAME_None, RF_Transactional);
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 FirstID = Graph->AddUnit(FVector2f(0.f, 1.f));
	Graph->AddUnit(FVector2f(0.f, 2.f));
	const int32 LastID = Graph->AddUnit(FVector2f(0.f, 3.f));
	const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
	Editor->SetPattern(Pattern);
	Editor->bEnableAutoRearrangeUnits = false;
	Editor->GetRecoilUnitSelection().AddSelection(LastID);
	Editor->GetRecoilUnitSelection().AddSelection(FirstID);
	const TSharedRef<FRecoilTestGraphWidget> Widget = SNew(FRecoilTestGraphWidget).RecoilPatternEditor(&Editor.Get());
	Widget->SetRecoilUnitGraph(Graph);
	Widget->CopySelectedUnits();
	Widget->PasteUnits();
	if (TestEqual(TEXT("Pasting adds the selected units"), Graph->GetUnitCount(), 5))
	{
		TestEqual(TEXT("Pasted units retain shot order regardless of selection order"), Graph->GetUnitAt(4).Position.Y - Graph->GetUnitAt(3).Position.Y, 2.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilLaterShotTest, "CrystalRecoil.Runtime.LaterShot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilLaterShotTest::RunTest(const FString& Parameters)
{
	FRecoilTestWorld Fixture;
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(1.f, 0.f));
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(3.f, 0.f));
	Fixture.Component->StartShooting();
	Fixture.Component->ApplyShot();
	Fixture.Advance(120);
	TestFalse(TEXT("Recovery stops ticking"), Fixture.Component->IsComponentTickEnabled());
	Fixture.Pattern->RecoveryDelay = 1.f;
	Fixture.Controller->SetControlRotation(FRotator(0.0, -10.0, 0.0));
	Fixture.Component->ApplyShot();
	TestTrue(TEXT("The next shot resumes ticking"), Fixture.Component->IsComponentTickEnabled());
	Fixture.Advance(12);
	TestTrue(TEXT("The next pattern delta is applied without restarting the sequence"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, -8.0, 0.001));
	Fixture.Advance(240);
	TestTrue(TEXT("Idle aiming does not compensate the next shot"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, -10.0, 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilYawCompensationTest, "CrystalRecoil.Runtime.YawCompensation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilYawCompensationTest::RunTest(const FString& Parameters)
{
	for (const float RecoilYaw : { -5.f, 5.f })
	{
		for (const bool bCounterSteering : { false, true })
		{
			FRecoilTestWorld Fixture;
			Fixture.Pattern->RecoveryDelay = 0.5f;
			Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(RecoilYaw, 0.f));
			Fixture.Component->StartShooting();
			Fixture.Component->ApplyShot();
			Fixture.Advance(24);
			TestTrue(TEXT("Yaw uplift completes"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, static_cast<double>(RecoilYaw), 0.001));
			const double PlayerYaw = FMath::Sign(RecoilYaw) * (bCounterSteering ? -2.0 : 2.0);
			Fixture.Controller->SetControlRotation(Fixture.Controller->GetControlRotation() + FRotator(0.0, PlayerYaw, 0.0));
			Fixture.Advance(240);
			const double ExpectedYaw = bCounterSteering ? 0.0 : PlayerYaw;
			TestTrue(TEXT("Recovery respects manual yaw in both directions"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, ExpectedYaw, 0.001));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilCompensationResidueTest, "CrystalRecoil.Runtime.CompensationResidue", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilCompensationResidueTest::RunTest(const FString& Parameters)
{
	FRecoilTestWorld Fixture;
	Fixture.Pattern->RecoveryDelay = 0.5f;
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(5.f, 0.f));
	Fixture.Component->StartShooting();
	Fixture.Component->ApplyShot();
	Fixture.Advance(24);
	Fixture.Controller->SetControlRotation(FRotator(0.0, 0.0005, 0.0));
	Fixture.Advance(120);
	TestFalse(TEXT("Nearly complete compensation stops ticking"), Fixture.Component->IsComponentTickEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilActiveDragUndoTest, "CrystalRecoil.Editor.ActiveDragUndo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilActiveDragUndoTest::RunTest(const FString& Parameters)
{
	for (const bool bScaling : { false, true })
	{
		for (const bool bUndoOtherGraph : { false, true })
		{
			UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>(CreatePackage(TEXT("/Game/CrystalRecoilActiveDragUndoTest")), NAME_None, RF_Transactional);
			UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
			const int32 FirstID = Graph->AddUnit(FVector2f(0.f, 1.f));
			int32 SecondID;
			{
				FScopedTransaction Transaction(NSLOCTEXT("CrystalRecoilTests", "AddUnit", "Add recoil unit"));
				Graph->Modify();
				SecondID = Graph->AddUnit(FVector2f(0.f, 2.f));
			}
			UCRRecoilUnitGraph* UndoGraph = Graph;
			if (bUndoOtherGraph)
			{
				UndoGraph = NewObject<UCRRecoilUnitGraph>(Pattern, NAME_None, RF_Transactional);
				FScopedTransaction Transaction(NSLOCTEXT("CrystalRecoilTests", "AddOtherUnit", "Add unit to other graph"));
				UndoGraph->Modify();
				UndoGraph->AddUnit(FVector2f::ZeroVector);
			}
			FCRRecoilUnitSelection Selection;
			Selection.AddSelection(FirstID);
			Selection.AddSelection(SecondID);
			const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
			Editor->SetPattern(Pattern);
			const TSharedRef<FRecoilTestGraphWidget> Widget = SNew(FRecoilTestGraphWidget).RecoilPatternEditor(&Editor.Get());
			Widget->SetRecoilUnitGraph(Graph);
			Widget->BeginDrag(Graph, Selection, bScaling);
			GEditor->UndoTransaction();
			TestFalse(TEXT("Undo ends the active drag"), Widget->HasActiveDrag());
			TestEqual(TEXT("Undo restores the preview position"), Graph->GetUnitByID(FirstID)->Position.Y, 1.f);
			if (bUndoOtherGraph)
			{
				TestEqual(TEXT("Undo on another graph cancels the scaled point"), Graph->GetUnitByID(SecondID)->Position.Y, 2.f);
			}
			TestEqual(TEXT("Undo removes the last committed addition"), UndoGraph->GetUnitCount(), bUndoOtherGraph ? 0 : 1);
			GEditor->RedoTransaction();
			TestEqual(TEXT("Redo preserves the original preview position"), Graph->GetUnitByID(FirstID)->Position.Y, 1.f);
			TestEqual(TEXT("Redo preserves the second preview position"), Graph->GetUnitByID(SecondID)->Position.Y, 2.f);
			TestEqual(TEXT("Redo restores the addition"), UndoGraph->GetUnitCount(), bUndoOtherGraph ? 1 : 2);
			const int32 NextID = Graph->AddUnit(FVector2f(0.f, 4.f));
			TestTrue(TEXT("Canceled drag preserves the next unique point ID"), NextID != FirstID && NextID != SecondID);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilUndoDetailsTest, "CrystalRecoil.Editor.UndoDetails", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilUndoDetailsTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>(CreatePackage(TEXT("/Game/CrystalRecoilUndoDetailsTest")), NAME_None, RF_Transactional);
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 UnitID = Graph->AddUnit(FVector2f(0.f, 1.f));
	const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
	Editor->SetPattern(Pattern);
	Editor->BuildTab_UnitDetails();
	Editor->GetRecoilUnitSelection().AddSelection(UnitID);
	{
		FScopedTransaction Transaction(NSLOCTEXT("CrystalRecoilTests", "MoveSelectedUnit", "Move selected recoil unit"));
		Graph->Modify();
		Graph->GetUnitByID(UnitID)->Position.Y = 3.f;
	}
	Editor->RefreshUnitPosition();
	TestEqual(TEXT("Details show the moved position"), Editor->GetSelectedPosition().Y, 3.f);
	GEditor->UndoTransaction();
	TestEqual(TEXT("Undo refreshes the details copy"), Editor->GetSelectedPosition().Y, 1.f);
	GEditor->RedoTransaction();
	TestEqual(TEXT("Redo refreshes the details copy"), Editor->GetSelectedPosition().Y, 3.f);
	int32 AddedID;
	{
		FScopedTransaction Transaction(NSLOCTEXT("CrystalRecoilTests", "AddSelectedUnit", "Add selected recoil unit"));
		Graph->Modify();
		AddedID = Graph->AddUnit(FVector2f(0.f, 4.f));
	}
	Editor->GetRecoilUnitSelection().ClearSelection();
	Editor->GetRecoilUnitSelection().AddSelection(AddedID);
	GEditor->UndoTransaction();
	TestTrue(TEXT("Undo prunes a removed point from selection"), Editor->GetRecoilUnitSelection().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilGraphSettingsTest, "CrystalRecoil.Editor.GraphSettings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilGraphSettingsTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>();
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 UnitID = Graph->AddUnit(FVector2f(0.f, 1.f));
	const TSharedRef<FRecoilTestPatternEditor> Editor = MakeShared<FRecoilTestPatternEditor>();
	Editor->SetPattern(Pattern);
	Editor->BuildTab_UnitDetails();
	Editor->BuildTab_UnitGraphDetails();
	Editor->GetRecoilUnitSelection().AddSelection(UnitID);
	Graph->GetUnitByID(UnitID)->Position.Y = 3.f;
	Editor->NotifyGraphSettingsChanged();
	TestEqual(TEXT("Graph Settings refresh the selected position"), Editor->GetSelectedPosition().Y, 3.f);
	Graph->RemoveUnit(UnitID);
	Editor->NotifyGraphSettingsChanged();
	TestTrue(TEXT("Graph Settings prune deleted selection IDs"), Editor->GetRecoilUnitSelection().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilControllerChangesTest, "CrystalRecoil.Runtime.ControllerChanges", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilControllerChangesTest::RunTest(const FString& Parameters)
{
	for (const bool bDestroyTarget : { false, true })
	{
		for (const bool bApplyBeforeSwitch : { false, true })
		{
			FRecoilTestWorld Fixture;
			Fixture.Pattern->RecoveryDelay = 0.5f;
			Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(5.f, 0.f));
			Fixture.Component->StartShooting();
			Fixture.Component->ApplyShot();
			if (bApplyBeforeSwitch)
			{
				Fixture.Advance(24);
			}
			APlayerController* NewController = Fixture.World->SpawnActor<APlayerController>();
			NewController->SetAsLocalPlayerController();
			Fixture.World->AddController(NewController);
			if (bDestroyTarget)
			{
				TestTrue(TEXT("The original controller is registered for fallback"), Fixture.World->GetFirstPlayerController() == Fixture.Controller);
				TestTrue(TEXT("The original controller is destroyed"), Fixture.Controller->Destroy());
				TestTrue(TEXT("The replacement controller becomes the fallback"), Fixture.World->GetFirstPlayerController() == NewController);
			}
			else
			{
				Fixture.Component->SetTargetController(NewController);
			}
			Fixture.Advance(120);
			TestTrue(TEXT("The new controller inherits neither uplift nor recovery"), FMath::IsNearlyZero(NewController->GetControlRotation().Yaw, 0.001));
			Fixture.Component->ApplyShot();
			Fixture.Advance(24);
			TestTrue(FString::Printf(TEXT("A new shot applies to the reassigned controller (destroy=%d, applied=%d, yaw=%f)"), bDestroyTarget, bApplyBeforeSwitch, NewController->GetControlRotation().Yaw), FMath::IsNearlyEqual(NewController->GetControlRotation().Yaw, 5.0, 0.001));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilExternalRotationTest, "CrystalRecoil.Runtime.ExternalControlRotation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilExternalRotationTest::RunTest(const FString& Parameters)
{
	for (const double TrackingYaw : { -2.0, 30.0 })
	{
		FRecoilTestWorld Fixture;
		Fixture.Pattern->RecoveryDelay = 0.5f;
		Fixture.Pattern->RecoveryCancelThreshold = 0.1f;
		Fixture.Pattern->InitialRecoverySpeed = 90.f;
		Fixture.Pattern->MaxRecoverySpeed = 90.f;
		Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(5.f, 0.f));
		Fixture.Component->StartShooting();
		Fixture.Component->ApplyShot();
		Fixture.Advance(12);
		const FRotator BeforeTracking = Fixture.Controller->GetControlRotation();
		Fixture.Controller->SetControlRotation(BeforeTracking + FRotator(0.f, TrackingYaw, 0.f));
		Fixture.Component->NotifyExternalControlRotationDelta(Fixture.Controller, (Fixture.Controller->GetControlRotation() - BeforeTracking).GetNormalized());
		Fixture.Advance(120);
		TestTrue(TEXT("Automatic tracking survives recovery without compensating or canceling it"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, TrackingYaw, 0.001));
	}

	FRecoilTestWorld Fixture;
	Fixture.Pattern->RecoveryDelay = 0.5f;
	Fixture.Pattern->RecoveryCancelThreshold = 0.1f;
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(5.f, 0.f));
	Fixture.Component->StartShooting();
	Fixture.Component->ApplyShot();
	Fixture.Advance(12);
	Fixture.Controller->SetControlRotation(FRotator(0.f, 3.f, 0.f));
	APlayerController* OtherController = Fixture.World->SpawnActor<APlayerController>();
	Fixture.Component->NotifyExternalControlRotationDelta(OtherController, FRotator(0.f, -2.f, 0.f));
	Fixture.Advance(120);
	TestTrue(TEXT("Notifications for another controller cannot mask manual recovery cancellation"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, 3.0, 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilScaledHookTest, "CrystalRecoil.Runtime.ScaledHook", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilScaledHookTest::RunTest(const FString& Parameters)
{
	FRecoilTestWorld Fixture(UCRRecoilScaledTestComponent::StaticClass());
	Fixture.Pattern->RecoveryDelay = 2.f;
	Fixture.Pattern->GetUnitGraph()->AddUnit(FVector2f(10.f, 0.f));
	Fixture.Component->StartShooting();
	Fixture.Component->ApplyShot();
	Fixture.Advance(60);
	TestTrue(TEXT("Scaling the hook halves total recoil rather than slowing it"), FMath::IsNearlyEqual(Fixture.Controller->GetControlRotation().Yaw, 5.0, 0.001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilGraphOrderTest, "CrystalRecoil.Editor.GraphOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilGraphOrderTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>();
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 HighID = Graph->AddUnit(FVector2f(0.0015f, 0.0015f));
	const int32 MiddleID = Graph->AddUnit(FVector2f(0.00075f, 0.00075f));
	const int32 LowID = Graph->AddUnit(FVector2f::ZeroVector);
	for (const ECRRecoilUnitGraphRearrangePolicy Policy : { ECRRecoilUnitGraphRearrangePolicy::AscendByX, ECRRecoilUnitGraphRearrangePolicy::AscendByY, ECRRecoilUnitGraphRearrangePolicy::DescendByX, ECRRecoilUnitGraphRearrangePolicy::DescendByY })
	{
		Graph->RearrangePolicy = Policy;
		Graph->RearrangeUnits();
		const bool bAscending = Policy == ECRRecoilUnitGraphRearrangePolicy::AscendByX || Policy == ECRRecoilUnitGraphRearrangePolicy::AscendByY;
		TestEqual(TEXT("First coordinate is ordered"), Graph->GetUnitAt(0).ID, static_cast<uint32>(bAscending ? LowID : HighID));
		TestEqual(TEXT("Middle coordinate is ordered"), Graph->GetUnitAt(1).ID, static_cast<uint32>(MiddleID));
		TestEqual(TEXT("Last coordinate is ordered"), Graph->GetUnitAt(2).ID, static_cast<uint32>(bAscending ? HighID : LowID));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilMoveUndoTest, "CrystalRecoil.Editor.MoveUndo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilMoveUndoTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>(CreatePackage(TEXT("/Game/CrystalRecoilUndoTest")), NAME_None, RF_Transactional);
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 FirstID = Graph->AddUnit(FVector2f(0.f, 1.f));
	const int32 SecondID = Graph->AddUnit(FVector2f(0.f, 2.f));
	FCRRecoilUnitSelection Selection;
	Selection.AddSelection(FirstID);
	{
		FCRUnitGraphMoveUnitsDelayedDrag Drag(Graph, Selection, FVector2f(0.f, 1.f), FVector2f::ZeroVector, EKeys::LeftMouseButton);
		Drag.ApplyMovement(Selection, FVector2f(0.f, 2.f));
		Graph->RearrangeUnits();
	}
	TestEqual(TEXT("Movement rearranges shot order"), Graph->GetUnitAt(0).ID, static_cast<uint32>(SecondID));
	GEditor->UndoTransaction();
	TestEqual(TEXT("Undo restores the original first shot"), Graph->GetUnitAt(0).ID, static_cast<uint32>(FirstID));
	TestEqual(TEXT("Undo restores its position"), Graph->GetUnitAt(0).Position.Y, 1.f);
	GEditor->RedoTransaction();
	TestEqual(TEXT("Redo restores the moved order"), Graph->GetUnitAt(0).ID, static_cast<uint32>(SecondID));
	TestEqual(TEXT("Redo restores the moved position"), Graph->GetUnitByID(FirstID)->Position.Y, 3.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCRRecoilFiniteScalingTest, "CrystalRecoil.Editor.FiniteScaling", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCRRecoilFiniteScalingTest::RunTest(const FString& Parameters)
{
	UCRRecoilPattern* Pattern = NewObject<UCRRecoilPattern>();
	UCRRecoilUnitGraph* Graph = Pattern->GetUnitGraph();
	const int32 FirstID = Graph->AddUnit(FVector2f(-1.f, 0.f));
	const int32 SecondID = Graph->AddUnit(FVector2f(1.f, 0.f));
	FCRRecoilUnitSelection Selection;
	Selection.AddSelection(FirstID);
	Selection.AddSelection(SecondID);
	FCRUnitGraphScaleUnitsDelayedDrag Drag(Graph, Selection, FVector2f::ZeroVector, FVector2f::ZeroVector, EKeys::LeftMouseButton);
	Drag.ApplyScaling(Selection, std::numeric_limits<float>::infinity());
	Drag.ApplyScaling(Selection, std::numeric_limits<float>::quiet_NaN());
	TestEqual(TEXT("Invalid scaling preserves the point"), Graph->GetUnitByID(SecondID)->Position, FVector2f(1.f, 0.f));
	Drag.ApplyScaling(Selection, 2.f);
	TestEqual(TEXT("Finite scaling still applies"), Graph->GetUnitByID(SecondID)->Position, FVector2f(2.f, 0.f));
	return true;
}

#endif
