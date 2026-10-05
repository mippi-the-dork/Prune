// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneState.h"

#include "DetailCategoryBuilder.h"
#include "GameFramework/Actor.h"
#include "IDetailsView.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Guid.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogPrune, Log, All);

namespace PruneStatePrivate
{
    static const TCHAR* RootSection = TEXT("Prune.UserState");
    static const TCHAR* PresetIdsKey = TEXT("PresetIds");
    static const TCHAR* HiddenCategoriesKey = TEXT("HiddenCategoryIds");
    static const TCHAR* NameKey = TEXT("Name");
    static constexpr int32 FirstPruneSectionOrder = 10000;

    static FString MakePresetSection(const FString& PresetId)
    {
        return FString::Printf(TEXT("Prune.Preset.%s"), *PresetId);
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
    FText InSelectionLabel,
    TSharedPtr<const IDetailsView> InDetailsView)
    : SelectionLabel(MoveTemp(InSelectionLabel))
    , DetailsView(InDetailsView)
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
            TEXT("  %s | Display: %s"),
            *Category.Id.ToString(),
            *Category.DisplayName.ToString());
    }
}

FPruneState::FPruneState()
{
    LoadPersistentState();
}

FPruneState::~FPruneState()
{
    RemoveAllNativeSections();
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

TSet<FName> FPruneState::GetPresetHiddenCategories(
    const FString& PresetId) const
{
    if (const FPresetData* Preset = PresetsById.Find(PresetId))
    {
        return Preset->HiddenCategories;
    }

    return TSet<FName>();
}

bool FPruneState::CreatePreset(
    const FString& PresetName,
    const TSet<FName>& HiddenCategories,
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
    Preset.HiddenCategories = HiddenCategories;

    PresetsById.Add(Preset.Id, MoveTemp(Preset));

    SavePersistentState();
    RebuildAllNativeSections();
    RefreshDetailsViews();
    return true;
}

bool FPruneState::UpdatePreset(
    const FString& PresetId,
    const FString& PresetName,
    const TSet<FName>& HiddenCategories,
    FString& OutError)
{
    OutError.Reset();

    FPresetData* Preset = PresetsById.Find(PresetId);
    if (Preset == nullptr)
    {
        OutError = TEXT("The Prune preset no longer exists.");
        return false;
    }

    const FString CleanName = PresetName.TrimStartAndEnd();
    if (CleanName.IsEmpty())
    {
        OutError = TEXT("Preset name cannot be empty.");
        return false;
    }

    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        if (Entry.Key != PresetId
            && Entry.Value.Name.Equals(CleanName, ESearchCase::IgnoreCase))
        {
            OutError = FString::Printf(
                TEXT("A Prune preset named '%s' already exists."),
                *CleanName);
            return false;
        }
    }

    Preset->Name = CleanName;
    Preset->HiddenCategories = HiddenCategories;

    SavePersistentState();
    RebuildAllNativeSections();
    RefreshDetailsViews();
    return true;
}

bool FPruneState::DeletePreset(const FString& PresetId)
{
    if (PresetId.IsEmpty() || PresetsById.Remove(PresetId) == 0)
    {
        return false;
    }

    if (FPropertyEditorModule* PropertyEditor =
        FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
    {
        PropertyEditor->RemoveSection(
            AActor::StaticClass()->GetFName(),
            MakeSectionName(PresetId));
    }

    SavePersistentState();
    RebuildAllNativeSections();

    // A live Details view may currently have the deleted section selected.
    // Reset those views to All before the refresh so they never become an
    // empty filter with a now-invalid section name.
    ResetLiveViewsToAll();
    RefreshDetailsViews();
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

    // Rebuild on first discovery so every named preset immediately becomes a
    // real native section before SDetailsView::RebuildSectionSelector runs.
    if (bDiscoveredNewCategory && !PresetsById.IsEmpty())
    {
        RebuildAllNativeSections();
    }
}

void FPruneState::RemoveAllNativeSections()
{
    if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        return;
    }

    FPropertyEditorModule& PropertyEditor =
        FModuleManager::GetModuleChecked<FPropertyEditorModule>(
            TEXT("PropertyEditor"));

    for (const TPair<FString, FPresetData>& Entry : PresetsById)
    {
        PropertyEditor.RemoveSection(
            AActor::StaticClass()->GetFName(),
            MakeSectionName(Entry.Key));
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

void FPruneState::CompactContexts()
{
    LayoutContexts.RemoveAll(
        [](const TWeakPtr<FPruneLayoutContext>& WeakContext)
        {
            return !WeakContext.IsValid();
        });
}

void FPruneState::RebuildAllNativeSections()
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
            RebuildNativeSection(*Preset, Order++);
        }
    }
}

void FPruneState::RebuildNativeSection(
    const FPresetData& Preset,
    int32 Order)
{
    FPropertyEditorModule& PropertyEditor =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
            TEXT("PropertyEditor"));

    const FName ActorClassName = AActor::StaticClass()->GetFName();
    const FName SectionName = MakeSectionName(Preset.Id);

    // Recreate instead of FindOrCreate-in-place so a rename immediately updates
    // the display label and order as well as category membership.
    PropertyEditor.RemoveSection(ActorClassName, SectionName);

    TSharedRef<FPropertySection> Section =
        PropertyEditor.FindOrCreateSection(
            ActorClassName,
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

void FPruneState::ResetLiveViewsToAll()
{
    CompactContexts();

    for (const TWeakPtr<FPruneLayoutContext>& WeakContext : LayoutContexts)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = WeakContext.Pin())
        {
            if (const TSharedPtr<const IDetailsView> ConstDetailsView =
                Context->GetDetailsView())
            {
                IDetailsView* DetailsView =
                    const_cast<IDetailsView*>(ConstDetailsView.Get());
                DetailsView->ResetToDefaultSection();
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

        const FString SectionName =
            PruneStatePrivate::MakePresetSection(PresetId);

        FString Name;
        GConfig->GetString(
            *SectionName,
            PruneStatePrivate::NameKey,
            Name,
            GEditorPerProjectIni);

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

        FPresetData Preset;
        Preset.Id = PresetId;
        Preset.Name = Name;
        Preset.HiddenCategories =
            PruneStatePrivate::StringsToNames(HiddenCategoryStrings);

        PresetsById.Add(Preset.Id, MoveTemp(Preset));
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
        const FString SectionName =
            PruneStatePrivate::MakePresetSection(Entry.Key);

        GConfig->SetString(
            *SectionName,
            PruneStatePrivate::NameKey,
            *Entry.Value.Name,
            GEditorPerProjectIni);

        const TArray<FString> HiddenCategoryStrings =
            PruneStatePrivate::NamesToStrings(Entry.Value.HiddenCategories);

        GConfig->SetArray(
            *SectionName,
            PruneStatePrivate::HiddenCategoriesKey,
            HiddenCategoryStrings,
            GEditorPerProjectIni);
    }

    GConfig->Flush(false, GEditorPerProjectIni);
}
