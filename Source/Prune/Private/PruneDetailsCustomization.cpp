// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Engine/World.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "ISettingsModule.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailsView.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PruneState.h"
#include "PruneSettings.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Types/ISlateMetaData.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
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
        Duplicate,
        Delete,
        ResetNative
    };

    struct FFilterEditorResult
    {
        EFilterEditorAction Action = EFilterEditorAction::Cancel;
        FString Name;
        FString Description;
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

    static const FSlateBrush* GetGlobalScopeIconBrush()
    {
        static TSharedPtr<FSlateVectorImageBrush> GlobalScopeIcon;

        if (!GlobalScopeIcon.IsValid())
        {
            const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Prune"));
            if (Plugin.IsValid())
            {
                const FString IconPath = FPaths::Combine(
                    Plugin->GetBaseDir(),
                    TEXT("Resources"),
                    TEXT("globe.svg"));

                if (IFileManager::Get().FileExists(*IconPath))
                {
                    GlobalScopeIcon = MakeShared<FSlateVectorImageBrush>(
                        IconPath,
                        FVector2D(16.0f, 16.0f),
                        FLinearColor::White);
                }
            }
        }

        return GlobalScopeIcon.IsValid()
            ? GlobalScopeIcon.Get()
            : FAppStyle::Get().GetBrush("Icons.Filter");
    }

    static const FSlateBrush* GetFilterEditIconBrush()
    {
        static TSharedPtr<FSlateVectorImageBrush> FilterEditIcon;

        if (!FilterEditIcon.IsValid())
        {
            const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Prune"));
            if (Plugin.IsValid())
            {
                const FString IconPath = FPaths::Combine(
                    Plugin->GetBaseDir(),
                    TEXT("Resources"),
                    TEXT("filter-edit.svg"));

                if (IFileManager::Get().FileExists(*IconPath))
                {
                    FilterEditIcon = MakeShared<FSlateVectorImageBrush>(
                        IconPath,
                        FVector2D(16.0f, 16.0f),
                        FLinearColor::White);
                }
            }
        }

        return FilterEditIcon.IsValid()
            ? FilterEditIcon.Get()
            : FAppStyle::Get().GetBrush("DetailsView.ViewOptions");
    }

    static const FSlateBrush* GetFilterManagerIconBrush()
    {
        static TSharedPtr<FSlateVectorImageBrush> FilterManagerIcon;

        if (!FilterManagerIcon.IsValid())
        {
            const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Prune"));
            if (Plugin.IsValid())
            {
                const FString IconPath = FPaths::Combine(
                    Plugin->GetBaseDir(),
                    TEXT("Resources"),
                    TEXT("filter-manager.svg"));

                if (IFileManager::Get().FileExists(*IconPath))
                {
                    FilterManagerIcon = MakeShared<FSlateVectorImageBrush>(
                        IconPath,
                        FVector2D(16.0f, 16.0f),
                        FLinearColor::White);
                }
            }
        }

        return FilterManagerIcon.IsValid()
            ? FilterManagerIcon.Get()
            : FAppStyle::Get().GetBrush("Icons.Settings");
    }

    static TOptional<FFilterEditorResult> ShowFilterEditor(
        const TSharedRef<FPruneLayoutContext>& LayoutContext,
        const FPruneFilterEditorData& InitialData,
        bool bCreatingNew,
        bool bEditingNative)
    {
        TSharedPtr<SEditableTextBox> NameTextBox;
        TSharedPtr<SEditableTextBox> DescriptionTextBox;
        TSharedPtr<SDragAndDropVerticalBox> CategoryList;
        TArray<FPruneCategoryGroupInfo> WorkingGroups = LayoutContext->GetCategoryGroups();
        TSet<FName> WorkingHiddenCategories = InitialData.HiddenCategories;
        FString CategorySearchText;
        TOptional<int32> DraggedGroupIndex;
        TOptional<int32> DropTargetGroupIndex;
        TOptional<SDragAndDropVerticalBox::EItemDropZone> DropTargetZone;
        const UPruneSettings* Settings = UPruneSettings::Get();
        bool bWorkingGlobal = InitialData.bGlobal;
        bool bWorkingCustomOrder = !InitialData.OrderedCategoryIds.IsEmpty();
        EFilterEditorAction Action = EFilterEditorAction::Cancel;

        ApplySavedOrderToGroups(WorkingGroups, InitialData.OrderedCategoryIds);
        const TArray<FPruneCategoryGroupInfo> NativeGroups = LayoutContext->GetCategoryGroups();

        static const FSlateColorBrush DragRowBackgroundBrush{ FLinearColor::White };

        TFunction<void()> RebuildCategoryList;
        RebuildCategoryList = [&]()
        {
            if (!CategoryList.IsValid())
            {
                return;
            }

            CategoryList->ClearChildren();

            int32 MatchingGroupCount = 0;
            for (int32 Index = 0; Index < WorkingGroups.Num(); ++Index)
            {
                const FPruneCategoryGroupInfo Group = WorkingGroups[Index];
                const FString Search = CategorySearchText.TrimStartAndEnd();
                bool bMatchesSearch = Search.IsEmpty()
                    || Group.DisplayName.ToString().Contains(Search, ESearchCase::IgnoreCase);

                if (!bMatchesSearch)
                {
                    for (const FName CategoryId : Group.Ids)
                    {
                        if (CategoryId.ToString().Contains(Search, ESearchCase::IgnoreCase))
                        {
                            bMatchesSearch = true;
                            break;
                        }
                    }
                }

                if (!bMatchesSearch)
                {
                    continue;
                }

                ++MatchingGroupCount;

                const TArray<FName> CategoryIds = Group.Ids;
                const int32 WorkingGroupIndex = Index;

                CategoryList->AddSlot()
                .AutoHeight()
                .Padding(2.0f, 1.0f)
                [
                    SNew(SOverlay)

                    + SOverlay::Slot()
                    [
                        SNew(SBorder)
                        .BorderImage(&DragRowBackgroundBrush)
                        .BorderBackgroundColor_Lambda(
                            [Settings, &DraggedGroupIndex, WorkingGroupIndex]()
                            {
                                return DraggedGroupIndex.IsSet()
                                    && DraggedGroupIndex.GetValue() == WorkingGroupIndex
                                    ? FSlateColor(Settings->ReorderDragColor)
                                    : FSlateColor(FLinearColor::Transparent);
                            })
                        .Padding(FMargin(4.0f, 2.0f))
                        [
                            SNew(SHorizontalBox)

                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(2.0f, 0.0f, 7.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(TEXT("≡")))
                                .TextStyle(FAppStyle::Get(), "SmallText")
                                .ColorAndOpacity_Lambda(
                                    [Settings, &DraggedGroupIndex, WorkingGroupIndex]()
                                    {
                                        return DraggedGroupIndex.IsSet()
                                            && DraggedGroupIndex.GetValue() == WorkingGroupIndex
                                            ? FSlateColor(Settings->ReorderDraggedContentColor)
                                            : FSlateColor::UseForeground();
                                    })
                                .ToolTipText_Lambda([&CategorySearchText]()
                                {
                                    return CategorySearchText.TrimStartAndEnd().IsEmpty()
                                        ? LOCTEXT("DragCategoryTooltip", "Drag to reorder this category.")
                                        : LOCTEXT("DragCategorySearchTooltip", "Clear the category search before reordering.");
                                })
                            ]

                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(SCheckBox)
                                .ForegroundColor_Lambda(
                                    [Settings, &DraggedGroupIndex, WorkingGroupIndex]()
                                    {
                                        return DraggedGroupIndex.IsSet()
                                            && DraggedGroupIndex.GetValue() == WorkingGroupIndex
                                            ? FSlateColor(Settings->ReorderDraggedContentColor)
                                            : FSlateColor::UseForeground();
                                    })
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
                                    SNew(SBox)
                                    .Padding(FMargin(6.0f, 0.0f, 0.0f, 0.0f))
                                    [
                                        SNew(STextBlock)
                                        .Text(Group.DisplayName)
                                        .ColorAndOpacity_Lambda(
                                            [Settings, &DraggedGroupIndex, WorkingGroupIndex]()
                                            {
                                                return DraggedGroupIndex.IsSet()
                                                    && DraggedGroupIndex.GetValue() == WorkingGroupIndex
                                                    ? FSlateColor(Settings->ReorderDraggedContentColor)
                                                    : FSlateColor::UseForeground();
                                            })
                                    ]
                                ]
                            ]
                        ]
                    ]

                    + SOverlay::Slot()
                    .VAlign(VAlign_Top)
                    [
                        SNew(SBox)
                        .HeightOverride(FMath::Clamp(Settings->ReorderDropLineThickness, 1.0f, 8.0f))
                        .Visibility(EVisibility::HitTestInvisible)
                        [
                            SNew(SBorder)
                            .BorderImage(&DragRowBackgroundBrush)
                            .BorderBackgroundColor_Lambda(
                                [Settings, &DropTargetGroupIndex, &DropTargetZone, WorkingGroupIndex]()
                                {
                                    return DropTargetGroupIndex.IsSet()
                                        && DropTargetGroupIndex.GetValue() == WorkingGroupIndex
                                        && DropTargetZone.IsSet()
                                        && DropTargetZone.GetValue() == SDragAndDropVerticalBox::EItemDropZone::AboveItem
                                        ? FSlateColor(Settings->ReorderDropLineColor)
                                        : FSlateColor(FLinearColor::Transparent);
                                })
                            .Padding(0.0f)
                        ]
                    ]

                    + SOverlay::Slot()
                    .VAlign(VAlign_Bottom)
                    [
                        SNew(SBox)
                        .HeightOverride(FMath::Clamp(Settings->ReorderDropLineThickness, 1.0f, 8.0f))
                        .Visibility(EVisibility::HitTestInvisible)
                        [
                            SNew(SBorder)
                            .BorderImage(&DragRowBackgroundBrush)
                            .BorderBackgroundColor_Lambda(
                                [Settings, &DropTargetGroupIndex, &DropTargetZone, WorkingGroupIndex]()
                                {
                                    return DropTargetGroupIndex.IsSet()
                                        && DropTargetGroupIndex.GetValue() == WorkingGroupIndex
                                        && DropTargetZone.IsSet()
                                        && DropTargetZone.GetValue() == SDragAndDropVerticalBox::EItemDropZone::BelowItem
                                        ? FSlateColor(Settings->ReorderDropLineColor)
                                        : FSlateColor(FLinearColor::Transparent);
                                })
                            .Padding(0.0f)
                        ]
                    ]
                ];
            }

            if (MatchingGroupCount == 0)
            {
                CategoryList->AddSlot()
                .AutoHeight()
                .Padding(8.0f, 12.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("NoCategorySearchMatches", "No categories match this search."))
                    .Justification(ETextJustify::Center)
                    .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
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
                : FText::Format(
                    LOCTEXT("EditFilterTitleNamed", "Edit Filter - {0}"),
                    FText::FromString(InitialData.Name)))
            .ClientSize(FVector2D(
                FMath::Clamp(Settings->FilterEditorDefaultWidth, 420.0f, 1600.0f),
                FMath::Clamp(Settings->FilterEditorDefaultHeight, 420.0f, 1600.0f)))
            .SizingRule(ESizingRule::UserSized)
            .SupportsMaximize(false)
            .SupportsMinimize(false);

        TSharedRef<SHorizontalBox> BottomButtons = SNew(SHorizontalBox);

        BottomButtons->AddSlot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SCheckBox)
            .Style(FAppStyle::Get(), "DetailsView.SectionButton")
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
            .ToolTipText_Lambda([LayoutContext, InitialData, &bWorkingGlobal]()
            {
                if (bWorkingGlobal)
                {
                    return LOCTEXT(
                        "GlobalScopeTooltipActive",
                        "Global filter. This filter is available on all Actor classes. Click to scope it to the current class instead.");
                }

                if (!InitialData.ScopeClassName.IsNone()
                    && InitialData.ScopeClassName != LayoutContext->GetActorClassName())
                {
                    return FText::Format(
                        LOCTEXT(
                            "InheritedScopeTooltip",
                            "Class filter. This filter is scoped to {0} and derived classes. Click to make it Global."),
                        FText::FromName(InitialData.ScopeClassName));
                }

                return FText::Format(
                    LOCTEXT(
                        "ClassScopeTooltip",
                        "Class filter. This filter is scoped to {0} and derived classes. Click to make it Global."),
                    LayoutContext->GetSelectionLabel());
            })
            [
                SNew(SBox)
                .WidthOverride(16.0f)
                .HeightOverride(16.0f)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(SImage)
                    .ColorAndOpacity(FSlateColor::UseForeground())
                    .Image(GetGlobalScopeIconBrush())
                ]
            ]
        ];

        if (!bCreatingNew && !bEditingNative)
        {
            BottomButtons->AddSlot()
            .AutoWidth()
            .Padding(0.0f, 0.0f, 6.0f, 0.0f)
            [
                SNew(SButton)
                .Text(LOCTEXT("DuplicateFilter", "Duplicate..."))
                .ToolTipText(LOCTEXT(
                    "DuplicateFilterTooltip",
                    "Open a new filter prefilled with the current visibility, order, and scope."))
                .OnClicked_Lambda([Dialog, &Action]()
                {
                    Action = EFilterEditorAction::Duplicate;
                    Dialog->RequestDestroyWindow();
                    return FReply::Handled();
                })
            ];

            BottomButtons->AddSlot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("DeleteFilter", "Delete Filter"))
                .ToolTipText(LOCTEXT("DeleteFilterTooltip", "Delete this Prune filter."))
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
            .Padding(0.0f, 0.0f, 6.0f, 0.0f)
            [
                SNew(SButton)
                .Text(LOCTEXT("DuplicateEpicFilter", "Duplicate..."))
                .ToolTipText(LOCTEXT(
                    "DuplicateEpicFilterTooltip",
                    "Create a new Prune filter from this Epic filter without changing the Epic filter itself."))
                .OnClicked_Lambda([Dialog, &Action]()
                {
                    Action = EFilterEditorAction::Duplicate;
                    Dialog->RequestDestroyWindow();
                    return FReply::Handled();
                })
            ];

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
                            "Choose which categories are visible and drag them into the order you want when this filter is active for {0}."),
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
                .Padding(0.0f, 10.0f, 0.0f, 4.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("FilterDescriptionLabel", "Description"))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(DescriptionTextBox, SEditableTextBox)
                    .Text(FText::FromString(InitialData.Description))
                    .HintText(LOCTEXT("FilterDescriptionHint", "Optional description shown in the filter tooltip"))
                    .MinDesiredWidth(430.0f)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 12.0f, 0.0f, 4.0f)
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("FilterCategoriesLabel", "Categories"))
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([&WorkingGroups, &WorkingHiddenCategories]()
                        {
                            int32 VisibleGroups = 0;
                            for (const FPruneCategoryGroupInfo& Group : WorkingGroups)
                            {
                                bool bAnyVisible = false;
                                for (const FName CategoryId : Group.Ids)
                                {
                                    if (!WorkingHiddenCategories.Contains(CategoryId))
                                    {
                                        bAnyVisible = true;
                                        break;
                                    }
                                }

                                VisibleGroups += bAnyVisible ? 1 : 0;
                            }

                            return FText::Format(
                                LOCTEXT("VisibleCategoryCount", "{0} of {1} shown"),
                                FText::AsNumber(VisibleGroups),
                                FText::AsNumber(WorkingGroups.Num()));
                        })
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        SNew(SBox)
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .Text(LOCTEXT("ShowAllCategories", "Show All"))
                        .ToolTipText(LOCTEXT("ShowAllCategoriesTooltip", "Show every category in this filter."))
                        .OnClicked_Lambda([&WorkingGroups, &WorkingHiddenCategories]()
                        {
                            for (const FPruneCategoryGroupInfo& Group : WorkingGroups)
                            {
                                for (const FName CategoryId : Group.Ids)
                                {
                                    WorkingHiddenCategories.Remove(CategoryId);
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
                        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                        .Text(LOCTEXT("HideAllCategories", "Hide All"))
                        .ToolTipText(LOCTEXT("HideAllCategoriesTooltip", "Hide every category in this filter."))
                        .OnClicked_Lambda([&WorkingGroups, &WorkingHiddenCategories]()
                        {
                            for (const FPruneCategoryGroupInfo& Group : WorkingGroups)
                            {
                                for (const FName CategoryId : Group.Ids)
                                {
                                    WorkingHiddenCategories.Add(CategoryId);
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
                .Padding(0.0f, 0.0f, 0.0f, 6.0f)
                [
                    SNew(SSearchBox)
                    .HintText(LOCTEXT("CategorySearchHint", "Search categories"))
                    .OnTextChanged_Lambda([&CategorySearchText, &DraggedGroupIndex, &DropTargetGroupIndex, &DropTargetZone, &RebuildCategoryList](const FText& NewText)
                    {
                        CategorySearchText = NewText.ToString();
                        DraggedGroupIndex.Reset();
                        DropTargetGroupIndex.Reset();
                        DropTargetZone.Reset();
                        RebuildCategoryList();
                    })
                ]

                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .Padding(4.0f)
                    [
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SAssignNew(CategoryList, SDragAndDropVerticalBox)
                            .OnDragDetected_Lambda(
                                [&CategorySearchText, &DraggedGroupIndex, &DropTargetGroupIndex, &DropTargetZone, &CategoryList](
                                   const FGeometry&,
                                   const FPointerEvent&,
                                   int32 SourceIndex,
                                   SVerticalBox::FSlot* SourceSlot)
                                {
                                    if (!CategorySearchText.TrimStartAndEnd().IsEmpty())
                                    {
                                        return FReply::Unhandled();
                                    }

                                    DraggedGroupIndex = SourceIndex;
                                    DropTargetGroupIndex.Reset();
                                    DropTargetZone.Reset();
                                    if (CategoryList.IsValid())
                                    {
                                        CategoryList->Invalidate(EInvalidateWidgetReason::Paint);
                                    }

                                    TSharedRef<FDragAndDropVerticalBoxOp> DragOp =
                                        MakeShared<FDragAndDropVerticalBoxOp>();
                                    DragOp->SlotIndexBeingDragged = SourceIndex;
                                    DragOp->SlotBeingDragged = SourceSlot;
                                    return FReply::Handled().BeginDragDrop(DragOp);
                                })
                            .OnDragEnter_Lambda(
                                [&DraggedGroupIndex, &CategoryList](const FDragDropEvent& DragDropEvent)
                                {
                                    const TSharedPtr<FDragAndDropVerticalBoxOp> DragOp =
                                        DragDropEvent.GetOperationAs<FDragAndDropVerticalBoxOp>();
                                    if (DragOp.IsValid())
                                    {
                                        DraggedGroupIndex = DragOp->SlotIndexBeingDragged;
                                        if (CategoryList.IsValid())
                                        {
                                            CategoryList->Invalidate(EInvalidateWidgetReason::Paint);
                                        }
                                    }
                                })
                            .OnDragLeave_Lambda(
                                [&DraggedGroupIndex, &DropTargetGroupIndex, &DropTargetZone, &CategoryList](const FDragDropEvent&)
                                {
                                    DraggedGroupIndex.Reset();
                                    DropTargetGroupIndex.Reset();
                                    DropTargetZone.Reset();
                                    if (CategoryList.IsValid())
                                    {
                                        CategoryList->Invalidate(EInvalidateWidgetReason::Paint);
                                    }
                                })
                            .OnCanAcceptDropAdvanced_Lambda(
                                [&CategorySearchText, &DropTargetGroupIndex, &DropTargetZone, &CategoryList](
                                   const FDragDropEvent& DragDropEvent,
                                   SDragAndDropVerticalBox::EItemDropZone DropZone,
                                   int32 TargetIndex,
                                   SVerticalBox::FSlot*) -> TOptional<SDragAndDropVerticalBox::EItemDropZone>
                                {
                                    const TSharedPtr<FDragAndDropVerticalBoxOp> DragOp =
                                        DragDropEvent.GetOperationAs<FDragAndDropVerticalBoxOp>();

                                    if (!CategorySearchText.TrimStartAndEnd().IsEmpty()
                                        || !DragOp.IsValid()
                                        || DragOp->SlotIndexBeingDragged == TargetIndex)
                                    {
                                        DropTargetGroupIndex.Reset();
                                        DropTargetZone.Reset();
                                        if (CategoryList.IsValid())
                                        {
                                            CategoryList->Invalidate(EInvalidateWidgetReason::Paint);
                                        }
                                        return TOptional<SDragAndDropVerticalBox::EItemDropZone>();
                                    }

                                    DropTargetGroupIndex = TargetIndex;
                                    DropTargetZone = DropZone;
                                    if (CategoryList.IsValid())
                                    {
                                        CategoryList->Invalidate(EInvalidateWidgetReason::Paint);
                                    }

                                    return DropZone;
                                })
                            .OnAcceptDrop_Lambda(
                                [&WorkingGroups, &bWorkingCustomOrder, &DraggedGroupIndex, &DropTargetGroupIndex, &DropTargetZone, &RebuildCategoryList](
                                    const FDragDropEvent& DragDropEvent,
                                    SDragAndDropVerticalBox::EItemDropZone DropZone,
                                    int32 TargetIndex,
                                    SVerticalBox::FSlot*)
                                {
                                    const TSharedPtr<FDragAndDropVerticalBoxOp> DragOp =
                                        DragDropEvent.GetOperationAs<FDragAndDropVerticalBoxOp>();
                                    if (!DragOp.IsValid())
                                    {
                                        return FReply::Unhandled();
                                    }

                                    const int32 SourceIndex = DragOp->SlotIndexBeingDragged;
                                    if (!WorkingGroups.IsValidIndex(SourceIndex)
                                        || !WorkingGroups.IsValidIndex(TargetIndex))
                                    {
                                        return FReply::Unhandled();
                                    }

                                    FPruneCategoryGroupInfo MovingGroup = WorkingGroups[SourceIndex];
                                    WorkingGroups.RemoveAt(SourceIndex);

                                    int32 InsertIndex = TargetIndex;
                                    if (SourceIndex < TargetIndex)
                                    {
                                        --InsertIndex;
                                    }
                                    if (DropZone == SDragAndDropVerticalBox::EItemDropZone::BelowItem)
                                    {
                                        ++InsertIndex;
                                    }

                                    InsertIndex = FMath::Clamp(InsertIndex, 0, WorkingGroups.Num());
                                    WorkingGroups.Insert(MoveTemp(MovingGroup), InsertIndex);
                                    bWorkingCustomOrder = true;
                                    DraggedGroupIndex.Reset();
                                    DropTargetGroupIndex.Reset();
                                    DropTargetZone.Reset();
                                    RebuildCategoryList();
                                    return FReply::Handled();
                                })
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

        if (CategoryList.IsValid())
        {
            // Prune draws its own configurable insertion line inside each row.
            // Keep the native drag box's paint-only drop brushes invisible.
            const FSlateColorBrush TransparentDropIndicatorBrush{ FLinearColor::Transparent };
            CategoryList->SetDropIndicator_Above(TransparentDropIndicatorBrush);
            CategoryList->SetDropIndicator_Below(TransparentDropIndicatorBrush);
        }

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
        Result.Description = DescriptionTextBox.IsValid()
            ? DescriptionTextBox->GetText().ToString().TrimStartAndEnd()
            : InitialData.Description;
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
                Result.Description,
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
                Result.Description,
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
                Result.Description,
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

    static void OpenDuplicateFilterEditor(
        const TSharedRef<FPruneState>& State,
        const TSharedRef<FPruneLayoutContext>& Context,
        const FPruneEditableFilter& SourceFilter,
        const FPruneFilterEditorData* EditedSourceData = nullptr)
    {
        FPruneFilterEditorData SourceData;
        if (EditedSourceData != nullptr)
        {
            SourceData = *EditedSourceData;
        }
        else if (!State->BuildEditorData(Context, SourceFilter, SourceData))
        {
            return;
        }

        FPruneFilterEditorData DuplicateData;
        DuplicateData.Name = State->MakeSuggestedDuplicateName(
            Context,
            SourceData.Name);
        DuplicateData.Description = SourceData.Description;
        DuplicateData.bGlobal = SourceData.bGlobal;
        DuplicateData.ScopeClassName = DuplicateData.bGlobal
            ? NAME_None
            : Context->GetActorClassName();
        DuplicateData.HiddenCategories = SourceData.HiddenCategories;
        DuplicateData.OrderedCategoryIds = SourceData.OrderedCategoryIds;
        DuplicateData.bNativeFilter = false;
        DuplicateData.bHasNativeOverride = false;

        const TOptional<FFilterEditorResult> DuplicateResult =
            ShowFilterEditor(
                Context,
                DuplicateData,
                true,
                false);

        if (DuplicateResult.IsSet())
        {
            HandleEditorResult(
                State,
                Context,
                nullptr,
                DuplicateResult.GetValue());
        }
    }

    static void OpenSpecificFilterEditor(
        const TSharedRef<FPruneState>& State,
        const TSharedRef<FPruneLayoutContext>& Context,
        const FPruneEditableFilter& Filter)
    {
        FPruneFilterEditorData InitialData;
        if (!State->BuildEditorData(Context, Filter, InitialData))
        {
            return;
        }

        const TOptional<FFilterEditorResult> Result = ShowFilterEditor(
            Context,
            InitialData,
            false,
            Filter.Kind == EPruneFilterKind::Native);

        if (!Result.IsSet())
        {
            return;
        }

        const FFilterEditorResult& EditorResult = Result.GetValue();
        if (EditorResult.Action == EFilterEditorAction::Duplicate)
        {
            FPruneFilterEditorData EditedSourceData = InitialData;
            EditedSourceData.Name = EditorResult.Name.IsEmpty()
                ? InitialData.Name
                : EditorResult.Name;
            EditedSourceData.Description = EditorResult.Description;
            EditedSourceData.bGlobal = EditorResult.bGlobal;
            EditedSourceData.HiddenCategories = EditorResult.HiddenCategories;
            EditedSourceData.OrderedCategoryIds = EditorResult.OrderedCategoryIds;
            OpenDuplicateFilterEditor(
                State,
                Context,
                Filter,
                &EditedSourceData);
            return;
        }

        HandleEditorResult(
            State,
            Context,
            &Filter,
            EditorResult);
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
        if (!State->ResolveSingleActiveFilter(Context.ToSharedRef(), Filter))
        {
            return;
        }

        OpenSpecificFilterEditor(State.ToSharedRef(), Context.ToSharedRef(), Filter);
    }

    static FText BuildManagedFilterMetaText(const FPruneManagedFilterInfo& Info)
    {
        if (Info.Kind == EPruneFilterKind::Native)
        {
            if (!Info.bHasNativeOverride)
            {
                return LOCTEXT("ManagedFilterEpic", "Epic");
            }

            if (Info.bGlobal)
            {
                return LOCTEXT("ManagedFilterEpicGlobal", "Epic Override | Global");
            }

            return FText::Format(
                LOCTEXT("ManagedFilterEpicClass", "Epic Override | Class: {0}"),
                FText::FromName(Info.ScopeClassName));
        }

        if (Info.bGlobal)
        {
            return LOCTEXT("ManagedFilterPruneGlobal", "Prune | Global");
        }

        return FText::Format(
            LOCTEXT("ManagedFilterPruneClass", "Prune | Class: {0}"),
            FText::FromName(Info.ScopeClassName));
    }

    static void OpenFilterManager(
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
        if (!Context.IsValid() || Context->GetActorClass() == nullptr)
        {
            return;
        }

        const UPruneSettings* Settings = UPruneSettings::Get();
        TArray<FPruneManagedFilterInfo> WorkingFilters =
            State->GetManagedFilters(Context.ToSharedRef());
        TSharedPtr<SDragAndDropVerticalBox> FilterList;
        FString ManagerSearchText;
        TOptional<int32> DraggedFilterIndex;
        TOptional<int32> DropTargetFilterIndex;
        TOptional<SDragAndDropVerticalBox::EItemDropZone> DropTargetFilterZone;
        static const FSlateColorBrush ManagerRowBrush{ FLinearColor::White };

        TSharedRef<SWindow> Dialog =
            SNew(SWindow)
            .Title(FText::Format(
                LOCTEXT("FilterManagerTitle", "Prune Filter Manager - {0}"),
                Context->GetSelectionLabel()))
            .ClientSize(FVector2D(
                FMath::Clamp(Settings->FilterManagerDefaultWidth, 480.0f, 1600.0f),
                FMath::Clamp(Settings->FilterManagerDefaultHeight, 420.0f, 1600.0f)))
            .SizingRule(ESizingRule::UserSized)
            .SupportsMaximize(false)
            .SupportsMinimize(false);

        const TWeakPtr<SWindow> WeakDialog = Dialog;

        TFunction<void()> RebuildFilterList;
        RebuildFilterList = [&]()
        {
            if (!FilterList.IsValid())
            {
                return;
            }

            FilterList->ClearChildren();

            if (WorkingFilters.IsEmpty())
            {
                FilterList->AddSlot()
                .AutoHeight()
                .Padding(8.0f, 16.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("NoManagedFilters", "No editable filters are available for this Actor class."))
                    .Justification(ETextJustify::Center)
                    .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                ];
                return;
            }

            const FString Search = ManagerSearchText.TrimStartAndEnd();
            const bool bSearchActive = !Search.IsEmpty();
            int32 VisibleFilterCount = 0;
            const TArray<FString> CheckedSectionLabels = Context->GetCheckedSectionLabels();

            for (int32 Index = 0; Index < WorkingFilters.Num(); ++Index)
            {
                const FPruneManagedFilterInfo Info = WorkingFilters[Index];
                const int32 WorkingIndex = Index;
                const FText MetaText = BuildManagedFilterMetaText(Info);
                const bool bIsActive = CheckedSectionLabels.Contains(Info.DisplayName.ToString());
                const FText DisplayMetaText = bIsActive
                    ? FText::Format(
                        LOCTEXT("ManagedFilterActiveMeta", "{0} | Active"),
                        MetaText)
                    : MetaText;

                if (bSearchActive)
                {
                    const FString SearchableText = FString::Printf(
                        TEXT("%s %s %s"),
                        *Info.DisplayName.ToString(),
                        *DisplayMetaText.ToString(),
                        *Info.Description);
                    if (!SearchableText.Contains(Search, ESearchCase::IgnoreCase))
                    {
                        continue;
                    }
                }

                ++VisibleFilterCount;

                FilterList->AddSlot()
                .AutoHeight()
                .Padding(2.0f, 2.0f)
                [
                    SNew(SOverlay)

                    + SOverlay::Slot()
                    [
                        SNew(SBorder)
                        .BorderImage(&ManagerRowBrush)
                        .BorderBackgroundColor_Lambda(
                            [Settings, &DraggedFilterIndex, WorkingIndex]()
                            {
                                return DraggedFilterIndex.IsSet()
                                    && DraggedFilterIndex.GetValue() == WorkingIndex
                                    ? FSlateColor(Settings->ReorderDragColor)
                                    : FSlateColor(FLinearColor::Transparent);
                            })
                        .Padding(FMargin(6.0f, 5.0f))
                        [
                            SNew(SHorizontalBox)

                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(2.0f, 0.0f, 8.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(TEXT("≡")))
                                .TextStyle(FAppStyle::Get(), "SmallText")
                                .ColorAndOpacity_Lambda(
                                    [Settings, &DraggedFilterIndex, WorkingIndex]()
                                    {
                                        return DraggedFilterIndex.IsSet()
                                            && DraggedFilterIndex.GetValue() == WorkingIndex
                                            ? FSlateColor(Settings->ReorderDraggedContentColor)
                                            : FSlateColor::UseForeground();
                                    })
                                .ToolTipText(LOCTEXT("DragFilterButtonTooltip", "Drag to reorder this filter button for the current Actor class."))
                            ]

                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(SVerticalBox)

                                + SVerticalBox::Slot()
                                .AutoHeight()
                                [
                                    SNew(STextBlock)
                                    .Text(Info.DisplayName)
                                    .ColorAndOpacity_Lambda(
                                        [Settings, &DraggedFilterIndex, WorkingIndex]()
                                        {
                                            return DraggedFilterIndex.IsSet()
                                                && DraggedFilterIndex.GetValue() == WorkingIndex
                                                ? FSlateColor(Settings->ReorderDraggedContentColor)
                                                : FSlateColor::UseForeground();
                                        })
                                ]

                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 1.0f, 0.0f, 0.0f)
                                [
                                    SNew(STextBlock)
                                    .Text(DisplayMetaText)
                                    .TextStyle(FAppStyle::Get(), "SmallText")
                                    .ColorAndOpacity_Lambda(
                                        [Settings, &DraggedFilterIndex, WorkingIndex, bIsActive]()
                                        {
                                            return DraggedFilterIndex.IsSet()
                                                && DraggedFilterIndex.GetValue() == WorkingIndex
                                                ? FSlateColor(Settings->ReorderDraggedContentColor)
                                                : bIsActive
                                                    ? FSlateColor(EStyleColor::AccentBlue)
                                                    : FSlateColor(EStyleColor::ForegroundHeader);
                                        })
                                ]

                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 3.0f, 0.0f, 0.0f)
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(Info.Description))
                                    .TextStyle(FAppStyle::Get(), "SmallText")
                                    .AutoWrapText(true)
                                    .Visibility(Info.Description.IsEmpty()
                                        ? EVisibility::Collapsed
                                        : EVisibility::HitTestInvisible)
                                    .ColorAndOpacity_Lambda(
                                        [Settings, &DraggedFilterIndex, WorkingIndex]()
                                        {
                                            return DraggedFilterIndex.IsSet()
                                                && DraggedFilterIndex.GetValue() == WorkingIndex
                                                ? FSlateColor(Settings->ReorderDraggedContentColor)
                                                : FSlateColor(EStyleColor::ForegroundHeader);
                                        })
                                ]
                            ]

                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(SButton)
                                .Text(LOCTEXT("ManageEditFilter", "Edit"))
                                .ToolTipText(LOCTEXT("ManageEditFilterTooltip", "Edit this filter's categories, order, description, and scope."))
                                .OnClicked_Lambda([State, Context, Info, &WorkingFilters, &RebuildFilterList]()
                                {
                                    FPruneEditableFilter Filter;
                                    if (State->ResolveManagedFilter(Context.ToSharedRef(), Info.OrderKey, Filter))
                                    {
                                        OpenSpecificFilterEditor(
                                            State.ToSharedRef(),
                                            Context.ToSharedRef(),
                                            Filter);
                                        WorkingFilters = State->GetManagedFilters(Context.ToSharedRef());
                                        RebuildFilterList();
                                    }
                                    return FReply::Handled();
                                })
                            ]

                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                            [
                                SNew(SButton)
                                .Text(LOCTEXT("ManageDuplicateFilter", "Duplicate..."))
                                .ToolTipText(Info.Kind == EPruneFilterKind::Native
                                    ? LOCTEXT("ManageDuplicateEpicTooltip", "Create a new Prune filter from this Epic filter without changing the Epic filter itself.")
                                    : LOCTEXT("ManageDuplicatePruneTooltip", "Create a new Prune filter prefilled from this filter."))
                                .OnClicked_Lambda([State, Context, Info, &WorkingFilters, &RebuildFilterList]()
                                {
                                    FPruneEditableFilter Filter;
                                    if (State->ResolveManagedFilter(Context.ToSharedRef(), Info.OrderKey, Filter))
                                    {
                                        OpenDuplicateFilterEditor(
                                            State.ToSharedRef(),
                                            Context.ToSharedRef(),
                                            Filter);
                                        WorkingFilters = State->GetManagedFilters(Context.ToSharedRef());
                                        RebuildFilterList();
                                    }
                                    return FReply::Handled();
                                })
                            ]
                        ]
                    ]

                    + SOverlay::Slot()
                    .VAlign(VAlign_Top)
                    [
                        SNew(SBox)
                        .HeightOverride(FMath::Clamp(Settings->ReorderDropLineThickness, 1.0f, 8.0f))
                        .Visibility(EVisibility::HitTestInvisible)
                        [
                            SNew(SBorder)
                            .BorderImage(&ManagerRowBrush)
                            .BorderBackgroundColor_Lambda(
                                [Settings, &DropTargetFilterIndex, &DropTargetFilterZone, WorkingIndex]()
                                {
                                    return DropTargetFilterIndex.IsSet()
                                        && DropTargetFilterIndex.GetValue() == WorkingIndex
                                        && DropTargetFilterZone.IsSet()
                                        && DropTargetFilterZone.GetValue() == SDragAndDropVerticalBox::EItemDropZone::AboveItem
                                        ? FSlateColor(Settings->ReorderDropLineColor)
                                        : FSlateColor(FLinearColor::Transparent);
                                })
                            .Padding(0.0f)
                        ]
                    ]

                    + SOverlay::Slot()
                    .VAlign(VAlign_Bottom)
                    [
                        SNew(SBox)
                        .HeightOverride(FMath::Clamp(Settings->ReorderDropLineThickness, 1.0f, 8.0f))
                        .Visibility(EVisibility::HitTestInvisible)
                        [
                            SNew(SBorder)
                            .BorderImage(&ManagerRowBrush)
                            .BorderBackgroundColor_Lambda(
                                [Settings, &DropTargetFilterIndex, &DropTargetFilterZone, WorkingIndex]()
                                {
                                    return DropTargetFilterIndex.IsSet()
                                        && DropTargetFilterIndex.GetValue() == WorkingIndex
                                        && DropTargetFilterZone.IsSet()
                                        && DropTargetFilterZone.GetValue() == SDragAndDropVerticalBox::EItemDropZone::BelowItem
                                        ? FSlateColor(Settings->ReorderDropLineColor)
                                        : FSlateColor(FLinearColor::Transparent);
                                })
                            .Padding(0.0f)
                        ]
                    ]
                ];
            }

            if (bSearchActive && VisibleFilterCount == 0)
            {
                FilterList->AddSlot()
                .AutoHeight()
                .Padding(8.0f, 16.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("NoManagedFilterMatches", "No filters match this search."))
                    .Justification(ETextJustify::Center)
                    .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                ];
            }

            FilterList->Invalidate(EInvalidateWidgetReason::Layout);
        };

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
                            "FilterManagerDescription",
                            "Manage filters available for {0}. Drag rows to change the filter-button order for this Actor class. All remains after the editable filters."),
                        Context->GetSelectionLabel()))
                    .AutoWrapText(true)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                [
                    SNew(SSearchBox)
                    .HintText(LOCTEXT("FilterManagerSearchHint", "Search filters"))
                    .OnTextChanged_Lambda([&ManagerSearchText, &DraggedFilterIndex, &DropTargetFilterIndex, &DropTargetFilterZone, &RebuildFilterList](const FText& NewText)
                    {
                        ManagerSearchText = NewText.ToString();
                        DraggedFilterIndex.Reset();
                        DropTargetFilterIndex.Reset();
                        DropTargetFilterZone.Reset();
                        RebuildFilterList();
                    })
                    .ToolTipText(LOCTEXT("FilterManagerSearchTooltip", "Search filter names, descriptions, and scope. Clear the search to reorder filter buttons."))
                ]

                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                .Padding(0.0f, 8.0f, 0.0f, 10.0f)
                [
                    SNew(SBorder)
                    .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
                    .Padding(4.0f)
                    [
                        SNew(SScrollBox)
                        + SScrollBox::Slot()
                        [
                            SAssignNew(FilterList, SDragAndDropVerticalBox)
                            .OnDragDetected_Lambda(
                                [&ManagerSearchText, &DraggedFilterIndex, &DropTargetFilterIndex, &DropTargetFilterZone, &FilterList](
                                    const FGeometry&,
                                    const FPointerEvent&,
                                    int32 SourceIndex,
                                    SVerticalBox::FSlot* SourceSlot)
                                {
                                    if (!ManagerSearchText.TrimStartAndEnd().IsEmpty())
                                    {
                                        return FReply::Unhandled();
                                    }

                                    DraggedFilterIndex = SourceIndex;
                                    DropTargetFilterIndex.Reset();
                                    DropTargetFilterZone.Reset();
                                    if (FilterList.IsValid())
                                    {
                                        FilterList->Invalidate(EInvalidateWidgetReason::Paint);
                                    }

                                    TSharedRef<FDragAndDropVerticalBoxOp> DragOp =
                                        MakeShared<FDragAndDropVerticalBoxOp>();
                                    DragOp->SlotIndexBeingDragged = SourceIndex;
                                    DragOp->SlotBeingDragged = SourceSlot;
                                    return FReply::Handled().BeginDragDrop(DragOp);
                                })
                            .OnDragEnter_Lambda(
                                [&DraggedFilterIndex, &FilterList](const FDragDropEvent& DragDropEvent)
                                {
                                    const TSharedPtr<FDragAndDropVerticalBoxOp> DragOp =
                                        DragDropEvent.GetOperationAs<FDragAndDropVerticalBoxOp>();
                                    if (DragOp.IsValid())
                                    {
                                        DraggedFilterIndex = DragOp->SlotIndexBeingDragged;
                                        if (FilterList.IsValid())
                                        {
                                            FilterList->Invalidate(EInvalidateWidgetReason::Paint);
                                        }
                                    }
                                })
                            .OnDragLeave_Lambda(
                                [&DraggedFilterIndex, &DropTargetFilterIndex, &DropTargetFilterZone, &FilterList](const FDragDropEvent&)
                                {
                                    DraggedFilterIndex.Reset();
                                    DropTargetFilterIndex.Reset();
                                    DropTargetFilterZone.Reset();
                                    if (FilterList.IsValid())
                                    {
                                        FilterList->Invalidate(EInvalidateWidgetReason::Paint);
                                    }
                                })
                            .OnCanAcceptDropAdvanced_Lambda(
                                [&ManagerSearchText, &DropTargetFilterIndex, &DropTargetFilterZone, &FilterList](
                                    const FDragDropEvent& DragDropEvent,
                                    SDragAndDropVerticalBox::EItemDropZone DropZone,
                                    int32 TargetIndex,
                                    SVerticalBox::FSlot*) -> TOptional<SDragAndDropVerticalBox::EItemDropZone>
                                {
                                    const TSharedPtr<FDragAndDropVerticalBoxOp> DragOp =
                                        DragDropEvent.GetOperationAs<FDragAndDropVerticalBoxOp>();
                                    if (!ManagerSearchText.TrimStartAndEnd().IsEmpty()
                                        || !DragOp.IsValid()
                                        || DragOp->SlotIndexBeingDragged == TargetIndex)
                                    {
                                        DropTargetFilterIndex.Reset();
                                        DropTargetFilterZone.Reset();
                                        if (FilterList.IsValid())
                                        {
                                            FilterList->Invalidate(EInvalidateWidgetReason::Paint);
                                        }
                                        return TOptional<SDragAndDropVerticalBox::EItemDropZone>();
                                    }

                                    DropTargetFilterIndex = TargetIndex;
                                    DropTargetFilterZone = DropZone;
                                    if (FilterList.IsValid())
                                    {
                                        FilterList->Invalidate(EInvalidateWidgetReason::Paint);
                                    }
                                    return DropZone;
                                })
                            .OnAcceptDrop_Lambda(
                                [State, Context, WeakDialog, &ManagerSearchText, &WorkingFilters, &DraggedFilterIndex, &DropTargetFilterIndex, &DropTargetFilterZone, &RebuildFilterList, &FilterList](
                                    const FDragDropEvent& DragDropEvent,
                                    SDragAndDropVerticalBox::EItemDropZone DropZone,
                                    int32 TargetIndex,
                                    SVerticalBox::FSlot*)
                                {
                                    if (!ManagerSearchText.TrimStartAndEnd().IsEmpty())
                                    {
                                        return FReply::Unhandled();
                                    }

                                    const TSharedPtr<FDragAndDropVerticalBoxOp> DragOp =
                                        DragDropEvent.GetOperationAs<FDragAndDropVerticalBoxOp>();
                                    if (!DragOp.IsValid())
                                    {
                                        return FReply::Unhandled();
                                    }

                                    const int32 SourceIndex = DragOp->SlotIndexBeingDragged;
                                    if (!WorkingFilters.IsValidIndex(SourceIndex)
                                        || !WorkingFilters.IsValidIndex(TargetIndex))
                                    {
                                        return FReply::Unhandled();
                                    }

                                    FPruneManagedFilterInfo MovingFilter = WorkingFilters[SourceIndex];
                                    WorkingFilters.RemoveAt(SourceIndex);

                                    int32 InsertIndex = TargetIndex;
                                    if (SourceIndex < TargetIndex)
                                    {
                                        --InsertIndex;
                                    }
                                    if (DropZone == SDragAndDropVerticalBox::EItemDropZone::BelowItem)
                                    {
                                        ++InsertIndex;
                                    }
                                    InsertIndex = FMath::Clamp(InsertIndex, 0, WorkingFilters.Num());
                                    WorkingFilters.Insert(MoveTemp(MovingFilter), InsertIndex);

                                    TArray<FString> OrderedKeys;
                                    OrderedKeys.Reserve(WorkingFilters.Num());
                                    for (const FPruneManagedFilterInfo& Filter : WorkingFilters)
                                    {
                                        OrderedKeys.Add(Filter.OrderKey);
                                    }
                                    State->SetFilterButtonOrder(Context.ToSharedRef(), OrderedKeys);

                                    DraggedFilterIndex.Reset();
                                    DropTargetFilterIndex.Reset();
                                    DropTargetFilterZone.Reset();

                                    // Rebuild after SDragAndDropVerticalBox finishes its OnDrop call.
                                    // Replacing child slots synchronously here can leave the manager
                                    // painting cached slot geometry until the modal closes.
                                    if (const TSharedPtr<SWindow> ManagerWindow = WeakDialog.Pin())
                                    {
                                        ManagerWindow->RegisterActiveTimer(
                                            0.0f,
                                            FWidgetActiveTimerDelegate::CreateLambda(
                                                [State, Context, &WorkingFilters, &RebuildFilterList, &FilterList](double, float)
                                                {
                                                    WorkingFilters = State->GetManagedFilters(Context.ToSharedRef());
                                                    RebuildFilterList();
                                                    if (FilterList.IsValid())
                                                    {
                                                        FilterList->Invalidate(EInvalidateWidgetReason::Layout);
                                                    }
                                                    return EActiveTimerReturnType::Stop;
                                                }));
                                    }
                                    else
                                    {
                                        WorkingFilters = State->GetManagedFilters(Context.ToSharedRef());
                                        RebuildFilterList();
                                    }

                                    return FReply::Handled();
                                })
                        ]
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("ManagerNewFilter", "New Filter..."))
                        .OnClicked_Lambda([State, Context, &WorkingFilters, &RebuildFilterList]()
                        {
                            FPruneFilterEditorData InitialData;
                            InitialData.bGlobal = false;
                            InitialData.ScopeClassName = Context->GetActorClassName();

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
                                WorkingFilters = State->GetManagedFilters(Context.ToSharedRef());
                                RebuildFilterList();
                            }
                            return FReply::Handled();
                        })
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("ManagerResetOrder", "Reset Button Order"))
                        .ToolTipText(LOCTEXT("ManagerResetOrderTooltip", "Restore Unreal's normal filter-button order for this Actor class."))
                        .IsEnabled_Lambda([State, Context]()
                        {
                            return State->HasCustomFilterButtonOrder(Context.ToSharedRef());
                        })
                        .OnClicked_Lambda([State, Context, &WorkingFilters, &RebuildFilterList]()
                        {
                            State->SetFilterButtonOrder(Context.ToSharedRef(), TArray<FString>());
                            WorkingFilters = State->GetManagedFilters(Context.ToSharedRef());
                            RebuildFilterList();
                            return FReply::Handled();
                        })
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(6.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("ManagerSettings", "Settings..."))
                        .ToolTipText(LOCTEXT("ManagerSettingsTooltip", "Open Project Settings > Plugins > Prune."))
                        .OnClicked_Lambda([]()
                        {
                            ISettingsModule& SettingsModule =
                                FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings"));
                            SettingsModule.ShowViewer(TEXT("Project"), TEXT("Plugins"), TEXT("Prune"));
                            return FReply::Handled();
                        })
                    ]

                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    .Padding(10.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([State, Context]()
                        {
                            return State->HasCustomFilterButtonOrder(Context.ToSharedRef())
                                ? LOCTEXT("ManagerImmediateCustomOrder", "Custom button order | Changes apply immediately")
                                : LOCTEXT("ManagerImmediateNativeOrder", "Unreal button order | Changes apply immediately");
                        })
                        .TextStyle(FAppStyle::Get(), "SmallText")
                        .Justification(ETextJustify::Right)
                        .ColorAndOpacity(FSlateColor(EStyleColor::ForegroundHeader))
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SNew(SButton)
                        .Text(LOCTEXT("DoneFilterManager", "Done"))
                        .ToolTipText(LOCTEXT("DoneFilterManagerTooltip", "Close the manager. Filter changes are applied immediately."))
                        .OnClicked_Lambda([Dialog]()
                        {
                            Dialog->RequestDestroyWindow();
                            return FReply::Handled();
                        })
                    ]
                ]
            ]);

        RebuildFilterList();
        if (FilterList.IsValid())
        {
            const FSlateColorBrush TransparentDropIndicatorBrush{ FLinearColor::Transparent };
            FilterList->SetDropIndicator_Above(TransparentDropIndicatorBrush);
            FilterList->SetDropIndicator_Below(TransparentDropIndicatorBrush);
        }

        FSlateApplication::Get().AddModalWindow(
            Dialog,
            FSlateApplication::Get().GetActiveTopLevelWindow(),
            false);
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
        return SNew(SBox)
            .WidthOverride(24.0f)
            .HeightOverride(24.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .ContentPadding(2.0f)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .ToolTipText(LOCTEXT("NewFilterTooltip", "Create a new Prune filter"))
                .OnClicked_Lambda([WeakState, WeakDetailsView]()
                {
                    OpenNewFilterEditor(WeakState, WeakDetailsView);
                    return FReply::Handled();
                })
                [
                    SNew(SBox)
                    .WidthOverride(16.0f)
                    .HeightOverride(16.0f)
                    [
                        SNew(SImage)
                        .ColorAndOpacity(FSlateColor::UseForeground())
                        .Image(FAppStyle::Get().GetBrush("Icons.Plus"))
                    ]
                ]
            ];
    }

    static TSharedRef<SWidget> BuildManagerButton(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<const IDetailsView>& WeakDetailsView)
    {
        return SNew(SBox)
            .WidthOverride(24.0f)
            .HeightOverride(24.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .ContentPadding(2.0f)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
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
                    return Context.IsValid() && Context->GetActorClass() != nullptr;
                })
                .ToolTipText(LOCTEXT("FilterManagerTooltip", "Manage Prune filters and reorder filter buttons for this Actor class"))
                .OnClicked_Lambda([WeakState, WeakDetailsView]()
                {
                    OpenFilterManager(WeakState, WeakDetailsView);
                    return FReply::Handled();
                })
                [
                    SNew(SBox)
                    .WidthOverride(16.0f)
                    .HeightOverride(16.0f)
                    [
                        SNew(SImage)
                        .ColorAndOpacity(FSlateColor::UseForeground())
                        .Image(GetFilterManagerIconBrush())
                    ]
                ]
            ];
    }

    static TSharedRef<SWidget> BuildFilterGearIcon()
    {
        return SNew(SBox)
            .WidthOverride(16.0f)
            .HeightOverride(16.0f)
            [
                SNew(SImage)
                .ColorAndOpacity(FSlateColor::UseForeground())
                .Image(GetFilterEditIconBrush())
            ];
    }

    static TSharedRef<SWidget> BuildEditButton(
        const TWeakPtr<FPruneState>& WeakState,
        const TWeakPtr<const IDetailsView>& WeakDetailsView)
    {
        return SNew(SBox)
            .WidthOverride(24.0f)
            .HeightOverride(24.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                .ContentPadding(2.0f)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
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
                ]
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

        TSharedRef<SOverlay> PruneSectionRow =
            SNew(SOverlay)
            .AddMetaData<FTagMetaData>(PruneSectionRowWrapperTag)

            + SOverlay::Slot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            [
                SNew(SBox)
                .Padding(FMargin(0.0f, 0.0f, 88.0f, 0.0f))
                [
                    SectionSelectorRef
                ]
            ]

            + SOverlay::Slot()
            .HAlign(HAlign_Right)
            .VAlign(VAlign_Bottom)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                [
                    BuildPlusButton(WeakState, WeakDetailsView)
                ]

                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                [
                    BuildManagerButton(WeakState, WeakDetailsView)
                ]

                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(4.0f, 0.0f, 0.0f, 0.0f)
                [
                    BuildEditButton(WeakState, WeakDetailsView)
                ]
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
            TEXT("Prune added +, manager, and edit controls beside the native Details SectionView."));
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
