// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailsView.h"
#include "Misc/MessageDialog.h"
#include "PruneState.h"
#include "Styling/AppStyle.h"
#include "Types/ISlateMetaData.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "PruneDetailsCustomization"

DEFINE_LOG_CATEGORY_STATIC(LogPruneDetails, Log, All);

namespace PruneDetailsCustomizationPrivate
{
    static const FName SectionViewTag(TEXT("SectionView"));
    static const FName PruneSectionRowWrapperTag(TEXT("Prune.SectionRowWrapper"));

    /** Restore UE's normal simple/advanced-only grouping before Prune reads it. */
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

            (bAdvancedOnly ? AdvancedOnlyCategories : SimpleCategories).Add(OrderEntry);
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

    static void ApplyStoredCategoryOrder(
        const TArray<FName>& StoredOrder,
        const TMap<FName, IDetailCategoryBuilder*>& Categories)
    {
        if (StoredOrder.IsEmpty())
        {
            return;
        }

        TSet<FName> Applied;
        int32 NewOrder = 0;

        for (const FName CategoryId : StoredOrder)
        {
            if (IDetailCategoryBuilder* const* Category = Categories.Find(CategoryId))
            {
                if (*Category != nullptr && !Applied.Contains(CategoryId))
                {
                    (*Category)->SetSortOrder(NewOrder++);
                    Applied.Add(CategoryId);
                }
            }
        }

        struct FRemainingCategory
        {
            FName Id = NAME_None;
            IDetailCategoryBuilder* Category = nullptr;
            int32 NativeOrder = 0;
        };

        TArray<FRemainingCategory> Remaining;
        for (const TPair<FName, IDetailCategoryBuilder*>& Entry : Categories)
        {
            if (Entry.Value == nullptr || Applied.Contains(Entry.Key))
            {
                continue;
            }

            FRemainingCategory& Item = Remaining.AddDefaulted_GetRef();
            Item.Id = Entry.Key;
            Item.Category = Entry.Value;
            Item.NativeOrder = Entry.Value->GetSortOrder();
        }

        Remaining.Sort(
            [](const FRemainingCategory& A, const FRemainingCategory& B)
            {
                if (A.NativeOrder != B.NativeOrder)
                {
                    return A.NativeOrder < B.NativeOrder;
                }
                return A.Id.ToString() < B.Id.ToString();
            });

        for (FRemainingCategory& Item : Remaining)
        {
            Item.Category->SetSortOrder(NewOrder++);
        }
    }

    static UClass* GetSingleActorClass(
        const TArray<TWeakObjectPtr<AActor>>& SelectedActors)
    {
        UClass* Result = nullptr;

        for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
        {
            AActor* Actor = WeakActor.Get();
            UClass* ActorClass = Actor ? Actor->GetClass() : nullptr;
            if (ActorClass == nullptr)
            {
                continue;
            }

            if (Result == nullptr)
            {
                Result = ActorClass;
            }
            else if (Result != ActorClass)
            {
                return nullptr;
            }
        }

        return Result;
    }

    static FText BuildSelectionLabel(
        const TArray<TWeakObjectPtr<AActor>>& SelectedActors)
    {
        if (UClass* SingleClass = GetSingleActorClass(SelectedActors))
        {
            return SingleClass->GetDisplayNameText();
        }

        TSet<const UClass*> Classes;
        for (const TWeakObjectPtr<AActor>& WeakActor : SelectedActors)
        {
            if (const AActor* Actor = WeakActor.Get())
            {
                if (const UClass* ActorClass = Actor->GetClass())
                {
                    Classes.Add(ActorClass);
                }
            }
        }

        return FText::Format(
            LOCTEXT("MixedSelectionLabel", "Mixed Selection ({0} Classes)"),
            FText::AsNumber(Classes.Num()));
    }

    enum class EFilterEditorAction : uint8
    {
        Cancel,
        Save,
        Delete,
        ResetNative
    };

    struct FFilterEditorResult
    {
        EFilterEditorAction Action = EFilterEditorAction::Cancel;
        FString Name;
        bool bGlobal = false;
        TSet<FName> HiddenCategories;
        TArray<FName> OrderedCategoryIds;
    };

    static TArray<FName> FlattenGroupOrder(
        const TArray<FPruneCategoryGroupInfo>& Groups)
    {
        TArray<FName> Result;
        for (const FPruneCategoryGroupInfo& Group : Groups)
        {
            for (const FName Id : Group.Ids)
            {
                Result.AddUnique(Id);
            }
        }
        return Result;
    }

    static void ApplySavedOrderToGroups(
        TArray<FPruneCategoryGroupInfo>& Groups,
        const TArray<FName>& OrderedCategoryIds)
    {
        if (OrderedCategoryIds.IsEmpty() || Groups.IsEmpty())
        {
            return;
        }

        TMap<FName, int32> RankById;
        for (int32 Index = 0; Index < OrderedCategoryIds.Num(); ++Index)
        {
            RankById.FindOrAdd(OrderedCategoryIds[Index]) = Index;
        }

        Groups.Sort(
            [&RankById](const FPruneCategoryGroupInfo& A, const FPruneCategoryGroupInfo& B)
            {
                int32 ARank = MAX_int32;
                int32 BRank = MAX_int32;

                for (const FName Id : A.Ids)
                {
                    if (const int32* Rank = RankById.Find(Id))
                    {
                        ARank = FMath::Min(ARank, *Rank);
                    }
                }
                for (const FName Id : B.Ids)
                {
                    if (const int32* Rank = RankById.Find(Id))
                    {
                        BRank = FMath::Min(BRank, *Rank);
                    }
                }

                if (ARank != BRank)
                {
                    return ARank < BRank;
                }
                if (A.NativeSortOrder != B.NativeSortOrder)
                {
                    return A.NativeSortOrder < B.NativeSortOrder;
                }
                return A.DisplayName.ToString() < B.DisplayName.ToString();
            });
    }

    static TOptional<FFilterEditorResult> ShowFilterEditor(
        const TSharedRef<FPruneLayoutContext>& LayoutContext,
        const FPruneFilterEditorData& InitialData,
        bool bCreatingNew,
        bool bEditingNative)
    {
        TSharedPtr<SEditableTextBox> NameTextBox;
        TSharedPtr<SVerticalBox> CategoryList;
        TArray<FPruneCategoryGroupInfo> WorkingGroups = LayoutContext->GetCategoryGroups();
        TSet<FName> WorkingHiddenCategories = InitialData.HiddenCategories;
        bool bWorkingGlobal = InitialData.bGlobal;
        bool bWorkingCustomOrder = !InitialData.OrderedCategoryIds.IsEmpty();
        EFilterEditorAction Action = EFilterEditorAction::Cancel;

        ApplySavedOrderToGroups(WorkingGroups, InitialData.OrderedCategoryIds);
        const TArray<FPruneCategoryGroupInfo> NativeGroups = LayoutContext->GetCategoryGroups();

        TFunction<void()> RebuildCategoryList;
        RebuildCategoryList = [&]()
        {
            if (!CategoryList.IsValid())
            {
                return;
            }

            CategoryList->ClearChildren();

            for (int32 Index = 0; Index < WorkingGroups.Num(); ++Index)
            {
                const FPruneCategoryGroupInfo Group = WorkingGroups[Index];
                const TArray<FName> CategoryIds = Group.Ids;

                CategoryList->AddSlot()
                .AutoHeight()
                .Padding(2.0f)
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(SCheckBox)
                        .IsChecked_Lambda(
                            [&WorkingHiddenCategories, CategoryIds]()
                            {
                                int32 HiddenCount = 0;
                                for (const FName CategoryId : CategoryIds)
                                {
                                    HiddenCount += WorkingHiddenCategories.Contains(CategoryId) ? 1 : 0;
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
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew(SButton)
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .ContentPadding(FMargin(5.0f, 1.0f))
                        .IsEnabled(Index > 0)
                        .ToolTipText(LOCTEXT("MoveCategoryUp", "Move category up"))
                        .Text(FText::FromString(TEXT("↑")))
                        .OnClicked_Lambda([&WorkingGroups, &bWorkingCustomOrder, &RebuildCategoryList, Index]()
                        {
                            if (Index > 0 && WorkingGroups.IsValidIndex(Index))
                            {
                                WorkingGroups.Swap(Index, Index - 1);
                                bWorkingCustomOrder = true;
                                RebuildCategoryList();
                            }
                            return FReply::Handled();
                        })
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(2.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew(SButton)
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .ContentPadding(FMargin(5.0f, 1.0f))
                        .IsEnabled(Index + 1 < WorkingGroups.Num())
                        .ToolTipText(LOCTEXT("MoveCategoryDown", "Move category down"))
                        .Text(FText::FromString(TEXT("↓")))
                        .OnClicked_Lambda([&WorkingGroups, &bWorkingCustomOrder, &RebuildCategoryList, Index]()
                        {
                            if (WorkingGroups.IsValidIndex(Index)
                                && WorkingGroups.IsValidIndex(Index + 1))
                            {
                                WorkingGroups.Swap(Index, Index + 1);
                                bWorkingCustomOrder = true;
                                RebuildCategoryList();
                            }
                            return FReply::Handled();
                        })
                    ]
                ];
            }
        };

        const bool bCanUseClassScope = LayoutContext->GetActorClass() != nullptr;
        if (!bCanUseClassScope)
        {
            bWorkingGlobal = true;
        }

        TSharedRef<SWindow> Dialog =
            SNew(SWindow)
            .Title(bCreatingNew
                ? LOCTEXT("NewFilterTitle", "New Filter")
                : LOCTEXT("EditFilterTitle", "Edit Filter"))
            .SizingRule(ESizingRule::Autosized)
            .SupportsMaximize(false)
            .SupportsMinimize(false);

        TSharedRef<SHorizontalBox> BottomButtons = SNew(SHorizontalBox);

        if (!bCreatingNew && !bEditingNative)
        {
            BottomButtons->AddSlot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("DeleteFilter", "Delete Filter"))
                .OnClicked_Lambda([Dialog, &Action]()
                {
                    Action = EFilterEditorAction::Delete;
                    Dialog->RequestDestroyWindow();
                    return FReply::Handled();
                })
            ];
        }
        else if (!bCreatingNew && bEditingNative)
        {
            BottomButtons->AddSlot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("ResetEpicDefault", "Reset to Epic Default"))
                .IsEnabled(InitialData.bHasNativeOverride)
                .OnClicked_Lambda([Dialog, &Action]()
                {
                    Action = EFilterEditorAction::ResetNative;
                    Dialog->RequestDestroyWindow();
                    return FReply::Handled();
                })
            ];
        }

        BottomButtons->AddSlot()
        .FillWidth(1.0f)
        [
            SNew(SBox)
        ];

        BottomButtons->AddSlot()
        .AutoWidth()
        [
            SNew(SButton)
            .Text(LOCTEXT("CancelFilterEdit", "Cancel"))
            .OnClicked_Lambda([Dialog]()
            {
                Dialog->RequestDestroyWindow();
                return FReply::Handled();
            })
        ];

        BottomButtons->AddSlot()
        .AutoWidth()
        .Padding(6.0f, 0.0f, 0.0f, 0.0f)
        [
            SNew(SButton)
            .Text(LOCTEXT("SaveFilterEdit", "Save"))
            .IsEnabled_Lambda([&NameTextBox, bEditingNative]()
            {
                return bEditingNative
                    || (NameTextBox.IsValid()
                        && !NameTextBox->GetText().ToString().TrimStartAndEnd().IsEmpty());
            })
            .OnClicked_Lambda([Dialog, &Action]()
            {
                Action = EFilterEditorAction::Save;
                Dialog->RequestDestroyWindow();
                return FReply::Handled();
            })
        ];

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
                            "FilterEditorDescription",
                            "Choose which categories are visible and their order when this filter is active for {0}."),
                        LayoutContext->GetSelectionLabel()))
                    .AutoWrapText(true)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 10.0f, 0.0f, 4.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("FilterNameLabel", "Filter Name"))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(NameTextBox, SEditableTextBox)
                    .Text(FText::FromString(InitialData.Name))
                    .MinDesiredWidth(430.0f)
                    .IsReadOnly(bEditingNative)
                    .SelectAllTextWhenFocused(!bEditingNative)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                [
                    SNew(SCheckBox)
                    .IsEnabled(bCanUseClassScope)
                    .IsChecked_Lambda([&bWorkingGlobal]()
                    {
                        return bWorkingGlobal
                            ? ECheckBoxState::Checked
                            : ECheckBoxState::Unchecked;
                    })
                    .OnCheckStateChanged_Lambda([&bWorkingGlobal](ECheckBoxState State)
                    {
                        bWorkingGlobal = State == ECheckBoxState::Checked;
                    })
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("GlobalFilterLabel", "Global - show this filter on any Actor"))
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(22.0f, 2.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text_Lambda([LayoutContext, InitialData, &bWorkingGlobal]()
                    {
                        if (bWorkingGlobal)
                        {
                            return LOCTEXT("GlobalScopeDescription", "Scope: all Actor classes");
                        }

                        if (!InitialData.ScopeClassName.IsNone()
                            && InitialData.ScopeClassName != LayoutContext->GetActorClassName())
                        {
                            return FText::Format(
                                LOCTEXT("InheritedClassScopeDescription", "Scope: {0} and derived classes"),
                                FText::FromName(InitialData.ScopeClassName));
                        }

                        return FText::Format(
                            LOCTEXT("ClassScopeDescription", "Scope: {0} and derived classes"),
                            LayoutContext->GetSelectionLabel());
                    })
                    .TextStyle(FAppStyle::Get(), "SmallText")
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 10.0f, 0.0f, 4.0f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("FilterCategoriesLabel", "Categories"))
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .Text(LOCTEXT("ResetCategoryOrder", "Reset Order"))
                        .ToolTipText(LOCTEXT("ResetCategoryOrderTooltip", "Restore Unreal's native category order for the current Actor layout."))
                        .OnClicked_Lambda([&WorkingGroups, &bWorkingCustomOrder, NativeGroups, &RebuildCategoryList]()
                        {
                            WorkingGroups = NativeGroups;
                            bWorkingCustomOrder = false;
                            RebuildCategoryList();
                            return FReply::Handled();
                        })
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBox)
                    .MinDesiredWidth(430.0f)
                    .MaxDesiredHeight(520.0f)
                    [
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SAssignNew(CategoryList, SVerticalBox)
                        ]
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                [
                    BottomButtons
                ]
            ]);

        RebuildCategoryList();

        FSlateApplication::Get().AddModalWindow(
            Dialog,
            FSlateApplication::Get().GetActiveTopLevelWindow(),
            false);

        if (Action == EFilterEditorAction::Cancel)
        {
            return TOptional<FFilterEditorResult>();
        }

        FFilterEditorResult Result;
        Result.Action = Action;
        Result.Name = NameTextBox.IsValid()
            ? NameTextBox->GetText().ToString().TrimStartAndEnd()
            : InitialData.Name;
        Result.bGlobal = bWorkingGlobal;
        Result.HiddenCategories = MoveTemp(WorkingHiddenCategories);
        Result.OrderedCategoryIds = bWorkingCustomOrder
            ? FlattenGroupOrder(WorkingGroups)
            : TArray<FName>();
        return Result;
    }

    static void HandleEditorResult(
        const TSharedRef<FPruneState>& State,
        const TSharedRef<FPruneLayoutContext>& Context,
        const FPruneEditableFilter* ExistingFilter,
        const FFilterEditorResult& Result)
    {
        if (Result.Action == EFilterEditorAction::Delete
            && ExistingFilter != nullptr
            && ExistingFilter->Kind == EPruneFilterKind::Custom)
        {
            const FText Name = ExistingFilter->DisplayName;
            if (FMessageDialog::Open(
                EAppMsgType::YesNo,
                FText::Format(
                    LOCTEXT("DeleteFilterConfirm", "Delete Prune filter '{0}'?"),
                    Name)) == EAppReturnType::Yes)
            {
                State->DeletePreset(ExistingFilter->CustomPresetId);
            }
            return;
        }

        if (Result.Action == EFilterEditorAction::ResetNative
            && ExistingFilter != nullptr
            && ExistingFilter->Kind == EPruneFilterKind::Native)
        {
            State->ResetNativeOverride(*ExistingFilter);
            return;
        }

        if (Result.Action != EFilterEditorAction::Save)
        {
            return;
        }

        FString Error;
        bool bSuccess = false;

        if (ExistingFilter == nullptr)
        {
            bSuccess = State->CreatePreset(
                Context,
                Result.Name,
                Result.bGlobal,
                Result.HiddenCategories,
                Result.OrderedCategoryIds,
                Error);
        }
        else if (ExistingFilter->Kind == EPruneFilterKind::Custom)
        {
            bSuccess = State->UpdatePreset(
                Context,
                ExistingFilter->CustomPresetId,
                Result.Name,
                Result.bGlobal,
                Result.HiddenCategories,
                Result.OrderedCategoryIds,
                Error);
        }
        else if (ExistingFilter->Kind == EPruneFilterKind::Native)
        {
            bSuccess = State->SaveNativeOverride(
                Context,
                *ExistingFilter,
                Result.bGlobal,
                Result.HiddenCategories,
                Result.OrderedCategoryIds,
                Error);
        }

        if (!bSuccess && !Error.IsEmpty())
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Error));
        }
    }

    static void OpenNewFilterEditor(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<const IDetailsView>& WeakDetailsView)
    {
        const TSharedPtr<FPruneState> State = WeakState.Pin();
        const TSharedPtr<const IDetailsView> DetailsView = WeakDetailsView.Pin();
        if (!State.IsValid() || !DetailsView.IsValid())
        {
            return;
        }

        const TSharedPtr<FPruneLayoutContext> Context =
            State->FindLatestLayoutContextForDetailsView(DetailsView);
        if (!Context.IsValid())
        {
            return;
        }

        FPruneFilterEditorData InitialData;
        InitialData.Name = FString();
        InitialData.bGlobal = Context->GetActorClass() == nullptr;
        InitialData.ScopeClassName = InitialData.bGlobal
            ? NAME_None
            : Context->GetActorClassName();
        const TOptional<FFilterEditorResult> Result = ShowFilterEditor(
            Context.ToSharedRef(),
            InitialData,
            true,
            false);

        if (Result.IsSet())
        {
            HandleEditorResult(
                State.ToSharedRef(),
                Context.ToSharedRef(),
                nullptr,
                Result.GetValue());
        }
    }

    static void OpenActiveFilterEditor(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<const IDetailsView>& WeakDetailsView)
    {
        const TSharedPtr<FPruneState> State = WeakState.Pin();
        const TSharedPtr<const IDetailsView> DetailsView = WeakDetailsView.Pin();
        if (!State.IsValid() || !DetailsView.IsValid())
        {
            return;
        }

        const TSharedPtr<FPruneLayoutContext> Context =
            State->FindLatestLayoutContextForDetailsView(DetailsView);
        if (!Context.IsValid())
        {
            return;
        }

        FPruneEditableFilter Filter;
        FPruneFilterEditorData InitialData;
        if (!State->ResolveSingleActiveFilter(Context.ToSharedRef(), Filter)
            || !State->BuildEditorData(Context.ToSharedRef(), Filter, InitialData))
        {
            return;
        }

        const TOptional<FFilterEditorResult> Result = ShowFilterEditor(
            Context.ToSharedRef(),
            InitialData,
            false,
            Filter.Kind == EPruneFilterKind::Native);

        if (Result.IsSet())
        {
            HandleEditorResult(
                State.ToSharedRef(),
                Context.ToSharedRef(),
                &Filter,
                Result.GetValue());
        }
    }

    static bool WidgetHasTag(const SWidget& Widget, FName Tag)
    {
        const TSharedPtr<FTagMetaData> TagMeta = Widget.GetMetaData<FTagMetaData>();
        return TagMeta.IsValid() && TagMeta->Tag == Tag;
    }

    static bool HasWidgetTagRecursive(SWidget& Widget, FName Tag)
    {
        if (WidgetHasTag(Widget, Tag))
        {
            return true;
        }

        FChildren* Children = Widget.GetChildren();
        if (Children == nullptr)
        {
            return false;
        }

        for (int32 Index = 0; Index < Children->Num(); ++Index)
        {
            if (HasWidgetTagRecursive(Children->GetChildAt(Index).Get(), Tag))
            {
                return true;
            }
        }

        return false;
    }

    static TSharedRef<SWidget> BuildPlusButton(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<const IDetailsView>& WeakDetailsView)
    {
        return SNew(SCheckBox)
            .Style(FAppStyle::Get(), "DetailsView.SectionButton")
            .IsChecked_Lambda([]()
            {
                return ECheckBoxState::Unchecked;
            })
            .ToolTipText(LOCTEXT("NewFilterTooltip", "Create a new Prune filter"))
            .OnCheckStateChanged_Lambda([WeakState, WeakDetailsView](ECheckBoxState)
            {
                OpenNewFilterEditor(WeakState, WeakDetailsView);
            })
            [
                SNew(STextBlock)
                .TextStyle(FAppStyle::Get(), "SmallText")
                .Text(FText::FromString(TEXT("+")))
            ];
    }

    static TSharedRef<SWidget> BuildFilterGearIcon()
    {
        return SNew(SBox)
            .WidthOverride(18.0f)
            .HeightOverride(18.0f)
            [
                SNew(SOverlay)

                + SOverlay::Slot()
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Top)
                [
                    SNew(SImage)
                    .Image(FAppStyle::Get().GetBrush("Icons.Filter"))
                ]

                + SOverlay::Slot()
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Bottom)
                [
                    SNew(SBox)
                    .WidthOverride(9.0f)
                    .HeightOverride(9.0f)
                    [
                        SNew(SImage)
                        .Image(FAppStyle::Get().GetBrush("Icons.Settings"))
                    ]
                ]
            ];
    }

    static TSharedRef<SWidget> BuildEditButton(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<const IDetailsView>& WeakDetailsView)
    {
        return SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ContentPadding(FMargin(4.0f, 2.0f))
            .IsEnabled_Lambda([WeakState, WeakDetailsView]()
            {
                const TSharedPtr<FPruneState> State = WeakState.Pin();
                const TSharedPtr<const IDetailsView> DetailsView = WeakDetailsView.Pin();
                if (!State.IsValid() || !DetailsView.IsValid())
                {
                    return false;
                }

                const TSharedPtr<FPruneLayoutContext> Context =
                    State->FindLatestLayoutContextForDetailsView(DetailsView);
                return Context.IsValid()
                    && State->CanEditActiveFilter(Context.ToSharedRef());
            })
            .ToolTipText_Lambda([WeakState, WeakDetailsView]()
            {
                const TSharedPtr<FPruneState> State = WeakState.Pin();
                const TSharedPtr<const IDetailsView> DetailsView = WeakDetailsView.Pin();
                if (State.IsValid() && DetailsView.IsValid())
                {
                    const TSharedPtr<FPruneLayoutContext> Context =
                        State->FindLatestLayoutContextForDetailsView(DetailsView);
                    FPruneEditableFilter Filter;
                    if (Context.IsValid()
                        && State->ResolveSingleActiveFilter(Context.ToSharedRef(), Filter))
                    {
                        return FText::Format(
                            LOCTEXT("EditFilterTooltipNamed", "Edit filter: {0}"),
                            Filter.DisplayName);
                    }
                }
                return LOCTEXT("EditFilterTooltipDisabled", "Select one editable filter to edit it. All and Ctrl-multiselections cannot be edited.");
            })
            .OnClicked_Lambda([WeakState, WeakDetailsView]()
            {
                OpenActiveFilterEditor(WeakState, WeakDetailsView);
                return FReply::Handled();
            })
            [
                BuildFilterGearIcon()
            ];
    }

    static bool InjectPruneControls(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<FPruneLayoutContext>& WeakLayoutContext)
    {
        const TSharedPtr<FPruneState> State = WeakState.Pin();
        const TSharedPtr<FPruneLayoutContext> Context = WeakLayoutContext.Pin();
        if (!State.IsValid() || !Context.IsValid())
        {
            return false;
        }

        const TSharedPtr<const IDetailsView> ConstDetailsView = Context->GetDetailsView();
        if (!ConstDetailsView.IsValid())
        {
            return false;
        }

        IDetailsView* DetailsView = const_cast<IDetailsView*>(ConstDetailsView.Get());
        const TSharedPtr<SWidget> FilterAreaWidget = DetailsView->GetFilterAreaWidget();
        if (!FilterAreaWidget.IsValid())
        {
            return false;
        }

        // If the wrapper already exists, recover SectionView so the new layout
        // context can still read the native checked state after a Details refresh.
        if (HasWidgetTagRecursive(FilterAreaWidget.ToSharedRef().Get(), PruneSectionRowWrapperTag))
        {
            FChildren* FilterChildren = FilterAreaWidget->GetChildren();
            if (FilterChildren != nullptr)
            {
                TFunction<TSharedPtr<SWidget>(const TSharedRef<SWidget>&)> FindSectionView;
                FindSectionView = [&FindSectionView](const TSharedRef<SWidget>& Widget) -> TSharedPtr<SWidget>
                {
                    if (WidgetHasTag(Widget.Get(), SectionViewTag))
                    {
                        return Widget;
                    }

                    FChildren* Children = Widget->GetChildren();
                    if (Children != nullptr)
                    {
                        for (int32 Index = 0; Index < Children->Num(); ++Index)
                        {
                            const TSharedRef<SWidget> Child = Children->GetChildAt(Index);
                            if (TSharedPtr<SWidget> Found = FindSectionView(Child))
                            {
                                return Found;
                            }
                        }
                    }
                    return nullptr;
                };

                if (TSharedPtr<SWidget> ExistingSectionView = FindSectionView(FilterAreaWidget.ToSharedRef()))
                {
                    Context->SetSectionSelectorWidget(ExistingSectionView);
                }
            }
            return true;
        }

        if (FilterAreaWidget->GetType() != FName(TEXT("SVerticalBox")))
        {
            return false;
        }

        SVerticalBox* FilterAreaVBox = static_cast<SVerticalBox*>(FilterAreaWidget.Get());
        FChildren* FilterChildren = FilterAreaVBox->GetChildren();
        if (FilterChildren == nullptr)
        {
            return false;
        }

        TSharedPtr<SWidget> SectionSelectorWidget;
        for (int32 Index = 0; Index < FilterChildren->Num(); ++Index)
        {
            TSharedRef<SWidget> Child = FilterChildren->GetChildAt(Index);
            if (WidgetHasTag(Child.Get(), SectionViewTag)
                && Child->GetType() == FName(TEXT("SWrapBox")))
            {
                SectionSelectorWidget = Child;
                break;
            }
        }

        if (!SectionSelectorWidget.IsValid())
        {
            return false;
        }

        Context->SetSectionSelectorWidget(SectionSelectorWidget);

        const TSharedRef<SWidget> SectionSelectorRef = SectionSelectorWidget.ToSharedRef();
        const int32 RemovedIndex = FilterAreaVBox->RemoveSlot(SectionSelectorRef);
        if (RemovedIndex == INDEX_NONE)
        {
            return false;
        }

        const TWeakPtr<const IDetailsView> WeakDetailsView = ConstDetailsView;

        TSharedRef<SHorizontalBox> PruneSectionRow =
            SNew(SHorizontalBox)
            .AddMetaData<FTagMetaData>(PruneSectionRowWrapperTag)

            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SectionSelectorRef
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(4.0f, 0.0f, 0.0f, 0.0f)
            [
                BuildPlusButton(WeakState, WeakDetailsView)
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(8.0f, 0.0f, 0.0f, 0.0f)
            [
                BuildEditButton(WeakState, WeakDetailsView)
            ];

        FilterAreaVBox->InsertSlot(RemovedIndex)
            .AutoHeight()
            .Padding(8.0f, 2.0f, 8.0f, 7.0f)
            [
                PruneSectionRow
            ];

        FilterAreaVBox->Invalidate(EInvalidateWidgetReason::Layout);

        UE_LOG(
            LogPruneDetails,
            Log,
            TEXT("Prune added + and edit controls beside the native Details SectionView."));
        return true;
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

    UClass* ActorClass =
        PruneDetailsCustomizationPrivate::GetSingleActorClass(SelectedActors);

    const TSharedRef<FPruneLayoutContext> LayoutContext =
        MakeShared<FPruneLayoutContext>(
            PruneDetailsCustomizationPrivate::BuildSelectionLabel(SelectedActors),
            DetailsView,
            ActorClass);

    State->RegisterLayoutContext(LayoutContext);
    const TWeakPtr<FPruneState> WeakState = State;

    DetailBuilder.SortCategories(
        [WeakState, LayoutContext](
            const TMap<FName, IDetailCategoryBuilder*>& Categories)
        {
            const TSharedPtr<FPruneState> LiveState = WeakState.Pin();
            if (!LiveState.IsValid())
            {
                return;
            }

            PruneDetailsCustomizationPrivate::RestoreStandardCategoryGrouping(Categories);

            // Capture the finished native order before applying any active
            // filter-specific ordering.
            LayoutContext->UpdateFromFinalCategories(Categories);

            // Register Prune filters and apply persisted Epic-section overrides
            // before SDetailsView rebuilds the section selector.
            LiveState->SyncNativeSectionsForContext(LayoutContext);

            // Recover/install the native SectionView pointer before asking the
            // state which filter is currently selected. On subsequent refreshes
            // the wrapper already exists, so this simply reconnects the new
            // layout context to the existing selector widget.
            PruneDetailsCustomizationPrivate::InjectPruneControls(
                WeakState,
                LayoutContext);

            const TArray<FName> ActiveOrder =
                LiveState->GetActiveCategoryOrder(LayoutContext);
            PruneDetailsCustomizationPrivate::ApplyStoredCategoryOrder(
                ActiveOrder,
                Categories);
        });

    UE_LOG(
        LogPruneDetails,
        Verbose,
        TEXT("Prune attached filter editing to %d Actor(s)."),
        SelectedActors.Num());
}

#undef LOCTEXT_NAMESPACE
