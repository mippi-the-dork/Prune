// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneState.h"

#include "DetailCategoryBuilder.h"
#include "GameFramework/Actor.h"
#include "IDetailsView.h"
#include "Layout/Children.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Guid.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SWidget.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogPrune, Log, All);

namespace PruneStatePrivate
{
    static const TCHAR* RootSection = TEXT("Prune.UserState");
    static const TCHAR* PresetIdsKey = TEXT("PresetIds");
    static const TCHAR* NativeOverrideIdsKey = TEXT("NativeOverrideIds");

    static const TCHAR* HiddenCategoriesKey = TEXT("HiddenCategoryIds");
    static const TCHAR* AddedCategoriesKey = TEXT("AddedCategoryIds");
    static const TCHAR* RemovedCategoriesKey = TEXT("RemovedCategoryIds");
    static const TCHAR* CategoryOrderKey = TEXT("CategoryOrder");
    static const TCHAR* NameKey = TEXT("Name");
    static const TCHAR* GlobalKey = TEXT("Global");
    static const TCHAR* ClassNameKey = TEXT("ClassName");
    static const TCHAR* SectionNameKey = TEXT("SectionName");
    static const TCHAR* DisplayNameKey = TEXT("DisplayName");
    static const TCHAR* SectionOrderKey = TEXT("SectionOrder");

    static constexpr int32 FirstPruneSectionOrder = 10000;

    static FString MakePresetSection(const FString& PresetId)
    {
        return FString::Printf(TEXT("Prune.Preset.%s"), *PresetId);
    }

    static FString MakeNativeOverrideSection(const FString& OverrideId)
    {
        return FString::Printf(TEXT("Prune.NativeOverride.%s"), *OverrideId);
    }

    static TArray<FString> NamesToStrings(const TSet<FName>& Names)
    {
        TArray<FString> Result;
        Result.Reserve(Names.Num());

        for (const FName Name : Names)
        {
            if (!Name.IsNone())
            {
                Result.Add(Name.ToString());
            }
        }

        Result.Sort();
        return Result;
    }

    static TArray<FString> OrderedNamesToStrings(const TArray<FName>& Names)
    {
        TArray<FString> Result;
        Result.Reserve(Names.Num());

        for (const FName Name : Names)
        {
            if (!Name.IsNone())
            {
                Result.Add(Name.ToString());
            }
        }

        return Result;
    }

    static TSet<FName> StringsToNames(const TArray<FString>& Strings)
    {
        TSet<FName> Result;

        for (const FString& Value : Strings)
        {
            const FString Trimmed = Value.TrimStartAndEnd();
            if (!Trimmed.IsEmpty())
            {
                Result.Add(FName(*Trimmed));
            }
        }

        return Result;
    }

    static TArray<FName> StringsToOrderedNames(const TArray<FString>& Strings)
    {
        TArray<FName> Result;
        TSet<FName> Seen;

        for (const FString& Value : Strings)
        {
            const FString Trimmed = Value.TrimStartAndEnd();
            if (!Trimmed.IsEmpty())
            {
                const FName Name(*Trimmed);
                if (!Seen.Contains(Name))
                {
                    Seen.Add(Name);
                    Result.Add(Name);
                }
            }
        }

        return Result;
    }

    static SCheckBox* FindCheckBoxRecursive(SWidget& Widget)
    {
        if (Widget.GetType() == FName(TEXT("SCheckBox")))
        {
            return static_cast<SCheckBox*>(&Widget);
        }

        FChildren* Children = Widget.GetChildren();
        if (Children == nullptr)
        {
            return nullptr;
        }

        for (int32 Index = 0; Index < Children->Num(); ++Index)
        {
            if (SCheckBox* CheckBox = FindCheckBoxRecursive(Children->GetChildAt(Index).Get()))
            {
                return CheckBox;
            }
        }

        return nullptr;
    }

    static STextBlock* FindTextBlockRecursive(SWidget& Widget)
    {
        if (Widget.GetType() == FName(TEXT("STextBlock")))
        {
            return static_cast<STextBlock*>(&Widget);
        }

        FChildren* Children = Widget.GetChildren();
        if (Children == nullptr)
        {
            return nullptr;
        }

        for (int32 Index = 0; Index < Children->Num(); ++Index)
        {
            if (STextBlock* TextBlock = FindTextBlockRecursive(Children->GetChildAt(Index).Get()))
            {
                return TextBlock;
            }
        }

        return nullptr;
    }
}

FPruneLayoutContext::FPruneLayoutContext(
    FText InSelectionLabel,
    TSharedPtr<const IDetailsView> InDetailsView,
    UClass* InActorClass)
    : SelectionLabel(MoveTemp(InSelectionLabel))
    , DetailsView(InDetailsView)
    , ActorClass(InActorClass)
{
}

void FPruneLayoutContext::UpdateFromFinalCategories(
    const TMap<FName, IDetailCategoryBuilder*>& InCategories)
{
    RawCategories.Reset();
    CategoryGroups.Reset();

    RawCategories.Reserve(InCategories.Num());
    CategoryGroups.Reserve(InCategories.Num());

    TMap<FString, int32> GroupIndexByDisplayLabel;

    for (const TPair<FName, IDetailCategoryBuilder*>& Entry : InCategories)
    {
        const FName CategoryId = Entry.Key;
        IDetailCategoryBuilder* Category = Entry.Value;

        if (CategoryId.IsNone() || Category == nullptr)
        {
            continue;
        }

        const FText DisplayName = Category->GetDisplayName().IsEmpty()
            ? FText::FromName(CategoryId)
            : Category->GetDisplayName();

        FPruneCategoryInfo& RawInfo = RawCategories.AddDefaulted_GetRef();
        RawInfo.Id = CategoryId;
        RawInfo.DisplayName = DisplayName;
        RawInfo.NativeSortOrder = Category->GetSortOrder();

        const FString DisplayKey = DisplayName.ToString();
        int32* ExistingGroupIndex = GroupIndexByDisplayLabel.Find(DisplayKey);

        if (ExistingGroupIndex == nullptr)
        {
            const int32 NewGroupIndex = CategoryGroups.AddDefaulted();
            FPruneCategoryGroupInfo& Group = CategoryGroups[NewGroupIndex];
            Group.DisplayName = DisplayName;
            Group.NativeSortOrder = Category->GetSortOrder();
            Group.Ids.Add(CategoryId);
            GroupIndexByDisplayLabel.Add(DisplayKey, NewGroupIndex);
        }
        else
        {
            FPruneCategoryGroupInfo& Group = CategoryGroups[*ExistingGroupIndex];
            Group.Ids.AddUnique(CategoryId);
            Group.NativeSortOrder = FMath::Min(Group.NativeSortOrder, Category->GetSortOrder());
        }
    }

    RawCategories.Sort(
        [](const FPruneCategoryInfo& A, const FPruneCategoryInfo& B)
        {
            if (A.NativeSortOrder != B.NativeSortOrder)
            {
                return A.NativeSortOrder < B.NativeSortOrder;
            }
            return A.Id.ToString() < B.Id.ToString();
        });

    for (FPruneCategoryGroupInfo& Group : CategoryGroups)
    {
        Group.Ids.Sort(
            [&InCategories](const FName& A, const FName& B)
            {
                IDetailCategoryBuilder* const* ACategory = InCategories.Find(A);
                IDetailCategoryBuilder* const* BCategory = InCategories.Find(B);
                const int32 AOrder = ACategory && *ACategory ? (*ACategory)->GetSortOrder() : 0;
                const int32 BOrder = BCategory && *BCategory ? (*BCategory)->GetSortOrder() : 0;
                if (AOrder != BOrder)
                {
                    return AOrder < BOrder;
                }
                return A.ToString() < B.ToString();
            });
    }

    CategoryGroups.Sort(
        [](const FPruneCategoryGroupInfo& A, const FPruneCategoryGroupInfo& B)
        {
            if (A.NativeSortOrder != B.NativeSortOrder)
            {
                return A.NativeSortOrder < B.NativeSortOrder;
            }
            return A.DisplayName.ToString() < B.DisplayName.ToString();
        });
}

TArray<FPruneCategoryGroupInfo> FPruneLayoutContext::GetCategoryGroups() const
{
    return CategoryGroups;
}

TArray<FName> FPruneLayoutContext::GetCategoryIds() const
{
    TArray<FName> Result;
    Result.Reserve(RawCategories.Num());

    for (const FPruneCategoryInfo& Category : RawCategories)
    {
        Result.Add(Category.Id);
    }

    return Result;
}

FName FPruneLayoutContext::GetActorClassName() const
{
    return ActorClass.IsValid() ? ActorClass->GetFName() : NAME_None;
}

void FPruneLayoutContext::SetSectionSelectorWidget(const TSharedPtr<SWidget>& InWidget)
{
    SectionSelectorWidget = InWidget;
}

TArray<FString> FPruneLayoutContext::GetCheckedSectionLabels() const
{
    TArray<FString> Result;

    const TSharedPtr<SWidget> Selector = SectionSelectorWidget.Pin();
    if (!Selector.IsValid())
    {
        return Result;
    }

    FChildren* Children = Selector->GetChildren();
    if (Children == nullptr)
    {
        return Result;
    }

    for (int32 Index = 0; Index < Children->Num(); ++Index)
    {
        SWidget& Child = Children->GetChildAt(Index).Get();
        SCheckBox* CheckBox = PruneStatePrivate::FindCheckBoxRecursive(Child);
        if (CheckBox == nullptr || !CheckBox->IsChecked())
        {
            continue;
        }

        if (STextBlock* TextBlock = PruneStatePrivate::FindTextBlockRecursive(Child))
        {
            Result.Add(TextBlock->GetText().ToString());
        }
    }

    return Result;
}

void FPruneLayoutContext::LogCurrentCategories() const
{
    UE_LOG(
        LogPrune,
        Log,
        TEXT("Prune current finished category set: %d categories | Selection: %s"),
        RawCategories.Num(),
        *SelectionLabel.ToString());

    for (const FPruneCategoryInfo& Category : RawCategories)
    {
        UE_LOG(
            LogPrune,
            Log,
            TEXT("  %s | Display: %s | Sort: %d"),
            *Category.Id.ToString(),
            *Category.DisplayName.ToString(),
            Category.NativeSortOrder);
    }
}

FPruneState::FPruneState()
{
    LoadPersistentState();

    ActiveOrderTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateRaw(this, &FPruneState::TickActiveCategoryOrders),
        0.15f);
}

FPruneState::~FPruneState()
{
    if (ActiveOrderTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(ActiveOrderTickerHandle);
        ActiveOrderTickerHandle.Reset();
    }

    RemoveAllNativeSections();
}

bool FPruneState::CreatePreset(
    const TSharedRef<FPruneLayoutContext>& Context,
    const FString& PresetName,
    bool bGlobal,
    const TSet<FName>& HiddenCategories,
    const TArray<FName>& OrderedCategoryIds,
    FString& OutError)
{
    OutError.Reset();

    const FString CleanName = PresetName.TrimStartAndEnd();
    if (!ValidateCustomPresetName(Context, CleanName, FString(), OutError))
    {
        return false;
    }

    if (!bGlobal && Context->GetActorClassName().IsNone())
    {
        OutError = TEXT("A class-scoped filter requires a single Actor class selection.");
        return false;
    }

    FPresetData Preset;
    Preset.Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Preset.Name = CleanName;
    Preset.bGlobal = bGlobal;
    Preset.ClassName = bGlobal ? NAME_None : Context->GetActorClassName();
    Preset.HiddenCategories = HiddenCategories;
    Preset.OrderedCategoryIds = OrderedCategoryIds;

    PresetsById.Add(Preset.Id, MoveTemp(Preset));

    SavePersistentState();
    RebuildAllCustomSections();
    RefreshDetailsViews();
    return true;
}

bool FPruneState::UpdatePreset(
    const TSharedRef<FPruneLayoutContext>& Context,
    const FString& PresetId,
    const FString& PresetName,
    bool bGlobal,
    const TSet<FName>& HiddenCategories,
    const TArray<FName>& OrderedCategoryIds,
    FString& OutError)
{
    OutError.Reset();

    FPresetData* Preset = PresetsById.Find(PresetId);
    if (Preset == nullptr)
    {
        OutError = TEXT("The Prune filter no longer exists.");
        return false;
    }

    const FString CleanName = PresetName.TrimStartAndEnd();
    if (!ValidateCustomPresetName(Context, CleanName, PresetId, OutError))
    {
        return false;
    }

    if (!bGlobal && Context->GetActorClassName().IsNone())
    {
        OutError = TEXT("A class-scoped filter requires a single Actor class selection.");
        return false;
    }

    const FName NewScopeClassName = bGlobal
        ? NAME_None
        : (Preset->bGlobal ? Context->GetActorClassName() : Preset->ClassName);

    RemoveCustomSection(*Preset);

    const TArray<FName> CurrentIds = Context->GetCategoryIds();
    TSet<FName> MergedHidden = Preset->HiddenCategories;
    for (const FName Id : CurrentIds)
    {
        MergedHidden.Remove(Id);
        if (HiddenCategories.Contains(Id))
        {
            MergedHidden.Add(Id);
        }
    }

    Preset->Name = CleanName;
    Preset->bGlobal = bGlobal;
    Preset->ClassName = NewScopeClassName;
    Preset->HiddenCategories = MoveTemp(MergedHidden);
    Preset->OrderedCategoryIds = MergeCategoryOrder(
        Preset->OrderedCategoryIds,
        OrderedCategoryIds,
        CurrentIds);

    SavePersistentState();
    RebuildAllCustomSections();
    RefreshDetailsViews();
    return true;
}

bool FPruneState::DeletePreset(const FString& PresetId)
{
    FPresetData* Preset = PresetsById.Find(PresetId);
    if (Preset == nullptr)
    {
        return false;
    }

    RemoveCustomSection(*Preset);
    PresetsById.Remove(PresetId);

    SavePersistentState();
    ResetLiveViewsToAll();
    RefreshDetailsViews();
    return true;
}

bool FPruneState::ResolveSingleActiveFilter(
    const TSharedRef<FPruneLayoutContext>& Context,
    FPruneEditableFilter& OutFilter) const
{
    OutFilter = FPruneEditableFilter();

    const TArray<FString> CheckedLabels = Context->GetCheckedSectionLabels();
    if (CheckedLabels.Num() != 1 || CheckedLabels[0].Equals(TEXT("All"), ESearchCase::IgnoreCase))
    {
        return false;
    }

    const FString& Label = CheckedLabels[0];
    const UClass* ActorClass = Context->GetActorClass();

    const FPresetData* MatchedPreset = nullptr;
    int32 MatchingPresets = 0;
    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        if (Entry.Value.Name == Label
            && DoesScopeApplyToClass(Entry.Value.bGlobal, Entry.Value.ClassName, ActorClass))
        {
            MatchedPreset = &Entry.Value;
            ++MatchingPresets;
        }
    }

    const TArray<FSectionDefinition> NativeSections = CollectNativeSections(Context);
    const FSectionDefinition* MatchedNative = nullptr;
    int32 MatchingNative = 0;
    for (const FSectionDefinition& Section : NativeSections)
    {
        if (Section.DisplayName.ToString() == Label)
        {
            MatchedNative = &Section;
            ++MatchingNative;
        }
    }

    if (MatchingPresets == 1 && MatchingNative == 0 && MatchedPreset != nullptr)
    {
        OutFilter.Kind = EPruneFilterKind::Custom;
        OutFilter.SectionName = MakeSectionName(MatchedPreset->Id);
        OutFilter.DisplayName = FText::FromString(MatchedPreset->Name);
        OutFilter.CustomPresetId = MatchedPreset->Id;
        return true;
    }

    if (MatchingNative == 1 && MatchingPresets == 0 && MatchedNative != nullptr)
    {
        OutFilter.Kind = EPruneFilterKind::Native;
        OutFilter.SectionName = MatchedNative->Name;
        OutFilter.DisplayName = MatchedNative->DisplayName;
        OutFilter.SectionOrder = MatchedNative->Order;

        if (const FNativeOverrideData* Override =
            FindApplicableNativeOverride(MatchedNative->Name, ActorClass))
        {
            OutFilter.NativeOverrideId = Override->Id;
            OutFilter.bHasNativeOverride = true;
        }
        return true;
    }

    return false;
}

bool FPruneState::BuildEditorData(
    const TSharedRef<FPruneLayoutContext>& Context,
    const FPruneEditableFilter& Filter,
    FPruneFilterEditorData& OutData)
{
    OutData = FPruneFilterEditorData();

    if (Filter.Kind == EPruneFilterKind::Custom)
    {
        const FPresetData* Preset = PresetsById.Find(Filter.CustomPresetId);
        if (Preset == nullptr)
        {
            return false;
        }

        OutData.Name = Preset->Name;
        OutData.bGlobal = Preset->bGlobal;
        OutData.ScopeClassName = Preset->ClassName;
        OutData.HiddenCategories = Preset->HiddenCategories;
        OutData.OrderedCategoryIds = Preset->OrderedCategoryIds;
        OutData.bNativeFilter = false;
        return true;
    }

    if (Filter.Kind != EPruneFilterKind::Native || Context->GetActorClass() == nullptr)
    {
        return false;
    }

    OutData.Name = Filter.DisplayName.ToString();
    OutData.bNativeFilter = true;

    const FNativeOverrideData* Override =
        FindApplicableNativeOverride(Filter.SectionName, Context->GetActorClass());

    if (Override != nullptr)
    {
        OutData.bGlobal = Override->bGlobal;
        OutData.ScopeClassName = Override->ClassName;
        OutData.OrderedCategoryIds = Override->OrderedCategoryIds;
        OutData.bHasNativeOverride = true;
    }
    else
    {
        OutData.bGlobal = false;
        OutData.ScopeClassName = Context->GetActorClassName();
    }

    for (const FName CategoryId : Context->GetCategoryIds())
    {
        if (!IsSectionIncludedForCategory(
            Context->GetActorClass(),
            Filter.SectionName,
            CategoryId))
        {
            OutData.HiddenCategories.Add(CategoryId);
        }
    }

    return true;
}

bool FPruneState::SaveNativeOverride(
    const TSharedRef<FPruneLayoutContext>& Context,
    const FPruneEditableFilter& Filter,
    bool bGlobal,
    const TSet<FName>& HiddenCategories,
    const TArray<FName>& OrderedCategoryIds,
    FString& OutError)
{
    OutError.Reset();

    if (Filter.Kind != EPruneFilterKind::Native || Context->GetActorClass() == nullptr)
    {
        OutError = TEXT("The active Epic filter is no longer editable.");
        return false;
    }

    if (!bGlobal && Context->GetActorClassName().IsNone())
    {
        OutError = TEXT("A class-scoped filter override requires a single Actor class selection.");
        return false;
    }

    FNativeOverrideData* Override = Filter.NativeOverrideId.IsEmpty()
        ? nullptr
        : NativeOverridesById.Find(Filter.NativeOverrideId);
    bool bCreatedOverride = false;

    if (Override == nullptr)
    {
        FNativeOverrideData NewOverride;
        NewOverride.Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
        NewOverride.SectionName = Filter.SectionName;
        NewOverride.DisplayName = Filter.DisplayName.ToString();
        NewOverride.SectionOrder = Filter.SectionOrder;
        NewOverride.bGlobal = bGlobal;
        NewOverride.ClassName = bGlobal ? NAME_None : Context->GetActorClassName();
        const FString NewOverrideId = NewOverride.Id;
        NativeOverridesById.Add(NewOverrideId, MoveTemp(NewOverride));
        Override = NativeOverridesById.Find(NewOverrideId);
        bCreatedOverride = true;
    }

    if (Override == nullptr)
    {
        OutError = TEXT("Prune could not create the Epic filter override.");
        return false;
    }

    const bool bWasGlobal = Override->bGlobal;
    const FName NewScopeClassName = bGlobal
        ? NAME_None
        : (Override->bGlobal ? Context->GetActorClassName() : Override->ClassName);
    const bool bScopeChanged = Override->bGlobal != bGlobal
        || (!bGlobal && Override->ClassName != NewScopeClassName);

    bool bScopeConflict = false;
    for (const TPair<FString, FNativeOverrideData>& Entry : NativeOverridesById)
    {
        if (Entry.Key == Override->Id || Entry.Value.SectionName != Filter.SectionName)
        {
            continue;
        }

        const bool bConflicts = bGlobal
            ? Entry.Value.bGlobal
            : (!Entry.Value.bGlobal && Entry.Value.ClassName == NewScopeClassName);
        if (bConflicts)
        {
            bScopeConflict = true;
            break;
        }
    }

    if (bScopeConflict)
    {
        const FString ConflictingOverrideId = Override->Id;
        if (bCreatedOverride)
        {
            NativeOverridesById.Remove(ConflictingOverrideId);
        }
        OutError = TEXT("An override for this Epic filter already exists at the selected scope.");
        return false;
    }

    // Rebase the edited definition against the effective state beneath it.
    // For a global override, restore class-specific overrides of the same
    // section first so the global baseline remains the Epic/native baseline.
    if (bWasGlobal)
    {
        ClearDependentNativeBaselines(Filter.SectionName, Override->Id);
    }
    RestoreNativeOverrideRuntime(*Override);

    if (bScopeChanged)
    {
        Override->bGlobal = bGlobal;
        Override->ClassName = NewScopeClassName;
    }

    CaptureNativeBaseline(*Override, Context);

    const TArray<FName> CurrentCategoryIds = Context->GetCategoryIds();
    for (const FName CategoryId : CurrentCategoryIds)
    {
        const FString BaselineKey = MakeBaselineKey(
            Context->GetActorClassName(),
            CategoryId);
        const bool* BaselineIncluded = Override->BaselineMembership.Find(BaselineKey);
        const bool bBaseline = BaselineIncluded != nullptr
            ? *BaselineIncluded
            : IsSectionIncludedForCategory(
                Context->GetActorClass(),
                Filter.SectionName,
                CategoryId);
        const bool bDesiredIncluded = !HiddenCategories.Contains(CategoryId);

        Override->AddedCategories.Remove(CategoryId);
        Override->RemovedCategories.Remove(CategoryId);

        if (bDesiredIncluded != bBaseline)
        {
            if (bDesiredIncluded)
            {
                Override->AddedCategories.Add(CategoryId);
            }
            else
            {
                Override->RemovedCategories.Add(CategoryId);
            }
        }
    }

    Override->OrderedCategoryIds = MergeCategoryOrder(
        Override->OrderedCategoryIds,
        OrderedCategoryIds,
        CurrentCategoryIds);

    SavePersistentState();
    RefreshDetailsViews();
    return true;
}

bool FPruneState::ResetNativeOverride(const FPruneEditableFilter& Filter)
{
    if (Filter.Kind != EPruneFilterKind::Native || Filter.NativeOverrideId.IsEmpty())
    {
        return false;
    }

    FNativeOverrideData* Override = NativeOverridesById.Find(Filter.NativeOverrideId);
    if (Override == nullptr)
    {
        return false;
    }

    const FName SectionName = Override->SectionName;
    if (Override->bGlobal)
    {
        ClearDependentNativeBaselines(SectionName, Override->Id);
    }
    RestoreNativeOverrideRuntime(*Override);
    NativeOverridesById.Remove(Filter.NativeOverrideId);

    SavePersistentState();
    RefreshDetailsViews();
    return true;
}

bool FPruneState::CanEditActiveFilter(
    const TSharedRef<FPruneLayoutContext>& Context) const
{
    FPruneEditableFilter Filter;
    return ResolveSingleActiveFilter(Context, Filter)
        && Filter.Kind != EPruneFilterKind::None;
}

TArray<FName> FPruneState::GetActiveCategoryOrder(
    const TSharedRef<FPruneLayoutContext>& Context) const
{
    FPruneEditableFilter Filter;
    if (!ResolveSingleActiveFilter(Context, Filter))
    {
        return TArray<FName>();
    }

    if (Filter.Kind == EPruneFilterKind::Custom)
    {
        if (const FPresetData* Preset = PresetsById.Find(Filter.CustomPresetId))
        {
            return Preset->OrderedCategoryIds;
        }
    }
    else if (Filter.Kind == EPruneFilterKind::Native)
    {
        if (const FNativeOverrideData* Override =
            FindApplicableNativeOverride(Filter.SectionName, Context->GetActorClass()))
        {
            return Override->OrderedCategoryIds;
        }
    }

    return TArray<FName>();
}

void FPruneState::RegisterLayoutContext(
    const TSharedRef<FPruneLayoutContext>& Context)
{
    CompactContexts();

    const bool bAlreadyRegistered = LayoutContexts.ContainsByPredicate(
        [&Context](const TWeakPtr<FPruneLayoutContext>& WeakContext)
        {
            return WeakContext.Pin().Get() == &Context.Get();
        });

    if (!bAlreadyRegistered)
    {
        LayoutContexts.Add(Context);
    }
}

TArray<TSharedRef<FPruneLayoutContext>> FPruneState::GetLiveLayoutContexts()
{
    CompactContexts();

    TArray<TSharedRef<FPruneLayoutContext>> Result;
    Result.Reserve(LayoutContexts.Num());

    for (const TWeakPtr<FPruneLayoutContext>& WeakContext : LayoutContexts)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = WeakContext.Pin())
        {
            Result.Add(Context.ToSharedRef());
        }
    }

    return Result;
}

TSharedPtr<FPruneLayoutContext>
FPruneState::FindLatestLayoutContextForDetailsView(
    const TSharedPtr<const IDetailsView>& DetailsView)
{
    if (!DetailsView.IsValid())
    {
        return nullptr;
    }

    CompactContexts();

    for (int32 Index = LayoutContexts.Num() - 1; Index >= 0; --Index)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = LayoutContexts[Index].Pin())
        {
            if (const TSharedPtr<const IDetailsView> ContextDetailsView = Context->GetDetailsView())
            {
                if (ContextDetailsView.Get() == DetailsView.Get())
                {
                    return Context;
                }
            }
        }
    }

    return nullptr;
}

void FPruneState::SyncNativeSectionsForContext(
    const TSharedRef<FPruneLayoutContext>& Context)
{
    bool bDiscoveredNewCategory = false;

    for (const FName CategoryId : Context->GetCategoryIds())
    {
        if (!CategoryId.IsNone() && !KnownCategoryIds.Contains(CategoryId))
        {
            KnownCategoryIds.Add(CategoryId);
            bDiscoveredNewCategory = true;
        }
    }

    if (bDiscoveredNewCategory && !PresetsById.IsEmpty())
    {
        RebuildAllCustomSections();
    }

    ApplyNativeOverridesForContext(Context);
}

void FPruneState::RemoveAllNativeSections()
{
    if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        return;
    }

    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        RemoveCustomSection(Entry.Value);
    }

    for (TPair<FString, FNativeOverrideData>& Entry : NativeOverridesById)
    {
        RestoreNativeOverrideRuntime(Entry.Value);
    }
}

void FPruneState::LogCurrentCategories(
    const TSharedRef<FPruneLayoutContext>& Context) const
{
    Context->LogCurrentCategories();
}

FName FPruneState::MakeSectionName(const FString& PresetId)
{
    return FName(*FString::Printf(TEXT("Prune_%s"), *PresetId));
}

FString FPruneState::MakeBaselineKey(FName ClassName, FName CategoryId)
{
    return ClassName.ToString() + TEXT("|") + CategoryId.ToString();
}

void FPruneState::CompactContexts()
{
    LayoutContexts.RemoveAll(
        [](const TWeakPtr<FPruneLayoutContext>& WeakContext)
        {
            return !WeakContext.IsValid();
        });
}

void FPruneState::CompactObservedViews()
{
    ObservedViewOrders.RemoveAll(
        [](const FObservedViewOrderState& Entry)
        {
            return !Entry.DetailsView.IsValid();
        });
}

bool FPruneState::DoesScopeApplyToClass(
    bool bGlobal,
    FName ScopeClassName,
    const UClass* ActorClass) const
{
    if (bGlobal)
    {
        return true;
    }

    if (ScopeClassName.IsNone() || ActorClass == nullptr)
    {
        return false;
    }

    for (const UClass* ClassIt = ActorClass; ClassIt != nullptr; ClassIt = ClassIt->GetSuperClass())
    {
        if (ClassIt->GetFName() == ScopeClassName)
        {
            return true;
        }
    }

    return false;
}

bool FPruneState::IsCustomSectionName(FName SectionName, FString* OutPresetId) const
{
    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        if (MakeSectionName(Entry.Key) == SectionName)
        {
            if (OutPresetId != nullptr)
            {
                *OutPresetId = Entry.Key;
            }
            return true;
        }
    }

    return false;
}

const FPruneState::FPresetData* FPruneState::FindPresetBySectionName(FName SectionName) const
{
    FString PresetId;
    return IsCustomSectionName(SectionName, &PresetId)
        ? PresetsById.Find(PresetId)
        : nullptr;
}

FPruneState::FPresetData* FPruneState::FindPresetBySectionName(FName SectionName)
{
    FString PresetId;
    return IsCustomSectionName(SectionName, &PresetId)
        ? PresetsById.Find(PresetId)
        : nullptr;
}

TArray<FPruneState::FSectionDefinition> FPruneState::CollectNativeSections(
    const TSharedRef<FPruneLayoutContext>& Context) const
{
    TArray<FSectionDefinition> Result;
    const UClass* ActorClass = Context->GetActorClass();
    if (ActorClass == nullptr || !FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        return Result;
    }

    const FPropertyEditorModule& PropertyEditor =
        FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    TMap<FName, FSectionDefinition> SectionsByName;
    for (const FName CategoryId : Context->GetCategoryIds())
    {
        const TArray<TSharedPtr<FPropertySection>> Sections =
            PropertyEditor.FindSectionsForCategory(ActorClass, CategoryId);

        for (const TSharedPtr<FPropertySection>& Section : Sections)
        {
            if (!Section.IsValid() || IsCustomSectionName(Section->GetName()))
            {
                continue;
            }

            FSectionDefinition Definition;
            Definition.Name = Section->GetName();
            Definition.DisplayName = Section->GetDisplayName();
            Definition.Order = Section->GetOrder();
            SectionsByName.Add(Definition.Name, Definition);
        }
    }

    SectionsByName.GenerateValueArray(Result);
    return Result;
}

const FPruneState::FNativeOverrideData* FPruneState::FindApplicableNativeOverride(
    FName SectionName,
    const UClass* ActorClass) const
{
    const FNativeOverrideData* BestClassMatch = nullptr;
    int32 BestDistance = MAX_int32;
    const FNativeOverrideData* GlobalMatch = nullptr;

    for (const TPair<FString, FNativeOverrideData>& Entry : NativeOverridesById)
    {
        const FNativeOverrideData& Override = Entry.Value;
        if (Override.SectionName != SectionName)
        {
            continue;
        }

        if (Override.bGlobal)
        {
            GlobalMatch = &Override;
            continue;
        }

        int32 Distance = 0;
        for (const UClass* ClassIt = ActorClass; ClassIt != nullptr; ClassIt = ClassIt->GetSuperClass(), ++Distance)
        {
            if (ClassIt->GetFName() == Override.ClassName)
            {
                if (Distance < BestDistance)
                {
                    BestDistance = Distance;
                    BestClassMatch = &Override;
                }
                break;
            }
        }
    }

    return BestClassMatch != nullptr ? BestClassMatch : GlobalMatch;
}

FPruneState::FNativeOverrideData* FPruneState::FindApplicableNativeOverride(
    FName SectionName,
    const UClass* ActorClass)
{
    FNativeOverrideData* BestClassMatch = nullptr;
    int32 BestDistance = MAX_int32;
    FNativeOverrideData* GlobalMatch = nullptr;

    for (TPair<FString, FNativeOverrideData>& Entry : NativeOverridesById)
    {
        FNativeOverrideData& Override = Entry.Value;
        if (Override.SectionName != SectionName)
        {
            continue;
        }

        if (Override.bGlobal)
        {
            GlobalMatch = &Override;
            continue;
        }

        int32 Distance = 0;
        for (const UClass* ClassIt = ActorClass; ClassIt != nullptr; ClassIt = ClassIt->GetSuperClass(), ++Distance)
        {
            if (ClassIt->GetFName() == Override.ClassName)
            {
                if (Distance < BestDistance)
                {
                    BestDistance = Distance;
                    BestClassMatch = &Override;
                }
                break;
            }
        }
    }

    return BestClassMatch != nullptr ? BestClassMatch : GlobalMatch;
}

bool FPruneState::IsSectionIncludedForCategory(
    const UClass* ActorClass,
    FName SectionName,
    FName CategoryId) const
{
    if (ActorClass == nullptr || SectionName.IsNone())
    {
        return false;
    }

    const FPropertyEditorModule& PropertyEditor =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    const TArray<TSharedPtr<FPropertySection>> Sections =
        PropertyEditor.FindSectionsForCategory(ActorClass, CategoryId);

    return Sections.ContainsByPredicate(
        [SectionName](const TSharedPtr<FPropertySection>& Section)
        {
            return Section.IsValid() && Section->GetName() == SectionName;
        });
}

bool FPruneState::ValidateCustomPresetName(
    const TSharedRef<FPruneLayoutContext>& Context,
    const FString& CleanName,
    const FString& IgnorePresetId,
    FString& OutError) const
{
    if (CleanName.IsEmpty())
    {
        OutError = TEXT("Filter name cannot be empty.");
        return false;
    }

    if (CleanName.Equals(TEXT("All"), ESearchCase::IgnoreCase))
    {
        OutError = TEXT("'All' is reserved by Unreal Engine.");
        return false;
    }

    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        if (Entry.Key != IgnorePresetId
            && Entry.Value.Name.Equals(CleanName, ESearchCase::IgnoreCase))
        {
            OutError = FString::Printf(
                TEXT("A Prune filter named '%s' already exists."),
                *CleanName);
            return false;
        }
    }

    for (const FSectionDefinition& Section : CollectNativeSections(Context))
    {
        if (Section.DisplayName.ToString().Equals(CleanName, ESearchCase::IgnoreCase))
        {
            OutError = FString::Printf(
                TEXT("'%s' is already used by an Unreal Engine filter."),
                *CleanName);
            return false;
        }
    }

    return true;
}

void FPruneState::RebuildAllCustomSections()
{
    if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        return;
    }

    TArray<FString> OrderedPresetIds;
    PresetsById.GenerateKeyArray(OrderedPresetIds);
    OrderedPresetIds.Sort(
        [this](const FString& A, const FString& B)
        {
            const FPresetData* PresetA = PresetsById.Find(A);
            const FPresetData* PresetB = PresetsById.Find(B);
            if (PresetA == nullptr || PresetB == nullptr)
            {
                return A < B;
            }
            if (PresetA->Name != PresetB->Name)
            {
                return PresetA->Name < PresetB->Name;
            }
            return A < B;
        });

    int32 Order = PruneStatePrivate::FirstPruneSectionOrder;
    for (const FString& PresetId : OrderedPresetIds)
    {
        if (const FPresetData* Preset = PresetsById.Find(PresetId))
        {
            RebuildCustomSection(*Preset, Order++);
        }
    }
}

void FPruneState::RebuildCustomSection(const FPresetData& Preset, int32 Order)
{
    FPropertyEditorModule& PropertyEditor =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    const FName TargetClassName = Preset.bGlobal
        ? AActor::StaticClass()->GetFName()
        : Preset.ClassName;

    if (TargetClassName.IsNone())
    {
        return;
    }

    const FName SectionName = MakeSectionName(Preset.Id);
    PropertyEditor.RemoveSection(TargetClassName, SectionName);

    TSharedRef<FPropertySection> Section = PropertyEditor.FindOrCreateSection(
        TargetClassName,
        SectionName,
        FText::FromString(Preset.Name),
        Order);

    TArray<FName> SortedCategoryIds = KnownCategoryIds.Array();
    SortedCategoryIds.Sort(
        [](const FName& A, const FName& B)
        {
            return A.ToString() < B.ToString();
        });

    for (const FName CategoryId : SortedCategoryIds)
    {
        if (!Preset.HiddenCategories.Contains(CategoryId))
        {
            Section->AddCategory(CategoryId);
        }
    }
}

void FPruneState::RemoveCustomSection(const FPresetData& Preset)
{
    if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        return;
    }

    const FName TargetClassName = Preset.bGlobal
        ? AActor::StaticClass()->GetFName()
        : Preset.ClassName;

    if (TargetClassName.IsNone())
    {
        return;
    }

    FPropertyEditorModule& PropertyEditor =
        FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    PropertyEditor.RemoveSection(TargetClassName, MakeSectionName(Preset.Id));
}

void FPruneState::ApplyNativeOverridesForContext(
    const TSharedRef<FPruneLayoutContext>& Context)
{
    const UClass* ActorClass = Context->GetActorClass();
    if (ActorClass == nullptr)
    {
        return;
    }

    TArray<FString> GlobalIds;
    struct FClassOverrideOrder
    {
        FString Id;
        int32 Distance = 0;
    };
    TArray<FClassOverrideOrder> ClassOverrides;

    for (const TPair<FString, FNativeOverrideData>& Entry : NativeOverridesById)
    {
        if (Entry.Value.bGlobal)
        {
            GlobalIds.Add(Entry.Key);
            continue;
        }

        int32 Distance = 0;
        for (const UClass* ClassIt = ActorClass; ClassIt != nullptr; ClassIt = ClassIt->GetSuperClass(), ++Distance)
        {
            if (ClassIt->GetFName() == Entry.Value.ClassName)
            {
                FClassOverrideOrder& Ordered = ClassOverrides.AddDefaulted_GetRef();
                Ordered.Id = Entry.Key;
                Ordered.Distance = Distance;
                break;
            }
        }
    }

    GlobalIds.Sort();
    ClassOverrides.Sort([](const FClassOverrideOrder& A, const FClassOverrideOrder& B)
    {
        if (A.Distance != B.Distance)
        {
            return A.Distance > B.Distance; // base class first, exact class last
        }
        return A.Id < B.Id;
    });

    for (const FString& Id : GlobalIds)
    {
        if (FNativeOverrideData* Override = NativeOverridesById.Find(Id))
        {
            ApplyNativeOverrideForContext(*Override, Context);
        }
    }

    for (const FClassOverrideOrder& Ordered : ClassOverrides)
    {
        if (FNativeOverrideData* Override = NativeOverridesById.Find(Ordered.Id))
        {
            ApplyNativeOverrideForContext(*Override, Context);
        }
    }
}

void FPruneState::ApplyNativeOverrideForContext(
    FNativeOverrideData& Override,
    const TSharedRef<FPruneLayoutContext>& Context)
{
    if (Context->GetActorClass() == nullptr)
    {
        return;
    }

    CaptureNativeBaseline(Override, Context);

    FPropertyEditorModule& PropertyEditor =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    const FName CurrentClassName = Context->GetActorClassName();
    TSharedRef<FPropertySection> Section = PropertyEditor.FindOrCreateSection(
        CurrentClassName,
        Override.SectionName,
        FText::FromString(Override.DisplayName),
        Override.SectionOrder);

    for (const FName CategoryId : Context->GetCategoryIds())
    {
        if (Override.AddedCategories.Contains(CategoryId))
        {
            Section->AddCategory(CategoryId);
        }
        else if (Override.RemovedCategories.Contains(CategoryId))
        {
            Section->RemoveCategory(CategoryId);
        }
    }

    Override.TouchedClasses.Add(CurrentClassName);
}

void FPruneState::CaptureNativeBaseline(
    FNativeOverrideData& Override,
    const TSharedRef<FPruneLayoutContext>& Context)
{
    const UClass* ActorClass = Context->GetActorClass();
    const FName ClassName = Context->GetActorClassName();
    if (ActorClass == nullptr || ClassName.IsNone())
    {
        return;
    }

    for (const FName CategoryId : Context->GetCategoryIds())
    {
        const FString Key = MakeBaselineKey(ClassName, CategoryId);
        if (!Override.BaselineMembership.Contains(Key))
        {
            Override.BaselineMembership.Add(
                Key,
                IsSectionIncludedForCategory(
                    ActorClass,
                    Override.SectionName,
                    CategoryId));
        }
    }
}

void FPruneState::RestoreNativeOverrideRuntime(FNativeOverrideData& Override)
{
    if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        Override.BaselineMembership.Reset();
        Override.TouchedClasses.Reset();
        return;
    }

    FPropertyEditorModule& PropertyEditor =
        FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    for (const FName ClassName : Override.TouchedClasses)
    {
        if (ClassName.IsNone())
        {
            continue;
        }

        TSharedRef<FPropertySection> Section = PropertyEditor.FindOrCreateSection(
            ClassName,
            Override.SectionName,
            FText::FromString(Override.DisplayName),
            Override.SectionOrder);

        const FString Prefix = ClassName.ToString() + TEXT("|");
        for (const TPair<FString, bool>& Entry : Override.BaselineMembership)
        {
            if (!Entry.Key.StartsWith(Prefix))
            {
                continue;
            }

            const FString CategoryString = Entry.Key.RightChop(Prefix.Len());
            const FName CategoryId(*CategoryString);
            if (Entry.Value)
            {
                Section->AddCategory(CategoryId);
            }
            else
            {
                Section->RemoveCategory(CategoryId);
            }
        }
    }

    Override.BaselineMembership.Reset();
    Override.TouchedClasses.Reset();
}

void FPruneState::ClearDependentNativeBaselines(
    FName SectionName,
    const FString& IgnoreOverrideId)
{
    for (TPair<FString, FNativeOverrideData>& Entry : NativeOverridesById)
    {
        if (Entry.Key != IgnoreOverrideId && Entry.Value.SectionName == SectionName)
        {
            RestoreNativeOverrideRuntime(Entry.Value);
        }
    }
}

TArray<FName> FPruneState::MergeCategoryOrder(
    const TArray<FName>& ExistingOrder,
    const TArray<FName>& CurrentOrder,
    const TArray<FName>& CurrentCategoryIds)
{
    if (ExistingOrder.IsEmpty())
    {
        return CurrentOrder;
    }

    TSet<FName> CurrentSet;
    for (const FName Id : CurrentCategoryIds)
    {
        CurrentSet.Add(Id);
    }

    TArray<FName> Result;
    Result.Reserve(ExistingOrder.Num() + CurrentOrder.Num());

    int32 InsertIndex = INDEX_NONE;
    for (const FName Id : ExistingOrder)
    {
        if (CurrentSet.Contains(Id))
        {
            if (InsertIndex == INDEX_NONE)
            {
                InsertIndex = Result.Num();
            }
            continue;
        }
        Result.AddUnique(Id);
    }

    if (InsertIndex == INDEX_NONE)
    {
        InsertIndex = Result.Num();
    }

    int32 Offset = 0;
    for (const FName Id : CurrentOrder)
    {
        if (!Id.IsNone() && !Result.Contains(Id))
        {
            Result.Insert(Id, InsertIndex + Offset);
            ++Offset;
        }
    }

    return Result;
}

bool FPruneState::TickActiveCategoryOrders(float)
{
    CompactContexts();
    CompactObservedViews();

    TSet<const IDetailsView*> ProcessedViews;

    for (int32 ContextIndex = LayoutContexts.Num() - 1; ContextIndex >= 0; --ContextIndex)
    {
        const TSharedPtr<FPruneLayoutContext> Context = LayoutContexts[ContextIndex].Pin();
        if (!Context.IsValid())
        {
            continue;
        }

        const TSharedPtr<const IDetailsView> DetailsView = Context->GetDetailsView();
        if (!DetailsView.IsValid() || ProcessedViews.Contains(DetailsView.Get()))
        {
            continue;
        }
        ProcessedViews.Add(DetailsView.Get());

        bool bHasCustomOrder = false;
        const FString Signature = BuildActiveOrderSignature(Context.ToSharedRef(), bHasCustomOrder);

        FObservedViewOrderState* Observed = ObservedViewOrders.FindByPredicate(
            [&DetailsView](const FObservedViewOrderState& Entry)
            {
                const TSharedPtr<const IDetailsView> Existing = Entry.DetailsView.Pin();
                return Existing.IsValid() && Existing.Get() == DetailsView.Get();
            });

        if (Observed == nullptr)
        {
            FObservedViewOrderState& NewObserved = ObservedViewOrders.AddDefaulted_GetRef();
            NewObserved.DetailsView = DetailsView;
            NewObserved.Signature = Signature;
            NewObserved.bHadCustomOrder = bHasCustomOrder;

            if (bHasCustomOrder)
            {
                const_cast<IDetailsView*>(DetailsView.Get())->RequestForceRefresh();
            }
            continue;
        }

        if (Observed->Signature != Signature)
        {
            const bool bNeedsRefresh = Observed->bHadCustomOrder || bHasCustomOrder;
            Observed->Signature = Signature;
            Observed->bHadCustomOrder = bHasCustomOrder;

            if (bNeedsRefresh)
            {
                const_cast<IDetailsView*>(DetailsView.Get())->RequestForceRefresh();
            }
        }
    }

    return true;
}

FString FPruneState::BuildActiveOrderSignature(
    const TSharedRef<FPruneLayoutContext>& Context,
    bool& bOutHasCustomOrder) const
{
    bOutHasCustomOrder = false;

    FPruneEditableFilter Filter;
    if (!ResolveSingleActiveFilter(Context, Filter))
    {
        return Context->GetActorClassName().ToString() + TEXT("|NoEditableSingleFilter");
    }

    TArray<FName> Order;
    if (Filter.Kind == EPruneFilterKind::Custom)
    {
        if (const FPresetData* Preset = PresetsById.Find(Filter.CustomPresetId))
        {
            Order = Preset->OrderedCategoryIds;
        }
    }
    else if (Filter.Kind == EPruneFilterKind::Native)
    {
        if (const FNativeOverrideData* Override =
            FindApplicableNativeOverride(Filter.SectionName, Context->GetActorClass()))
        {
            Order = Override->OrderedCategoryIds;
        }
    }

    TSet<FName> CurrentIds;
    for (const FName Id : Context->GetCategoryIds())
    {
        CurrentIds.Add(Id);
    }

    FString Signature = Context->GetActorClassName().ToString();
    Signature += TEXT("|");
    Signature += Filter.SectionName.ToString();
    Signature += TEXT("|");

    for (const FName Id : Order)
    {
        if (CurrentIds.Contains(Id))
        {
            bOutHasCustomOrder = true;
            Signature += Id.ToString();
            Signature += TEXT(";");
        }
    }

    return Signature;
}

void FPruneState::ResetLiveViewsToAll()
{
    CompactContexts();

    for (const TWeakPtr<FPruneLayoutContext>& WeakContext : LayoutContexts)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = WeakContext.Pin())
        {
            if (const TSharedPtr<const IDetailsView> ConstDetailsView = Context->GetDetailsView())
            {
                const_cast<IDetailsView*>(ConstDetailsView.Get())->ResetToDefaultSection();
            }
        }
    }
}

void FPruneState::RefreshDetailsViews()
{
    if (FPropertyEditorModule* PropertyEditor =
        FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
    {
        PropertyEditor->NotifyCustomizationModuleChanged();
    }
}

void FPruneState::LoadPersistentState()
{
    if (GConfig == nullptr)
    {
        return;
    }

    TArray<FString> PresetIds;
    GConfig->GetArray(
        PruneStatePrivate::RootSection,
        PruneStatePrivate::PresetIdsKey,
        PresetIds,
        GEditorPerProjectIni);

    for (const FString& PresetIdValue : PresetIds)
    {
        const FString PresetId = PresetIdValue.TrimStartAndEnd();
        if (PresetId.IsEmpty())
        {
            continue;
        }

        const FString SectionName = PruneStatePrivate::MakePresetSection(PresetId);

        FString Name;
        GConfig->GetString(*SectionName, PruneStatePrivate::NameKey, Name, GEditorPerProjectIni);
        Name = Name.TrimStartAndEnd();
        if (Name.IsEmpty())
        {
            continue;
        }

        TArray<FString> HiddenCategoryStrings;
        GConfig->GetArray(
            *SectionName,
            PruneStatePrivate::HiddenCategoriesKey,
            HiddenCategoryStrings,
            GEditorPerProjectIni);

        TArray<FString> OrderStrings;
        GConfig->GetArray(
            *SectionName,
            PruneStatePrivate::CategoryOrderKey,
            OrderStrings,
            GEditorPerProjectIni);

        bool bGlobal = true; // 0.4.x presets were all global.
        GConfig->GetBool(*SectionName, PruneStatePrivate::GlobalKey, bGlobal, GEditorPerProjectIni);

        FString ClassNameString;
        GConfig->GetString(
            *SectionName,
            PruneStatePrivate::ClassNameKey,
            ClassNameString,
            GEditorPerProjectIni);

        FPresetData Preset;
        Preset.Id = PresetId;
        Preset.Name = Name;
        Preset.bGlobal = bGlobal;
        Preset.ClassName = bGlobal || ClassNameString.IsEmpty()
            ? NAME_None
            : FName(*ClassNameString);
        Preset.HiddenCategories = PruneStatePrivate::StringsToNames(HiddenCategoryStrings);
        Preset.OrderedCategoryIds = PruneStatePrivate::StringsToOrderedNames(OrderStrings);

        PresetsById.Add(Preset.Id, MoveTemp(Preset));
    }

    TArray<FString> NativeOverrideIds;
    GConfig->GetArray(
        PruneStatePrivate::RootSection,
        PruneStatePrivate::NativeOverrideIdsKey,
        NativeOverrideIds,
        GEditorPerProjectIni);

    for (const FString& OverrideIdValue : NativeOverrideIds)
    {
        const FString OverrideId = OverrideIdValue.TrimStartAndEnd();
        if (OverrideId.IsEmpty())
        {
            continue;
        }

        const FString ConfigSection =
            PruneStatePrivate::MakeNativeOverrideSection(OverrideId);

        FString SectionNameString;
        FString DisplayName;
        FString ClassNameString;
        int32 SectionOrder = 0;
        bool bGlobal = false;

        GConfig->GetString(*ConfigSection, PruneStatePrivate::SectionNameKey, SectionNameString, GEditorPerProjectIni);
        GConfig->GetString(*ConfigSection, PruneStatePrivate::DisplayNameKey, DisplayName, GEditorPerProjectIni);
        GConfig->GetString(*ConfigSection, PruneStatePrivate::ClassNameKey, ClassNameString, GEditorPerProjectIni);
        GConfig->GetInt(*ConfigSection, PruneStatePrivate::SectionOrderKey, SectionOrder, GEditorPerProjectIni);
        GConfig->GetBool(*ConfigSection, PruneStatePrivate::GlobalKey, bGlobal, GEditorPerProjectIni);

        if (SectionNameString.IsEmpty())
        {
            continue;
        }

        TArray<FString> AddedStrings;
        TArray<FString> RemovedStrings;
        TArray<FString> OrderStrings;
        GConfig->GetArray(*ConfigSection, PruneStatePrivate::AddedCategoriesKey, AddedStrings, GEditorPerProjectIni);
        GConfig->GetArray(*ConfigSection, PruneStatePrivate::RemovedCategoriesKey, RemovedStrings, GEditorPerProjectIni);
        GConfig->GetArray(*ConfigSection, PruneStatePrivate::CategoryOrderKey, OrderStrings, GEditorPerProjectIni);

        FNativeOverrideData Override;
        Override.Id = OverrideId;
        Override.SectionName = FName(*SectionNameString);
        Override.DisplayName = DisplayName.IsEmpty() ? SectionNameString : DisplayName;
        Override.SectionOrder = SectionOrder;
        Override.bGlobal = bGlobal;
        Override.ClassName = bGlobal || ClassNameString.IsEmpty()
            ? NAME_None
            : FName(*ClassNameString);
        Override.AddedCategories = PruneStatePrivate::StringsToNames(AddedStrings);
        Override.RemovedCategories = PruneStatePrivate::StringsToNames(RemovedStrings);
        Override.OrderedCategoryIds = PruneStatePrivate::StringsToOrderedNames(OrderStrings);

        NativeOverridesById.Add(Override.Id, MoveTemp(Override));
    }
}

void FPruneState::SavePersistentState() const
{
    if (GConfig == nullptr)
    {
        return;
    }

    TArray<FString> PresetIds;
    PresetsById.GenerateKeyArray(PresetIds);
    PresetIds.Sort();

    GConfig->SetArray(
        PruneStatePrivate::RootSection,
        PruneStatePrivate::PresetIdsKey,
        PresetIds,
        GEditorPerProjectIni);

    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        const FString SectionName = PruneStatePrivate::MakePresetSection(Entry.Key);

        GConfig->SetString(*SectionName, PruneStatePrivate::NameKey, *Entry.Value.Name, GEditorPerProjectIni);
        GConfig->SetBool(*SectionName, PruneStatePrivate::GlobalKey, Entry.Value.bGlobal, GEditorPerProjectIni);
        GConfig->SetString(
            *SectionName,
            PruneStatePrivate::ClassNameKey,
            Entry.Value.bGlobal ? TEXT("") : *Entry.Value.ClassName.ToString(),
            GEditorPerProjectIni);

        const TArray<FString> HiddenStrings =
            PruneStatePrivate::NamesToStrings(Entry.Value.HiddenCategories);
        const TArray<FString> OrderStrings =
            PruneStatePrivate::OrderedNamesToStrings(Entry.Value.OrderedCategoryIds);

        GConfig->SetArray(*SectionName, PruneStatePrivate::HiddenCategoriesKey, HiddenStrings, GEditorPerProjectIni);
        GConfig->SetArray(*SectionName, PruneStatePrivate::CategoryOrderKey, OrderStrings, GEditorPerProjectIni);
    }

    TArray<FString> NativeOverrideIds;
    NativeOverridesById.GenerateKeyArray(NativeOverrideIds);
    NativeOverrideIds.Sort();

    GConfig->SetArray(
        PruneStatePrivate::RootSection,
        PruneStatePrivate::NativeOverrideIdsKey,
        NativeOverrideIds,
        GEditorPerProjectIni);

    for (const TPair<FString, FNativeOverrideData>& Entry : NativeOverridesById)
    {
        const FString ConfigSection =
            PruneStatePrivate::MakeNativeOverrideSection(Entry.Key);
        const FNativeOverrideData& Override = Entry.Value;

        GConfig->SetString(*ConfigSection, PruneStatePrivate::SectionNameKey, *Override.SectionName.ToString(), GEditorPerProjectIni);
        GConfig->SetString(*ConfigSection, PruneStatePrivate::DisplayNameKey, *Override.DisplayName, GEditorPerProjectIni);
        GConfig->SetInt(*ConfigSection, PruneStatePrivate::SectionOrderKey, Override.SectionOrder, GEditorPerProjectIni);
        GConfig->SetBool(*ConfigSection, PruneStatePrivate::GlobalKey, Override.bGlobal, GEditorPerProjectIni);
        GConfig->SetString(
            *ConfigSection,
            PruneStatePrivate::ClassNameKey,
            Override.bGlobal ? TEXT("") : *Override.ClassName.ToString(),
            GEditorPerProjectIni);

        GConfig->SetArray(
            *ConfigSection,
            PruneStatePrivate::AddedCategoriesKey,
            PruneStatePrivate::NamesToStrings(Override.AddedCategories),
            GEditorPerProjectIni);
        GConfig->SetArray(
            *ConfigSection,
            PruneStatePrivate::RemovedCategoriesKey,
            PruneStatePrivate::NamesToStrings(Override.RemovedCategories),
            GEditorPerProjectIni);
        GConfig->SetArray(
            *ConfigSection,
            PruneStatePrivate::CategoryOrderKey,
            PruneStatePrivate::OrderedNamesToStrings(Override.OrderedCategoryIds),
            GEditorPerProjectIni);
    }

    GConfig->Flush(false, GEditorPerProjectIni);
}
