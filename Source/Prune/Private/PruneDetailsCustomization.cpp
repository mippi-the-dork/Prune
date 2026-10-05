// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "IPropertyUtilities.h"
#include "PruneState.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "PruneDetailsCustomization"

DEFINE_LOG_CATEGORY_STATIC(LogPruneDetails, Log, All);

namespace PruneDetailsCustomizationPrivate
{
    static const FName PruneControlCategoryName(TEXT("PruneControl"));

    /**
     * Reproduce Unreal's standard no-SortCategories ordering before Prune
     * reads the finished category map.
     *
     * UE 5.8 normally sorts simple categories and advanced-only categories
     * separately, then appends the advanced-only block. Merely registering a
     * SortCategories callback switches the engine to a single combined sort.
     * Prune needs the callback for finished-category discovery, so restore the
     * standard grouping here using only public IDetailCategoryBuilder APIs.
     *
     * A category is considered advanced-only when it exposes no simple default
     * properties and at least one advanced default property. Categories made
     * entirely from custom rows/builders are treated as simple. That matches
     * Unreal's built-in Actor Details categories used by the prototype and
     * keeps custom plugin categories such as Surface in the normal block.
     */
    static void RestoreStandardCategoryGrouping(
        const TMap<FName, IDetailCategoryBuilder*>& Categories)
    {
        struct FCategoryOrderEntry
        {
            IDetailCategoryBuilder* Category = nullptr;
            int32 OriginalSortOrder = 0;
            FName Id = NAME_None;
        };

        TArray<FCategoryOrderEntry> SimpleCategories;
        TArray<FCategoryOrderEntry> AdvancedOnlyCategories;

        SimpleCategories.Reserve(Categories.Num());
        AdvancedOnlyCategories.Reserve(Categories.Num());

        for (const TPair<FName, IDetailCategoryBuilder*>& Entry : Categories)
        {
            IDetailCategoryBuilder* Category = Entry.Value;

            if (Category == nullptr)
            {
                continue;
            }

            TArray<TSharedRef<IPropertyHandle>> SimpleProperties;
            TArray<TSharedRef<IPropertyHandle>> AdvancedProperties;

            Category->GetDefaultProperties(
                SimpleProperties,
                true,
                false);

            Category->GetDefaultProperties(
                AdvancedProperties,
                false,
                true);

            FCategoryOrderEntry OrderEntry;
            OrderEntry.Category = Category;
            OrderEntry.OriginalSortOrder = Category->GetSortOrder();
            OrderEntry.Id = Entry.Key;

            const bool bAdvancedOnly =
                SimpleProperties.IsEmpty()
                && !AdvancedProperties.IsEmpty();

            if (bAdvancedOnly)
            {
                AdvancedOnlyCategories.Add(OrderEntry);
            }
            else
            {
                SimpleCategories.Add(OrderEntry);
            }
        }

        const auto SortByOriginalOrder =
            [](const FCategoryOrderEntry& A, const FCategoryOrderEntry& B)
            {
                if (A.OriginalSortOrder != B.OriginalSortOrder)
                {
                    return A.OriginalSortOrder < B.OriginalSortOrder;
                }

                return A.Id.ToString() < B.Id.ToString();
            };

        SimpleCategories.Sort(SortByOriginalOrder);
        AdvancedOnlyCategories.Sort(SortByOriginalOrder);

        int32 NewSortOrder = 0;

        for (FCategoryOrderEntry& Entry : SimpleCategories)
        {
            Entry.Category->SetSortOrder(NewSortOrder++);
        }

        for (FCategoryOrderEntry& Entry : AdvancedOnlyCategories)
        {
            Entry.Category->SetSortOrder(NewSortOrder++);
        }
    }

    struct FPruneSelectionIdentity
    {
        FString Key;
        FText Label;
    };

    static FString GetStableActorClassKey(const UClass* ActorClass)
    {
        if (ActorClass == nullptr)
        {
            return FString();
        }

        // Blueprint-generated classes can be transiently reinstanced during a
        // compile. Use the generating asset path when available so the session
        // filter survives that reinstance instead of creating a REINST_* key.
        if (const UObject* GeneratedBy = ActorClass->ClassGeneratedBy)
        {
            return FString::Printf(
                TEXT("Blueprint:%s"),
                *GeneratedBy->GetPathName());
        }

        return FString::Printf(
            TEXT("Class:%s"),
            *ActorClass->GetPathName());
    }

    static FPruneSelectionIdentity BuildSelectionIdentity(
        const TArray<TWeakObjectPtr<AActor>>& SelectedActors)
    {
        TArray<FString> ClassKeys;
        const UClass* FirstClass = nullptr;

        for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
        {
            const AActor* Actor = WeakActor.Get();
            const UClass* ActorClass = Actor ? Actor->GetClass() : nullptr;

            if (ActorClass == nullptr)
            {
                continue;
            }

            if (FirstClass == nullptr)
            {
                FirstClass = ActorClass;
            }

            ClassKeys.AddUnique(GetStableActorClassKey(ActorClass));
        }

        ClassKeys.Remove(FString());
        ClassKeys.Sort();

        FPruneSelectionIdentity Identity;

        if (ClassKeys.Num() == 1)
        {
            Identity.Key = ClassKeys[0];
            Identity.Label = FirstClass
                ? FirstClass->GetDisplayNameText()
                : LOCTEXT("UnknownActorClass", "Actor");
        }
        else
        {
            Identity.Key = FString::Printf(
                TEXT("Mixed:%s"),
                *FString::Join(ClassKeys, TEXT("|")));
            Identity.Label = FText::Format(
                LOCTEXT(
                    "MixedSelectionLabelFmt",
                    "Mixed Selection ({0} Classes)"),
                FText::AsNumber(ClassKeys.Num()));
        }

        return Identity;
    }

    static TOptional<FString> PromptForPresetName()
    {
        TSharedPtr<SEditableTextBox> NameTextBox;
        bool bAccepted = false;

        TSharedRef<SWindow> Dialog =
            SNew(SWindow)
            .Title(LOCTEXT("PresetNameDialogTitle", "Save Prune Preset"))
            .SizingRule(ESizingRule::Autosized)
            .SupportsMaximize(false)
            .SupportsMinimize(false);

        Dialog->SetContent(
            SNew(SBorder)
            .Padding(12.0f)
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT(
                        "PresetNamePrompt",
                        "Name this class-agnostic category preset."))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 8.0f, 0.0f, 10.0f)
                [
                    SAssignNew(NameTextBox, SEditableTextBox)
                    .MinDesiredWidth(300.0f)
                    .SelectAllTextWhenFocused(true)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Right)
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("PresetCancel", "Cancel"))
                        .OnClicked_Lambda([Dialog]()
                        {
                            Dialog->RequestDestroyWindow();
                            return FReply::Handled();
                        })
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("PresetSave", "Save"))
                        .IsEnabled_Lambda([&NameTextBox]()
                        {
                            return NameTextBox.IsValid()
                                && !NameTextBox->GetText().ToString()
                                    .TrimStartAndEnd().IsEmpty();
                        })
                        .OnClicked_Lambda([Dialog, &NameTextBox, &bAccepted]()
                        {
                            if (NameTextBox.IsValid()
                                && !NameTextBox->GetText().ToString()
                                    .TrimStartAndEnd().IsEmpty())
                            {
                                bAccepted = true;
                                Dialog->RequestDestroyWindow();
                            }

                            return FReply::Handled();
                        })
                    ]
                ]
            ]);

        FSlateApplication::Get().AddModalWindow(
            Dialog,
            FSlateApplication::Get().GetActiveTopLevelWindow(),
            false);

        if (!bAccepted || !NameTextBox.IsValid())
        {
            return TOptional<FString>();
        }

        return NameTextBox->GetText().ToString().TrimStartAndEnd();
    }

    static TSharedRef<SWidget> BuildPresetMenu(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<FPruneLayoutContext>& WeakLayoutContext)
    {
        const TSharedPtr<FPruneState> PinnedState = WeakState.Pin();
        const TSharedPtr<FPruneLayoutContext> PinnedContext =
            WeakLayoutContext.Pin();

        const FString SelectionKey = PinnedContext.IsValid()
            ? PinnedContext->GetSelectionKey()
            : FString();

        const TArray<FPrunePresetSummary> Presets = PinnedState.IsValid()
            ? PinnedState->GetPresets()
            : TArray<FPrunePresetSummary>();

        TSharedRef<SVerticalBox> List = SNew(SVerticalBox);

        List->AddSlot()
        .AutoHeight()
        .Padding(6.0f, 2.0f)
        [
            SNew(SCheckBox)
            .ToolTipText(LOCTEXT(
                "AllCategoriesPresetTooltip",
                "Show every category and clear the saved filter for this Actor class."))
            .IsChecked_Lambda([WeakState, SelectionKey]()
            {
                const TSharedPtr<FPruneState> LiveState = WeakState.Pin();
                return LiveState.IsValid()
                    && LiveState->GetActivePresetId(SelectionKey).IsEmpty()
                    && !LiveState->IsSelectionCustom(SelectionKey)
                    ? ECheckBoxState::Checked
                    : ECheckBoxState::Unchecked;
            })
            .OnCheckStateChanged_Lambda(
                [WeakState, SelectionKey](ECheckBoxState NewState)
                {
                    if (NewState == ECheckBoxState::Checked)
                    {
                        if (const TSharedPtr<FPruneState> LiveState =
                            WeakState.Pin())
                        {
                            LiveState->ShowAllCategories(SelectionKey);
                        }
                        FSlateApplication::Get().DismissAllMenus();
                    }
                })
            [
                SNew(STextBlock)
                .Text(LOCTEXT("AllCategoriesPreset", "All Categories"))
            ]
        ];

        if (!Presets.IsEmpty())
        {
            List->AddSlot()
            .AutoHeight()
            .Padding(4.0f, 3.0f)
            [
                SNew(SSeparator)
            ];
        }

        for (const FPrunePresetSummary& Preset : Presets)
        {
            const FString PresetId = Preset.Id;
            const FText PresetName = Preset.Name;

            List->AddSlot()
            .AutoHeight()
            .Padding(6.0f, 2.0f)
            [
                SNew(SCheckBox)
                .ToolTipText(LOCTEXT(
                    "NamedPresetTooltip",
                    "Apply this class-agnostic category deny-list to the current Actor class."))
                .IsChecked_Lambda([WeakState, SelectionKey, PresetId]()
                {
                    const TSharedPtr<FPruneState> LiveState = WeakState.Pin();
                    return LiveState.IsValid()
                        && LiveState->GetActivePresetId(SelectionKey) == PresetId
                        ? ECheckBoxState::Checked
                        : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda(
                    [WeakState, SelectionKey, PresetId](
                        ECheckBoxState NewState)
                    {
                        if (NewState == ECheckBoxState::Checked)
                        {
                            if (const TSharedPtr<FPruneState> LiveState =
                                WeakState.Pin())
                            {
                                LiveState->ApplyPreset(SelectionKey, PresetId);
                            }
                            FSlateApplication::Get().DismissAllMenus();
                        }
                    })
                [
                    SNew(STextBlock)
                    .Text(PresetName)
                ]
            ];
        }

        List->AddSlot()
        .AutoHeight()
        .Padding(4.0f, 3.0f)
        [
            SNew(SSeparator)
        ];

        List->AddSlot()
        .AutoHeight()
        .Padding(6.0f, 2.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("SaveCurrentPreset", "Save Current as Preset..."))
            .ToolTipText(LOCTEXT(
                "SaveCurrentPresetTooltip",
                "Save the current hidden-category rules as a reusable class-agnostic preset."))
            .OnClicked_Lambda([WeakState, SelectionKey]()
            {
                FSlateApplication::Get().DismissAllMenus();

                const TOptional<FString> PresetName = PromptForPresetName();
                if (!PresetName.IsSet())
                {
                    return FReply::Handled();
                }

                if (const TSharedPtr<FPruneState> LiveState = WeakState.Pin())
                {
                    FString Error;
                    if (!LiveState->SaveCurrentAsPreset(
                        SelectionKey,
                        PresetName.GetValue(),
                        Error))
                    {
                        FMessageDialog::Open(
                            EAppMsgType::Ok,
                            FText::FromString(Error));
                    }
                }

                return FReply::Handled();
            })
        ];

        const FString ActivePresetId = PinnedState.IsValid()
            ? PinnedState->GetActivePresetId(SelectionKey)
            : FString();

        if (!ActivePresetId.IsEmpty())
        {
            List->AddSlot()
            .AutoHeight()
            .Padding(6.0f, 2.0f)
            [
                SNew(SButton)
                .Text(LOCTEXT("DeleteActivePreset", "Delete Active Preset..."))
                .ToolTipText(LOCTEXT(
                    "DeleteActivePresetTooltip",
                    "Delete the active named preset. Current category visibility is preserved and affected classes become Custom."))
                .OnClicked_Lambda([WeakState, ActivePresetId]()
                {
                    FSlateApplication::Get().DismissAllMenus();

                    if (FMessageDialog::Open(
                        EAppMsgType::YesNo,
                        LOCTEXT(
                            "DeletePresetConfirm",
                            "Delete the active Prune preset? Current hidden categories will be preserved as Custom state."))
                        == EAppReturnType::Yes)
                    {
                        if (const TSharedPtr<FPruneState> LiveState =
                            WeakState.Pin())
                        {
                            LiveState->DeletePreset(ActivePresetId);
                        }
                    }

                    return FReply::Handled();
                })
            ];
        }

        return SNew(SBox)
            .MinDesiredWidth(280.0f)
            .MaxDesiredHeight(520.0f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    List
                ]
            ];
    }

    static FText BuildCategoryIdsTooltip(const TArray<FName>& CategoryIds)
    {
        TArray<FString> IdStrings;
        IdStrings.Reserve(CategoryIds.Num());

        for (const FName CategoryId : CategoryIds)
        {
            IdStrings.Add(CategoryId.ToString());
        }

        return FText::Format(
            CategoryIds.Num() == 1
                ? LOCTEXT(
                    "CategoryTooltipFmt",
                    "Internal category ID: {0}")
                : LOCTEXT(
                    "CategoryGroupTooltipFmt",
                    "Internal category IDs: {0}"),
            FText::FromString(FString::Join(IdStrings, TEXT(", "))));
    }

    static TSharedRef<SWidget> BuildCategoryMenu(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<FPruneLayoutContext>& WeakLayoutContext)
    {
        const TSharedPtr<FPruneLayoutContext> PinnedContext =
            WeakLayoutContext.Pin();

        const TArray<FPruneCategoryGroupInfo> CategoryGroups =
            PinnedContext.IsValid()
                ? PinnedContext->GetCategoryGroups()
                : TArray<FPruneCategoryGroupInfo>();

        const FString SelectionKey = PinnedContext.IsValid()
            ? PinnedContext->GetSelectionKey()
            : FString();

        TSharedRef<SVerticalBox> List = SNew(SVerticalBox);

        if (CategoryGroups.IsEmpty())
        {
            List->AddSlot()
            .AutoHeight()
            .Padding(8.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT(
                    "NoCategories",
                    "No Actor Details categories are currently available."))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ];
        }
        else
        {
            for (const FPruneCategoryGroupInfo& Group : CategoryGroups)
            {
                const TArray<FName> CategoryIds = Group.Ids;
                const FText CategoryLabel = Group.DisplayName;

                List->AddSlot()
                .AutoHeight()
                .Padding(6.0f, 2.0f)
                [
                    SNew(SCheckBox)
                    .ToolTipText(BuildCategoryIdsTooltip(CategoryIds))
                    .IsChecked_Lambda(
                        [WeakState, SelectionKey, CategoryIds]()
                        {
                            const TSharedPtr<FPruneState> LiveState =
                                WeakState.Pin();

                            if (!LiveState.IsValid())
                            {
                                return ECheckBoxState::Unchecked;
                            }

                            if (LiveState->AreAllCategoriesVisible(
                                SelectionKey,
                                CategoryIds))
                            {
                                return ECheckBoxState::Checked;
                            }

                            if (LiveState->AreAnyCategoriesVisible(
                                SelectionKey,
                                CategoryIds))
                            {
                                return ECheckBoxState::Undetermined;
                            }

                            return ECheckBoxState::Unchecked;
                        })
                    .OnCheckStateChanged_Lambda(
                        [WeakState, SelectionKey, CategoryIds](
                            ECheckBoxState NewState)
                        {
                            if (const TSharedPtr<FPruneState> LiveState =
                                WeakState.Pin())
                            {
                                LiveState->SetCategoriesVisible(
                                    SelectionKey,
                                    CategoryIds,
                                    NewState == ECheckBoxState::Checked);
                            }
                        })
                    [
                        SNew(STextBlock)
                        .Text(CategoryLabel)
                    ]
                ];
            }
        }

        return SNew(SBox)
            .MinDesiredWidth(280.0f)
            .MaxDesiredHeight(520.0f)
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        List
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.0f, 3.0f)
                [
                    SNew(SSeparator)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(6.0f, 2.0f)
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("ShowAll", "Show All"))
                        .ToolTipText(LOCTEXT(
                            "ShowAllTooltip",
                            "Show every category for this selection. Exact Actor-class state is saved per user/project; mixed selections remain session-only."))
                        .OnClicked_Lambda(
                            [WeakState, SelectionKey]()
                            {
                                if (const TSharedPtr<FPruneState> LiveState =
                                    WeakState.Pin())
                                {
                                    LiveState->ShowAllCategories(SelectionKey);
                                }
                                return FReply::Handled();
                            })
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("LogCategories", "Log IDs"))
                        .ToolTipText(LOCTEXT(
                            "LogCategoriesTooltip",
                            "Write the complete category set, internal IDs, display labels, selection key, and Prune visibility to the Output Log."))
                        .OnClicked_Lambda(
                            [WeakState, WeakLayoutContext]()
                            {
                                const TSharedPtr<FPruneState> LiveState =
                                    WeakState.Pin();
                                const TSharedPtr<FPruneLayoutContext> LiveContext =
                                    WeakLayoutContext.Pin();

                                if (LiveState.IsValid() && LiveContext.IsValid())
                                {
                                    LiveState->LogCurrentCategories(
                                        LiveContext.ToSharedRef());
                                }

                                return FReply::Handled();
                            })
                    ]
                ]
            ];
    }

}

void FPruneDetailsCustomization::ExtendActorDetails(
    TSharedRef<FPruneState> State,
    IDetailLayoutBuilder& DetailBuilder,
    const TArray<TWeakObjectPtr<AActor>>& SelectedActors)
{
    if (SelectedActors.IsEmpty() || DetailBuilder.HasClassDefaultObject())
    {
        return;
    }

    // The engine only broadcasts OnExtendActorDetails for an instance Actor
    // layout with selected Actors. Keep a small defensive gate so Prune never
    // leaks into PIE/simulation or template layouts if that contract changes.
    for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
    {
        const AActor* Actor = WeakActor.Get();
        const UWorld* World = Actor ? Actor->GetWorld() : nullptr;

        if (!IsValid(Actor)
            || Actor->IsTemplate()
            || Actor->IsActorBeingDestroyed()
            || World == nullptr
            || World->WorldType != EWorldType::Editor)
        {
            return;
        }
    }

    UE_LOG(
        LogPruneDetails,
        Verbose,
        TEXT("Prune received OnExtendActorDetails for %d selected Actor(s)."),
        SelectedActors.Num());

    // Each Details layout gets its own bridge to the category builders that
    // actually exist in that layout. The Slate control owns this context, while
    // the shared Prune state keeps only weak references for cross-view updates.
    const PruneDetailsCustomizationPrivate::FPruneSelectionIdentity SelectionIdentity =
        PruneDetailsCustomizationPrivate::BuildSelectionIdentity(SelectedActors);

    if (SelectionIdentity.Key.IsEmpty())
    {
        return;
    }

    const TSharedRef<FPruneLayoutContext> LayoutContext =
        MakeShared<FPruneLayoutContext>(
            SelectionIdentity.Key,
            SelectionIdentity.Label);

    State->RegisterLayoutContext(LayoutContext);

    // IMPORTANT: SortCategories callbacks execute after all native/default/
    // plugin categories have been generated. This gives Prune the final public
    // category map, including categories added later by plugins such as Surface.
    // UE 5.8 changes its native category grouping whenever such a callback is
    // present, so Prune also restores the normal simple/advanced-only grouping
    // before reading the completed category map.
    const TWeakPtr<FPruneState> WeakState = State;
    const TWeakPtr<FPruneLayoutContext> WeakLayoutContext = LayoutContext;
    const TWeakPtr<IPropertyUtilities> WeakUtilities =
        DetailBuilder.GetPropertyUtilities();

    DetailBuilder.SortCategories(
        [WeakState, WeakLayoutContext, WeakUtilities](
            const TMap<FName, IDetailCategoryBuilder*>& Categories)
        {
            const TSharedPtr<FPruneState> LiveState = WeakState.Pin();
            const TSharedPtr<FPruneLayoutContext> LiveContext =
                WeakLayoutContext.Pin();

            if (!LiveState.IsValid() || !LiveContext.IsValid())
            {
                return;
            }

            // Registering any SortCategories callback changes UE 5.8 from
            // its normal two-block ordering (simple, then advanced-only) to a
            // combined sort. Restore that native grouping before discovery so
            // Prune's presence does not reshuffle ordinary Details categories.
            PruneDetailsCustomizationPrivate::RestoreStandardCategoryGrouping(
                Categories);

            LiveContext->UpdateFromFinalCategories(
                Categories,
                PruneDetailsCustomizationPrivate::PruneControlCategoryName);

            // Apply this selection key's current deny-list after layout generation.
            // This avoids rebuilding the layout and preserves every category's
            // established sort order. The deferred action also avoids asking the
            // Details view to refilter while GenerateDetailLayout is still in its
            // category-sort phase.
            if (const TSharedPtr<IPropertyUtilities> Utilities =
                WeakUtilities.Pin())
            {
                Utilities->EnqueueDeferredAction(
                    FSimpleDelegate::CreateLambda(
                        [WeakState, WeakLayoutContext]()
                        {
                            const TSharedPtr<FPruneState> DeferredState =
                                WeakState.Pin();
                            const TSharedPtr<FPruneLayoutContext> DeferredContext =
                                WeakLayoutContext.Pin();

                            if (DeferredState.IsValid()
                                && DeferredContext.IsValid())
                            {
                                DeferredContext->ApplyHiddenState(
                                    *DeferredState);
                            }
                        }));
            }
        });

    IDetailCategoryBuilder& PruneCategory =
        DetailBuilder.EditCategory(
            PruneDetailsCustomizationPrivate::PruneControlCategoryName,
            LOCTEXT("PruneCategoryLabel", "Prune"),
            ECategoryPriority::Important);

    PruneCategory.SetSortOrder(1);
    PruneCategory.InitiallyCollapsed(false);
    PruneCategory.RestoreExpansionState(false);

    PruneCategory.AddCustomRow(
        LOCTEXT("PruneSearchString", "Prune Categories Filter"))
        .WholeRowContent()
        [
            BuildControlWidget(State, LayoutContext)
        ];
}

TSharedRef<SWidget> FPruneDetailsCustomization::BuildControlWidget(
    const TSharedRef<FPruneState>& State,
    const TSharedRef<FPruneLayoutContext>& LayoutContext)
{
    // Capture the context strongly from the live Slate widget. When the Details
    // layout is destroyed/rebuilt, the widget releases it and Prune's shared
    // state naturally drops the corresponding weak layout reference.
    return SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(SComboButton)
            .ContentPadding(FMargin(6.0f, 2.0f))
            .ToolTipText(LOCTEXT(
                "PrunePresetComboTooltip",
                "Apply, create, or delete class-agnostic Prune presets. Exact Actor-class selections remember their state across editor restarts."))
            .OnGetMenuContent_Lambda([State, LayoutContext]()
            {
                return PruneDetailsCustomizationPrivate::BuildPresetMenu(
                    State,
                    LayoutContext);
            })
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text_Lambda([State, LayoutContext]()
                {
                    return FText::Format(
                        LOCTEXT("PresetButtonFmt", "Preset: {0}"),
                        State->GetSelectionModeLabel(
                            LayoutContext->GetSelectionKey()));
                })
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(6.0f, 0.0f, 0.0f, 0.0f)
        [
            SNew(SComboButton)
            .ContentPadding(FMargin(6.0f, 2.0f))
            .ToolTipText(LOCTEXT(
                "PruneComboTooltip",
                "Show or hide finished Details categories for the current Actor layout. Plugin-added categories are included when they use Unreal's category system."))
            .OnGetMenuContent_Lambda([State, LayoutContext]()
            {
                return PruneDetailsCustomizationPrivate::BuildCategoryMenu(
                    State,
                    LayoutContext);
            })
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text_Lambda([State, LayoutContext]()
                {
                    const int32 HiddenCount =
                        LayoutContext->GetHiddenCurrentCategoryCount(*State);

                    return HiddenCount > 0
                        ? FText::Format(
                            LOCTEXT(
                                "PruneButtonHiddenFmt",
                                "Categories ({0} Hidden)"),
                            FText::AsNumber(HiddenCount))
                        : LOCTEXT("PruneButton", "Categories");
                })
            ]
        ]

        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        .Padding(8.0f, 0.0f, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(LayoutContext->GetSelectionLabel())
            .ToolTipText_Lambda([LayoutContext]()
            {
                return LayoutContext->GetSelectionKey().StartsWith(TEXT("Mixed:"))
                    ? LOCTEXT(
                        "MixedPersistenceTooltip",
                        "Mixed-class selection filters are intentionally session-only. Named presets remain reusable.")
                    : LOCTEXT(
                        "ClassPersistenceTooltip",
                        "This Actor class remembers its Prune state in EditorPerProjectUserSettings.ini for this user and project.");
            })
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ];
}

#undef LOCTEXT_NAMESPACE
