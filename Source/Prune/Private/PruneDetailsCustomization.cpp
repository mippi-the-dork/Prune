// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "IPropertyUtilities.h"
#include "PruneState.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PruneDetailsCustomization"

DEFINE_LOG_CATEGORY_STATIC(LogPruneDetails, Log, All);

namespace PruneDetailsCustomizationPrivate
{
    static const FName PruneControlCategoryName(TEXT("PruneControl"));

    static TSharedRef<SWidget> BuildCategoryMenu(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<FPruneLayoutContext>& WeakLayoutContext)
    {
        const TSharedPtr<FPruneState> PinnedState = WeakState.Pin();
        const TSharedPtr<FPruneLayoutContext> PinnedContext =
            WeakLayoutContext.Pin();

        const TArray<FPruneCategoryInfo> Categories =
            PinnedContext.IsValid()
                ? PinnedContext->GetCategories()
                : TArray<FPruneCategoryInfo>();

        TSharedRef<SVerticalBox> List = SNew(SVerticalBox);

        if (Categories.IsEmpty())
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
            for (const FPruneCategoryInfo& Category : Categories)
            {
                const FName CategoryId = Category.Id;
                const FText CategoryLabel = Category.DisplayName;

                List->AddSlot()
                .AutoHeight()
                .Padding(6.0f, 2.0f)
                [
                    SNew(SCheckBox)
                    .ToolTipText(FText::Format(
                        LOCTEXT(
                            "CategoryTooltipFmt",
                            "Internal category ID: {0}"),
                        FText::FromString(CategoryId.ToString())))
                    .IsChecked_Lambda([WeakState, CategoryId]()
                    {
                        const TSharedPtr<FPruneState> LiveState =
                            WeakState.Pin();

                        return LiveState.IsValid()
                            && LiveState->IsCategoryVisible(CategoryId)
                                ? ECheckBoxState::Checked
                                : ECheckBoxState::Unchecked;
                    })
                    .OnCheckStateChanged_Lambda(
                        [WeakState, CategoryId](ECheckBoxState NewState)
                        {
                            if (const TSharedPtr<FPruneState> LiveState =
                                WeakState.Pin())
                            {
                                LiveState->SetCategoryVisible(
                                    CategoryId,
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
                            "Clear Prune's current hidden-category deny-list and restore categories in place."))
                        .OnClicked_Lambda([WeakState]()
                        {
                            if (const TSharedPtr<FPruneState> LiveState =
                                WeakState.Pin())
                            {
                                LiveState->ShowAllCategories();
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
                            "Write the complete category set, internal IDs, display labels, and Prune visibility to the Output Log."))
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
    const TSharedRef<FPruneLayoutContext> LayoutContext =
        MakeShared<FPruneLayoutContext>();

    State->RegisterLayoutContext(LayoutContext);

    // IMPORTANT: SortCategories callbacks execute after all native/default/
    // plugin categories have been generated. This gives Prune the final public
    // category map, including categories added later by plugins such as Surface.
    // Prune does not alter sort order in this callback.
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

            LiveContext->UpdateFromFinalCategories(
                Categories,
                PruneDetailsCustomizationPrivate::PruneControlCategoryName);

            // Apply the session deny-list after layout generation has completed.
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
            .Text(LOCTEXT(
                "PrototypeNote",
                "Prototype: session-only category deny-list"))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ];
}

#undef LOCTEXT_NAMESPACE
