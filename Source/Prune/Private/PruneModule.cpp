// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneModule.h"

#include "PruneDetailsCustomization.h"
#include "PruneState.h"

#include "ActorDetailsDelegates.h"
#include "CoreGlobals.h"
#include "DetailLayoutBuilder.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogPruneModule, Log, All);

void FPruneModule::StartupModule()
{
    if (ActorDetailsExtensionHandle.IsValid() || IsRunningCommandlet())
    {
        return;
    }

    State = MakeShared<FPruneState>();

    // ActorDetailsDelegates.h belongs to DetailCustomizations. Load that module
    // first so Unreal's stock FActorDetails and its extension delegate exist.
    FModuleManager::LoadModuleChecked<IModuleInterface>(
        TEXT("DetailCustomizations"));

    const TSharedRef<FPruneState> StateRef = State.ToSharedRef();

    ActorDetailsExtensionHandle = OnExtendActorDetails.AddLambda(
        [StateRef](
            IDetailLayoutBuilder& DetailBuilder,
            const FGetSelectedActors& GetSelectedActors)
        {
            if (!GetSelectedActors.IsBound())
            {
                return;
            }

            FPruneDetailsCustomization::ExtendActorDetails(
                StateRef,
                DetailBuilder,
                GetSelectedActors.Execute());
        });

    // The delegate affects future Actor layout builds. Force currently-open
    // Details views through the normal customization refresh once as well.
    FPropertyEditorModule& PropertyEditor =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
            TEXT("PropertyEditor"));

    PropertyEditor.NotifyCustomizationModuleChanged();

    // SDetailsView rebuilds its native section SWrapBox independently of the
    // Actor detail layout. Keep one lightweight guard running so Prune's
    // management control is restored after any native section-row rebuild.
    // The scan is limited to live Actor Details contexts and is a no-op while
    // the control is already present.
    const TWeakPtr<FPruneState> WeakState = State;
    ManagementUiTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda(
            [WeakState](float)
            {
                if (const TSharedPtr<FPruneState> LiveState = WeakState.Pin())
                {
                    FPruneDetailsCustomization::EnsureManagementButtons(
                        LiveState.ToSharedRef());
                    return true;
                }

                return false;
            }),
        0.25f);

    UE_LOG(
        LogPruneModule,
        Log,
        TEXT("Prune registered through OnExtendActorDetails."));
}

void FPruneModule::ShutdownModule()
{
    if (ManagementUiTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(ManagementUiTickerHandle);
        ManagementUiTickerHandle.Reset();
    }

    if (ActorDetailsExtensionHandle.IsValid()
        && FModuleManager::Get().IsModuleLoaded(TEXT("DetailCustomizations")))
    {
        OnExtendActorDetails.Remove(ActorDetailsExtensionHandle);
        ActorDetailsExtensionHandle.Reset();
    }

    if (State.IsValid())
    {
        State->RemoveAllNativeSections();
    }
    State.Reset();

    if (!IsEngineExitRequested())
    {
        if (FPropertyEditorModule* PropertyEditor =
            FModuleManager::GetModulePtr<FPropertyEditorModule>(
                TEXT("PropertyEditor")))
        {
            PropertyEditor->NotifyCustomizationModuleChanged();
        }
    }
}

IMPLEMENT_MODULE(FPruneModule, Prune)
