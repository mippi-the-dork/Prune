// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailsView.h"
#include "IPropertyUtilities.h"
#include "Misc/MessageDialog.h"
#include "PruneState.h"
#include "Styling/AppStyle.h"
#include "Types/ISlateMetaData.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "PruneDetailsCustomization"

DEFINE_LOG_CATEGORY_STATIC(LogPruneDetails, Log, All);

namespace PruneDetailsCustomizationPrivate
{
    static const FName SectionViewTag(TEXT("SectionView"));
    static const FName PruneManagerTag(TEXT("Prune.Manager"));

    /**
     * UE 5.8 normally sorts simple categories and advanced-only categories
     * separately, then appends the advanced-only block. Registering any
     * SortCategories callback changes that to a single combined sort. Prune
     * needs the callback for finished-category discovery, so restore the native
     * grouping before reading the completed category map.
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

            Category->GetDefaultProperties(SimpleProperties, true, false);
            Category->GetDefaultProperties(AdvancedProperties, false, true);

            FCategoryOrderEntry OrderEntry;
            OrderEntry.Category = Category;
            OrderEntry.OriginalSortOrder = Category->GetSortOrder();
            OrderEntry.Id = Entry.Key;

            const bool bAdvancedOnly =
                SimpleProperties.IsEmpty() && !AdvancedProperties.IsEmpty();

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

    static FText BuildSelectionLabel(
        const TArray<TWeakObjectPtr<AActor>>& SelectedActors)
    {
        TSet<const UClass*> Classes;
        const UClass* FirstClass = nullptr;

        for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
        {
            const AActor* Actor = WeakActor.Get();
            const UClass* ActorClass = Actor ? Actor->GetClass() : nullptr;
            if (ActorClass != nullptr)
            {
                Classes.Add(ActorClass);
                FirstClass = FirstClass ? FirstClass : ActorClass;
            }
        }

        if (Classes.Num() == 1 && FirstClass != nullptr)
        {
            return FirstClass->GetDisplayNameText();
        }

        return FText::Format(
            LOCTEXT("MixedSelectionLabel", "Mixed Selection ({0} Classes)"),
            FText::AsNumber(Classes.Num()));
    }

    struct FPresetEditorResult
    {
        FString Name;
        TSet<FName> HiddenCategories;
    };

    static TOptional<FPresetEditorResult> ShowPresetEditor(
        const TSharedRef<FPruneLayoutContext>& LayoutContext,
        const FString& InitialName,
        const TSet<FName>& InitialHiddenCategories,
        bool bEditingExisting)
    {
        TSharedPtr<SEditableTextBox> NameTextBox;
        TSet<FName> WorkingHiddenCategories = InitialHiddenCategories;
        bool bAccepted = false;

        const TArray<FPruneCategoryGroupInfo> CategoryGroups =
            LayoutContext->GetCategoryGroups();

        TSharedRef<SVerticalBox> CategoryList = SNew(SVerticalBox);

        for (const FPruneCategoryGroupInfo& Group : CategoryGroups)
        {
            const TArray<FName> CategoryIds = Group.Ids;

            CategoryList->AddSlot()
            .AutoHeight()
            .Padding(2.0f)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda(
                    [&WorkingHiddenCategories, CategoryIds]()
                    {
                        int32 HiddenCount = 0;
                        for (const FName CategoryId : CategoryIds)
                        {
                            HiddenCount += WorkingHiddenCategories.Contains(CategoryId)
                                ? 1
                                : 0;
                        }

                        if (HiddenCount == 0)
                        {
                            return ECheckBoxState::Checked;
                        }
                        if (HiddenCount == CategoryIds.Num())
                        {
                            return ECheckBoxState::Unchecked;
                        }
                        return ECheckBoxState::Undetermined;
                    })
                .OnCheckStateChanged_Lambda(
                    [&WorkingHiddenCategories, CategoryIds](ECheckBoxState State)
                    {
                        const bool bShow = State == ECheckBoxState::Checked;
                        for (const FName CategoryId : CategoryIds)
                        {
                            if (bShow)
                            {
                                WorkingHiddenCategories.Remove(CategoryId);
                            }
                            else
                            {
                                WorkingHiddenCategories.Add(CategoryId);
                            }
                        }
                    })
                [
                    SNew(STextBlock)
                    .Text(Group.DisplayName)
                ]
            ];
        }

        TSharedRef<SWindow> Dialog =
            SNew(SWindow)
            .Title(bEditingExisting
                ? LOCTEXT("EditPresetTitle", "Edit Prune Preset")
                : LOCTEXT("NewPresetTitle", "New Prune Preset"))
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
                    .Text(FText::Format(
                        LOCTEXT(
                            "PresetEditorDescription",
                            "Checked categories are included in the preset for {0}. New category IDs discovered later default to visible."),
                        LayoutContext->GetSelectionLabel()))
                    .AutoWrapText(true)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 10.0f, 0.0f, 4.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PresetNameLabel", "Preset Name"))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(NameTextBox, SEditableTextBox)
                    .Text(FText::FromString(InitialName))
                    .MinDesiredWidth(360.0f)
                    .SelectAllTextWhenFocused(true)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 10.0f, 0.0f, 4.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PresetCategoriesLabel", "Categories"))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBox)
                    .MinDesiredWidth(360.0f)
                    .MaxDesiredHeight(500.0f)
                    [
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            CategoryList
                        ]
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                .HAlign(HAlign_Right)
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("CancelPresetEdit", "Cancel"))
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
                        .Text(LOCTEXT("SavePresetEdit", "Save"))
                        .IsEnabled_Lambda([&NameTextBox]()
                        {
                            return NameTextBox.IsValid()
                                && !NameTextBox->GetText().ToString()
                                    .TrimStartAndEnd().IsEmpty();
                        })
                        .OnClicked_Lambda([Dialog, &bAccepted]()
                        {
                            bAccepted = true;
                            Dialog->RequestDestroyWindow();
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
            return TOptional<FPresetEditorResult>();
        }

        FPresetEditorResult Result;
        Result.Name = NameTextBox->GetText().ToString().TrimStartAndEnd();
        Result.HiddenCategories = MoveTemp(WorkingHiddenCategories);
        return Result;
    }

    static TSharedRef<SWidget> BuildManagementMenu(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<FPruneLayoutContext>& WeakLayoutContext)
    {
        const TSharedPtr<FPruneState> State = WeakState.Pin();
        const TSharedPtr<FPruneLayoutContext> Context = WeakLayoutContext.Pin();

        TSharedRef<SVerticalBox> List = SNew(SVerticalBox);

        List->AddSlot()
        .AutoHeight()
        .Padding(6.0f, 4.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("NewPreset", "+ New Preset..."))
            .IsEnabled(Context.IsValid())
            .OnClicked_Lambda([WeakState, WeakLayoutContext]()
            {
                FSlateApplication::Get().DismissAllMenus();

                const TSharedPtr<FPruneState> LiveState = WeakState.Pin();
                const TSharedPtr<FPruneLayoutContext> LiveContext =
                    WeakLayoutContext.Pin();
                if (!LiveState.IsValid() || !LiveContext.IsValid())
                {
                    return FReply::Handled();
                }

                const TOptional<FPresetEditorResult> Result =
                    ShowPresetEditor(
                        LiveContext.ToSharedRef(),
                        FString(),
                        TSet<FName>(),
                        false);

                if (Result.IsSet())
                {
                    FString Error;
                    if (!LiveState->CreatePreset(
                        Result->Name,
                        Result->HiddenCategories,
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

        const TArray<FPrunePresetSummary> Presets = State.IsValid()
            ? State->GetPresets()
            : TArray<FPrunePresetSummary>();

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
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(PresetName)
                ]

                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("EditPreset", "Edit"))
                    .IsEnabled(Context.IsValid())
                    .OnClicked_Lambda(
                        [WeakState, WeakLayoutContext, PresetId, PresetName]()
                        {
                            FSlateApplication::Get().DismissAllMenus();

                            const TSharedPtr<FPruneState> LiveState =
                                WeakState.Pin();
                            const TSharedPtr<FPruneLayoutContext> LiveContext =
                                WeakLayoutContext.Pin();
                            if (!LiveState.IsValid() || !LiveContext.IsValid())
                            {
                                return FReply::Handled();
                            }

                            const TOptional<FPresetEditorResult> Result =
                                ShowPresetEditor(
                                    LiveContext.ToSharedRef(),
                                    PresetName.ToString(),
                                    LiveState->GetPresetHiddenCategories(PresetId),
                                    true);

                            if (Result.IsSet())
                            {
                                FString Error;
                                if (!LiveState->UpdatePreset(
                                    PresetId,
                                    Result->Name,
                                    Result->HiddenCategories,
                                    Error))
                                {
                                    FMessageDialog::Open(
                                        EAppMsgType::Ok,
                                        FText::FromString(Error));
                                }
                            }

                            return FReply::Handled();
                        })
                ]

                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("DeletePreset", "Delete"))
                    .OnClicked_Lambda([WeakState, PresetId, PresetName]()
                    {
                        FSlateApplication::Get().DismissAllMenus();

                        if (FMessageDialog::Open(
                            EAppMsgType::YesNo,
                            FText::Format(
                                LOCTEXT(
                                    "DeletePresetConfirm",
                                    "Delete Prune preset '{0}'?"),
                                PresetName)) == EAppReturnType::Yes)
                        {
                            if (const TSharedPtr<FPruneState> LiveState =
                                WeakState.Pin())
                            {
                                LiveState->DeletePreset(PresetId);
                            }
                        }

                        return FReply::Handled();
                    })
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
            .Text(LOCTEXT("LogCategoryIds", "Log Current Category IDs"))
            .IsEnabled(Context.IsValid())
            .OnClicked_Lambda([WeakState, WeakLayoutContext]()
            {
                if (const TSharedPtr<FPruneState> LiveState = WeakState.Pin())
                {
                    if (const TSharedPtr<FPruneLayoutContext> LiveContext =
                        WeakLayoutContext.Pin())
                    {
                        LiveState->LogCurrentCategories(LiveContext.ToSharedRef());
                    }
                }
                return FReply::Handled();
            })
        ];

        return SNew(SBox)
            .MinDesiredWidth(360.0f)
            .MaxDesiredHeight(620.0f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    List
                ]
            ];
    }

    static TSharedRef<SWidget> BuildManagementButton(
        const TSharedRef<FPruneState>& State,
        const TSharedRef<FPruneLayoutContext>& LayoutContext)
    {
        return SNew(SBox)
            .AddMetaData<FTagMetaData>(PruneManagerTag)
            .HAlign(HAlign_Right)
            [
                SNew(SComboButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .ContentPadding(FMargin(7.0f, 2.0f))
                .ToolTipText(LOCTEXT(
                    "PruneManagerTooltip",
                    "Create, edit, rename, and delete Prune presets. Presets appear as native Details section buttons."))
                .OnGetMenuContent_Lambda([State, LayoutContext]()
                {
                    return BuildManagementMenu(State, LayoutContext);
                })
                .ButtonContent()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PruneManagerButton", "Prune"))
                    .TextStyle(FAppStyle::Get(), "SmallText")
                ]
            ];
    }

    static SWrapBox* FindSectionSelectorRecursive(SWidget& Widget)
    {
        const TSharedPtr<FTagMetaData> TagMeta =
            Widget.GetMetaData<FTagMetaData>();

        if (TagMeta.IsValid() && TagMeta->Tag == SectionViewTag)
        {
            return static_cast<SWrapBox*>(&Widget);
        }

        FChildren* Children = Widget.GetChildren();
        if (Children == nullptr)
        {
            return nullptr;
        }

        for (int32 Index = 0; Index < Children->Num(); ++Index)
        {
            TSharedRef<SWidget> Child = Children->GetChildAt(Index);
            if (SWrapBox* Found = FindSectionSelectorRecursive(Child.Get()))
            {
                return Found;
            }
        }

        return nullptr;
    }

    static bool HasPruneManager(SWrapBox& SectionSelector)
    {
        FChildren* Children = SectionSelector.GetChildren();
        if (Children == nullptr)
        {
            return false;
        }

        for (int32 Index = 0; Index < Children->Num(); ++Index)
        {
            const TSharedRef<SWidget> Child = Children->GetChildAt(Index);
            const TSharedPtr<FTagMetaData> TagMeta =
                Child->GetMetaData<FTagMetaData>();

            if (TagMeta.IsValid() && TagMeta->Tag == PruneManagerTag)
            {
                return true;
            }
        }

        return false;
    }

    static void InjectManagementButton(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<FPruneLayoutContext>& WeakLayoutContext)
    {
        const TSharedPtr<FPruneState> State = WeakState.Pin();
        const TSharedPtr<FPruneLayoutContext> Context = WeakLayoutContext.Pin();
        if (!State.IsValid() || !Context.IsValid())
        {
            return;
        }

        const TSharedPtr<const IDetailsView> DetailsView =
            Context->GetDetailsView();
        if (!DetailsView.IsValid())
        {
            return;
        }

        // IDetailsView is itself an SCompoundWidget. UE 5.8 tags the native
        // section SWrapBox with FTagMetaData("SectionView"), so Prune can find
        // that exact row without replacing the Details view or touching engine
        // source. This is intentionally a small 5.8-specific Slate bridge.
        IDetailsView* MutableDetailsView =
            const_cast<IDetailsView*>(DetailsView.Get());

        SWrapBox* SectionSelector =
            FindSectionSelectorRecursive(*MutableDetailsView);

        if (SectionSelector == nullptr || HasPruneManager(*SectionSelector))
        {
            return;
        }

        SectionSelector->AddSlot()
            .FillEmptySpace(true)
            .HAlign(HAlign_Right)
            [
                BuildManagementButton(State.ToSharedRef(), Context.ToSharedRef())
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

    const TSharedPtr<const IDetailsView> DetailsView =
        DetailBuilder.GetDetailsViewSharedPtr();

    if (!DetailsView.IsValid())
    {
        return;
    }

    const TSharedRef<FPruneLayoutContext> LayoutContext =
        MakeShared<FPruneLayoutContext>(
            PruneDetailsCustomizationPrivate::BuildSelectionLabel(SelectedActors),
            DetailsView);

    State->RegisterLayoutContext(LayoutContext);

    const TWeakPtr<FPruneState> WeakState = State;
    const TWeakPtr<IPropertyUtilities> WeakUtilities =
        DetailBuilder.GetPropertyUtilities();

    // IMPORTANT: the callback itself owns LayoutContext strongly.
    // SortCategories runs later, after ExtendActorDetails has returned. In
    // 0.4.0 the callback kept only a weak context, so removing the old Prune
    // Details-tree widget also removed the last strong owner. The context was
    // therefore destroyed before this callback ever ran. Keeping it here ties
    // the context lifetime directly to the DetailLayoutBuilder that stores the
    // callback, without leaking it beyond the life of that layout.
    DetailBuilder.SortCategories(
        [WeakState, LayoutContext, WeakUtilities](
            const TMap<FName, IDetailCategoryBuilder*>& Categories)
        {
            const TSharedPtr<FPruneState> LiveState = WeakState.Pin();
            if (!LiveState.IsValid())
            {
                return;
            }

            PruneDetailsCustomizationPrivate::RestoreStandardCategoryGrouping(
                Categories);

            LayoutContext->UpdateFromFinalCategories(Categories);

            // Native sections must be registered before SDetailsView performs
            // its section-selector rebuild at the end of the object refresh.
            LiveState->SyncNativeSectionsForContext(LayoutContext);

            // The engine rebuilds SectionSelectorBox after category generation,
            // which clears any custom children. Inject Prune's management
            // control deferred, after that native rebuild has completed.
            if (const TSharedPtr<IPropertyUtilities> Utilities =
                WeakUtilities.Pin())
            {
                const TWeakPtr<FPruneLayoutContext> WeakLayoutContext =
                    LayoutContext;

                Utilities->EnqueueDeferredAction(
                    FSimpleDelegate::CreateLambda(
                        [WeakState, WeakLayoutContext]()
                        {
                            PruneDetailsCustomizationPrivate::InjectManagementButton(
                                WeakState,
                                WeakLayoutContext);
                        }));
            }
        });

    UE_LOG(
        LogPruneDetails,
        Verbose,
        TEXT("Prune attached native-section preset support to %d Actor(s)."),
        SelectedActors.Num());
}

#undef LOCTEXT_NAMESPACE
