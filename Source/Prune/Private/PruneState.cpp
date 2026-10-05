// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneState.h"

#include "DetailCategoryBuilder.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Guid.h"

DEFINE_LOG_CATEGORY_STATIC(LogPrune, Log, All);

namespace PruneStatePrivate
{
    static const TCHAR* RootSection = TEXT("Prune.UserState");
    static const TCHAR* PresetIdsKey = TEXT("PresetIds");
    static const TCHAR* SelectionKeysKey = TEXT("SelectionKeys");
    static const TCHAR* HiddenCategoriesKey = TEXT("HiddenCategoryIds");
    static const TCHAR* ActivePresetIdKey = TEXT("ActivePresetId");
    static const TCHAR* NameKey = TEXT("Name");

    static FString MakePresetSection(const FString& PresetId)
    {
        return FString::Printf(TEXT("Prune.Preset.%s"), *PresetId);
    }

    static FString MakeSelectionSection(int32 Index)
    {
        return FString::Printf(TEXT("Prune.Selection.%d"), Index);
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
}

FPruneLayoutContext::FPruneLayoutContext(
    FString InSelectionKey,
    FText InSelectionLabel)
    : SelectionKey(MoveTemp(InSelectionKey))
    , SelectionLabel(MoveTemp(InSelectionLabel))
{
}

void FPruneLayoutContext::UpdateFromFinalCategories(
    const TMap<FName, IDetailCategoryBuilder*>& InCategories,
    FName ExcludedCategoryId)
{
    RawCategories.Reset();
    CategoryGroups.Reset();
    CategoryBuilders.Reset();

    RawCategories.Reserve(InCategories.Num());
    CategoryGroups.Reserve(InCategories.Num());
    CategoryBuilders.Reserve(InCategories.Num());

    TMap<FString, int32> GroupIndexByDisplayLabel;

    for (const TPair<FName, IDetailCategoryBuilder*>& Entry : InCategories)
    {
        const FName CategoryId = Entry.Key;
        IDetailCategoryBuilder* Category = Entry.Value;

        if (CategoryId.IsNone()
            || CategoryId == ExcludedCategoryId
            || Category == nullptr)
        {
            continue;
        }

        const FText DisplayName = Category->GetDisplayName().IsEmpty()
            ? FText::FromName(CategoryId)
            : Category->GetDisplayName();

        FPruneCategoryInfo& RawInfo = RawCategories.AddDefaulted_GetRef();
        RawInfo.Id = CategoryId;
        RawInfo.DisplayName = DisplayName;

        CategoryBuilders.Add(CategoryId, Category);

        const FString DisplayKey = DisplayName.ToString();
        int32* ExistingGroupIndex = GroupIndexByDisplayLabel.Find(DisplayKey);

        if (ExistingGroupIndex == nullptr)
        {
            const int32 NewGroupIndex = CategoryGroups.AddDefaulted();
            FPruneCategoryGroupInfo& Group = CategoryGroups[NewGroupIndex];
            Group.DisplayName = DisplayName;
            Group.Ids.Add(CategoryId);
            GroupIndexByDisplayLabel.Add(DisplayKey, NewGroupIndex);
        }
        else
        {
            CategoryGroups[*ExistingGroupIndex].Ids.AddUnique(CategoryId);
        }
    }

    // The menu and diagnostic log are alphabetical on purpose. This is
    // independent of the Details panel's visual category order.
    RawCategories.Sort(
        [](const FPruneCategoryInfo& A, const FPruneCategoryInfo& B)
        {
            const FString ADisplay = A.DisplayName.ToString();
            const FString BDisplay = B.DisplayName.ToString();

            if (ADisplay != BDisplay)
            {
                return ADisplay < BDisplay;
            }

            return A.Id.ToString() < B.Id.ToString();
        });

    for (FPruneCategoryGroupInfo& Group : CategoryGroups)
    {
        Group.Ids.Sort(
            [](const FName& A, const FName& B)
            {
                return A.ToString() < B.ToString();
            });
    }

    CategoryGroups.Sort(
        [](const FPruneCategoryGroupInfo& A, const FPruneCategoryGroupInfo& B)
        {
            return A.DisplayName.ToString() < B.DisplayName.ToString();
        });
}

TArray<FPruneCategoryGroupInfo> FPruneLayoutContext::GetCategoryGroups() const
{
    return CategoryGroups;
}

int32 FPruneLayoutContext::GetHiddenCurrentCategoryCount(
    const FPruneState& State) const
{
    int32 Count = 0;

    for (const FPruneCategoryGroupInfo& Group : CategoryGroups)
    {
        if (!State.AreAllCategoriesVisible(SelectionKey, Group.Ids))
        {
            ++Count;
        }
    }

    return Count;
}

void FPruneLayoutContext::ApplyCategoryVisibility(
    FName CategoryId,
    bool bVisible)
{
    if (IDetailCategoryBuilder** Category = CategoryBuilders.Find(CategoryId))
    {
        if (*Category != nullptr)
        {
            (*Category)->SetCategoryVisibility(bVisible);
        }
    }
}

void FPruneLayoutContext::ApplyCategoryGroupVisibility(
    const TArray<FName>& CategoryIds,
    bool bVisible)
{
    for (const FName CategoryId : CategoryIds)
    {
        ApplyCategoryVisibility(CategoryId, bVisible);
    }
}

void FPruneLayoutContext::ApplyHiddenState(const FPruneState& State)
{
    for (const TPair<FName, IDetailCategoryBuilder*>& Entry : CategoryBuilders)
    {
        if (Entry.Value != nullptr)
        {
            Entry.Value->SetCategoryVisibility(
                State.IsCategoryVisible(SelectionKey, Entry.Key));
        }
    }
}

void FPruneLayoutContext::ShowAllCurrentCategories()
{
    for (const TPair<FName, IDetailCategoryBuilder*>& Entry : CategoryBuilders)
    {
        if (Entry.Value != nullptr)
        {
            Entry.Value->SetCategoryVisibility(true);
        }
    }
}

void FPruneLayoutContext::LogCurrentCategories(
    const FPruneState& State) const
{
    UE_LOG(
        LogPrune,
        Log,
        TEXT("Prune current finished category set: %d categories | Selection: %s | Key: %s | Mode: %s"),
        RawCategories.Num(),
        *SelectionLabel.ToString(),
        *SelectionKey,
        *State.GetSelectionModeLabel(SelectionKey).ToString());

    for (const FPruneCategoryInfo& Category : RawCategories)
    {
        UE_LOG(
            LogPrune,
            Log,
            TEXT("  %s | Display: %s | Visible: %s"),
            *Category.Id.ToString(),
            *Category.DisplayName.ToString(),
            State.IsCategoryVisible(SelectionKey, Category.Id)
                ? TEXT("Yes")
                : TEXT("No"));
    }
}

FPruneState::FPruneState()
{
    LoadPersistentState();
}

bool FPruneState::IsCategoryVisible(
    const FString& SelectionKey,
    FName CategoryId) const
{
    if (const FSelectionState* Selection = SelectionStates.Find(SelectionKey))
    {
        return !Selection->HiddenCategories.Contains(CategoryId);
    }

    return true;
}

bool FPruneState::AreAllCategoriesVisible(
    const FString& SelectionKey,
    const TArray<FName>& CategoryIds) const
{
    for (const FName CategoryId : CategoryIds)
    {
        if (!IsCategoryVisible(SelectionKey, CategoryId))
        {
            return false;
        }
    }

    return true;
}

bool FPruneState::AreAnyCategoriesVisible(
    const FString& SelectionKey,
    const TArray<FName>& CategoryIds) const
{
    for (const FName CategoryId : CategoryIds)
    {
        if (IsCategoryVisible(SelectionKey, CategoryId))
        {
            return true;
        }
    }

    return false;
}

bool FPruneState::IsSelectionCustom(const FString& SelectionKey) const
{
    const FSelectionState* Selection = SelectionStates.Find(SelectionKey);

    return Selection != nullptr
        && Selection->ActivePresetId.IsEmpty()
        && !Selection->HiddenCategories.IsEmpty();
}

FText FPruneState::GetSelectionModeLabel(const FString& SelectionKey) const
{
    const FSelectionState* Selection = SelectionStates.Find(SelectionKey);

    if (Selection != nullptr && !Selection->ActivePresetId.IsEmpty())
    {
        if (const FPresetData* Preset = PresetsById.Find(Selection->ActivePresetId))
        {
            return FText::FromString(Preset->Name);
        }
    }

    if (Selection != nullptr && !Selection->HiddenCategories.IsEmpty())
    {
        return FText::FromString(TEXT("Custom"));
    }

    return FText::FromString(TEXT("All Categories"));
}

FString FPruneState::GetActivePresetId(const FString& SelectionKey) const
{
    if (const FSelectionState* Selection = SelectionStates.Find(SelectionKey))
    {
        if (!Selection->ActivePresetId.IsEmpty()
            && PresetsById.Contains(Selection->ActivePresetId))
        {
            return Selection->ActivePresetId;
        }
    }

    return FString();
}

void FPruneState::SetCategoriesVisible(
    const FString& SelectionKey,
    const TArray<FName>& CategoryIds,
    bool bVisible)
{
    if (SelectionKey.IsEmpty() || CategoryIds.IsEmpty())
    {
        return;
    }

    FSelectionState& Selection = SelectionStates.FindOrAdd(SelectionKey);

    // Any direct checkbox edit leaves a named preset and becomes Custom.
    Selection.ActivePresetId.Reset();

    for (const FName CategoryId : CategoryIds)
    {
        if (CategoryId.IsNone())
        {
            continue;
        }

        if (bVisible)
        {
            Selection.HiddenCategories.Remove(CategoryId);
        }
        else
        {
            Selection.HiddenCategories.Add(CategoryId);
        }
    }

    if (Selection.HiddenCategories.IsEmpty())
    {
        SelectionStates.Remove(SelectionKey);
    }

    SavePersistentState();

    CompactContexts();
    for (const TWeakPtr<FPruneLayoutContext>& WeakContext : LayoutContexts)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = WeakContext.Pin())
        {
            if (Context->GetSelectionKey() == SelectionKey)
            {
                Context->ApplyCategoryGroupVisibility(CategoryIds, bVisible);
            }
        }
    }
}

void FPruneState::ShowAllCategories(const FString& SelectionKey)
{
    if (SelectionKey.IsEmpty())
    {
        return;
    }

    SelectionStates.Remove(SelectionKey);
    SavePersistentState();

    CompactContexts();
    for (const TWeakPtr<FPruneLayoutContext>& WeakContext : LayoutContexts)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = WeakContext.Pin())
        {
            if (Context->GetSelectionKey() == SelectionKey)
            {
                Context->ShowAllCurrentCategories();
            }
        }
    }
}

TArray<FPrunePresetSummary> FPruneState::GetPresets() const
{
    TArray<FPrunePresetSummary> Result;
    Result.Reserve(PresetsById.Num());

    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        FPrunePresetSummary& Summary = Result.AddDefaulted_GetRef();
        Summary.Id = Entry.Key;
        Summary.Name = FText::FromString(Entry.Value.Name);
    }

    Result.Sort(
        [](const FPrunePresetSummary& A, const FPrunePresetSummary& B)
        {
            return A.Name.ToString() < B.Name.ToString();
        });

    return Result;
}

bool FPruneState::ApplyPreset(
    const FString& SelectionKey,
    const FString& PresetId)
{
    if (SelectionKey.IsEmpty())
    {
        return false;
    }

    const FPresetData* Preset = PresetsById.Find(PresetId);
    if (Preset == nullptr)
    {
        return false;
    }

    FSelectionState& Selection = SelectionStates.FindOrAdd(SelectionKey);
    Selection.HiddenCategories = Preset->HiddenCategories;
    Selection.ActivePresetId = PresetId;

    SavePersistentState();
    ApplySelectionStateToLiveContexts(SelectionKey);
    return true;
}

bool FPruneState::SaveCurrentAsPreset(
    const FString& SelectionKey,
    const FString& PresetName,
    FString& OutError)
{
    OutError.Reset();

    const FString CleanName = PresetName.TrimStartAndEnd();
    if (CleanName.IsEmpty())
    {
        OutError = TEXT("Preset name cannot be empty.");
        return false;
    }

    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        if (Entry.Value.Name.Equals(CleanName, ESearchCase::IgnoreCase))
        {
            OutError = FString::Printf(
                TEXT("A Prune preset named '%s' already exists."),
                *CleanName);
            return false;
        }
    }

    FPresetData Preset;
    Preset.Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Preset.Name = CleanName;

    if (const FSelectionState* Current = SelectionStates.Find(SelectionKey))
    {
        Preset.HiddenCategories = Current->HiddenCategories;
    }

    const FString NewPresetId = Preset.Id;
    PresetsById.Add(NewPresetId, MoveTemp(Preset));

    if (!SelectionKey.IsEmpty())
    {
        FSelectionState& Current = SelectionStates.FindOrAdd(SelectionKey);
        Current.ActivePresetId = NewPresetId;
    }

    SavePersistentState();
    return true;
}

bool FPruneState::DeletePreset(const FString& PresetId)
{
    if (PresetId.IsEmpty() || PresetsById.Remove(PresetId) == 0)
    {
        return false;
    }

    TArray<FString> EmptySelectionKeys;

    for (TPair<FString, FSelectionState>& Entry : SelectionStates)
    {
        if (Entry.Value.ActivePresetId == PresetId)
        {
            // Preserve the actual hidden categories. Deleting a preset should
            // never unexpectedly reveal Details categories; affected classes
            // simply become Custom instead.
            Entry.Value.ActivePresetId.Reset();

            if (Entry.Value.HiddenCategories.IsEmpty())
            {
                EmptySelectionKeys.Add(Entry.Key);
            }
        }
    }

    for (const FString& Key : EmptySelectionKeys)
    {
        SelectionStates.Remove(Key);
    }

    SavePersistentState();
    return true;
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

void FPruneState::LogCurrentCategories(
    const TSharedRef<FPruneLayoutContext>& Context) const
{
    Context->LogCurrentCategories(*this);
}

bool FPruneState::IsPersistentSelectionKey(const FString& SelectionKey)
{
    return !SelectionKey.IsEmpty()
        && !SelectionKey.StartsWith(TEXT("Mixed:"));
}

void FPruneState::CompactContexts()
{
    LayoutContexts.RemoveAll(
        [](const TWeakPtr<FPruneLayoutContext>& WeakContext)
        {
            return !WeakContext.IsValid();
        });
}

void FPruneState::ApplySelectionStateToLiveContexts(const FString& SelectionKey)
{
    CompactContexts();

    for (const TWeakPtr<FPruneLayoutContext>& WeakContext : LayoutContexts)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = WeakContext.Pin())
        {
            if (Context->GetSelectionKey() == SelectionKey)
            {
                Context->ApplyHiddenState(*this);
            }
        }
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

        const FString Section = PruneStatePrivate::MakePresetSection(PresetId);
        FString Name;
        GConfig->GetString(
            *Section,
            PruneStatePrivate::NameKey,
            Name,
            GEditorPerProjectIni);

        Name = Name.TrimStartAndEnd();
        if (Name.IsEmpty())
        {
            continue;
        }

        TArray<FString> HiddenStrings;
        GConfig->GetArray(
            *Section,
            PruneStatePrivate::HiddenCategoriesKey,
            HiddenStrings,
            GEditorPerProjectIni);

        FPresetData Preset;
        Preset.Id = PresetId;
        Preset.Name = Name;
        Preset.HiddenCategories =
            PruneStatePrivate::StringsToNames(HiddenStrings);

        PresetsById.Add(PresetId, MoveTemp(Preset));
    }

    TArray<FString> SelectionKeys;
    GConfig->GetArray(
        PruneStatePrivate::RootSection,
        PruneStatePrivate::SelectionKeysKey,
        SelectionKeys,
        GEditorPerProjectIni);

    for (int32 Index = 0; Index < SelectionKeys.Num(); ++Index)
    {
        const FString SelectionKey = SelectionKeys[Index].TrimStartAndEnd();
        if (!IsPersistentSelectionKey(SelectionKey))
        {
            continue;
        }

        const FString Section = PruneStatePrivate::MakeSelectionSection(Index);

        TArray<FString> HiddenStrings;
        GConfig->GetArray(
            *Section,
            PruneStatePrivate::HiddenCategoriesKey,
            HiddenStrings,
            GEditorPerProjectIni);

        FString ActivePresetId;
        GConfig->GetString(
            *Section,
            PruneStatePrivate::ActivePresetIdKey,
            ActivePresetId,
            GEditorPerProjectIni);

        ActivePresetId = ActivePresetId.TrimStartAndEnd();
        if (!ActivePresetId.IsEmpty() && !PresetsById.Contains(ActivePresetId))
        {
            ActivePresetId.Reset();
        }

        FSelectionState Selection;
        Selection.HiddenCategories =
            PruneStatePrivate::StringsToNames(HiddenStrings);
        Selection.ActivePresetId = ActivePresetId;

        if (!Selection.HiddenCategories.IsEmpty()
            || !Selection.ActivePresetId.IsEmpty())
        {
            SelectionStates.Add(SelectionKey, MoveTemp(Selection));
        }
    }

    UE_LOG(
        LogPrune,
        Log,
        TEXT("Prune loaded %d preset(s) and %d persistent class state(s)."),
        PresetsById.Num(),
        SelectionStates.Num());
}

void FPruneState::SavePersistentState() const
{
    if (GConfig == nullptr)
    {
        return;
    }

    TArray<FString> PresetIds;
    PresetIds.Reserve(PresetsById.Num());

    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        PresetIds.Add(Entry.Key);
    }

    PresetIds.Sort(
        [this](const FString& A, const FString& B)
        {
            const FPresetData* PresetA = PresetsById.Find(A);
            const FPresetData* PresetB = PresetsById.Find(B);

            if (PresetA == nullptr || PresetB == nullptr)
            {
                return A < B;
            }

            return PresetA->Name < PresetB->Name;
        });

    GConfig->SetArray(
        PruneStatePrivate::RootSection,
        PruneStatePrivate::PresetIdsKey,
        PresetIds,
        GEditorPerProjectIni);

    for (const FString& PresetId : PresetIds)
    {
        const FPresetData* Preset = PresetsById.Find(PresetId);
        if (Preset == nullptr)
        {
            continue;
        }

        const FString Section = PruneStatePrivate::MakePresetSection(PresetId);
        GConfig->SetString(
            *Section,
            PruneStatePrivate::NameKey,
            *Preset->Name,
            GEditorPerProjectIni);

        const TArray<FString> HiddenStrings =
            PruneStatePrivate::NamesToStrings(Preset->HiddenCategories);
        GConfig->SetArray(
            *Section,
            PruneStatePrivate::HiddenCategoriesKey,
            HiddenStrings,
            GEditorPerProjectIni);
    }

    TArray<FString> SelectionKeys;
    for (const TPair<FString, FSelectionState>& Entry : SelectionStates)
    {
        if (IsPersistentSelectionKey(Entry.Key)
            && (!Entry.Value.HiddenCategories.IsEmpty()
                || !Entry.Value.ActivePresetId.IsEmpty()))
        {
            SelectionKeys.Add(Entry.Key);
        }
    }

    SelectionKeys.Sort();

    GConfig->SetArray(
        PruneStatePrivate::RootSection,
        PruneStatePrivate::SelectionKeysKey,
        SelectionKeys,
        GEditorPerProjectIni);

    for (int32 Index = 0; Index < SelectionKeys.Num(); ++Index)
    {
        const FString& SelectionKey = SelectionKeys[Index];
        const FSelectionState* Selection = SelectionStates.Find(SelectionKey);
        if (Selection == nullptr)
        {
            continue;
        }

        const FString Section = PruneStatePrivate::MakeSelectionSection(Index);
        const TArray<FString> HiddenStrings =
            PruneStatePrivate::NamesToStrings(Selection->HiddenCategories);

        GConfig->SetArray(
            *Section,
            PruneStatePrivate::HiddenCategoriesKey,
            HiddenStrings,
            GEditorPerProjectIni);

        GConfig->SetString(
            *Section,
            PruneStatePrivate::ActivePresetIdKey,
            *Selection->ActivePresetId,
            GEditorPerProjectIni);
    }

    GConfig->Flush(false, GEditorPerProjectIni);
}
