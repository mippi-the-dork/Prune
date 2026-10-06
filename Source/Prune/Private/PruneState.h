// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtrTemplates.h"

class IDetailCategoryBuilder;
class IDetailsView;
class SWidget;
class UClass;

struct FPruneCategoryInfo
{
    FName Id = NAME_None;
    FText DisplayName;
    int32 NativeSortOrder = 0;
};

/** One user-facing category row in the filter editor. */
struct FPruneCategoryGroupInfo
{
    FText DisplayName;
    TArray<FName> Ids;
    int32 NativeSortOrder = 0;
};

enum class EPruneFilterKind : uint8
{
    None,
    Custom,
    Native
};

/** The single editable section currently selected in a Details view. */
struct FPruneEditableFilter
{
    EPruneFilterKind Kind = EPruneFilterKind::None;
    FName SectionName = NAME_None;
    FText DisplayName;
    int32 SectionOrder = 0;
    FString CustomPresetId;
    FString NativeOverrideId;
    bool bHasNativeOverride = false;
};

/** Initial data consumed by the New/Edit Filter modal. */
struct FPruneFilterEditorData
{
    FString Name;
    FString Description;
    bool bGlobal = false;
    FName ScopeClassName = NAME_None;
    TSet<FName> HiddenCategories;
    TArray<FName> OrderedCategoryIds;
    bool bNativeFilter = false;
    bool bHasNativeOverride = false;
};


struct FPruneManagedFilterInfo
{
    FString OrderKey;
    EPruneFilterKind Kind = EPruneFilterKind::None;
    FName SectionName = NAME_None;
    FText DisplayName;
    FString Description;
    FString CustomPresetId;
    FString NativeOverrideId;
    bool bHasNativeOverride = false;
    bool bGlobal = false;
    FName ScopeClassName = NAME_None;
    int32 NativeOrder = 0;
};

class FPruneLayoutContext : public TSharedFromThis<FPruneLayoutContext>
{
public:
    FPruneLayoutContext(
        FText InSelectionLabel,
        TSharedPtr<const IDetailsView> InDetailsView,
        UClass* InActorClass);

    void UpdateFromFinalCategories(
        const TMap<FName, IDetailCategoryBuilder*>& InCategories);

    TArray<FPruneCategoryGroupInfo> GetCategoryGroups() const;
    TArray<FName> GetCategoryIds() const;

    const FText& GetSelectionLabel() const { return SelectionLabel; }
    TSharedPtr<const IDetailsView> GetDetailsView() const
    {
        return DetailsView.Pin();
    }
    UClass* GetActorClass() const { return ActorClass.Get(); }
    FName GetActorClassName() const;

    void SetSectionSelectorWidget(const TSharedPtr<SWidget>& InWidget);
    TArray<FString> GetCheckedSectionLabels() const;
    bool EnsureDefaultSectionSelected();
    void ApplySectionButtonPresentation(
        const TArray<FString>& OrderedLabels,
        const TMap<FString, FText>& DescriptionTooltips);

    void SetFilterButtonContextMenuHandler(
        TFunction<void(const FString&, const FVector2D&)> InHandler)
    {
        FilterButtonContextMenuHandler = MoveTemp(InHandler);
    }

    void LogCurrentCategories() const;

private:
    FText SelectionLabel;
    TWeakPtr<const IDetailsView> DetailsView;
    TWeakObjectPtr<UClass> ActorClass;
    TWeakPtr<SWidget> SectionSelectorWidget;
    TFunction<void(const FString&, const FVector2D&)> FilterButtonContextMenuHandler;
    TArray<FPruneCategoryInfo> RawCategories;
    TArray<FPruneCategoryGroupInfo> CategoryGroups;
};

class FPruneState : public TSharedFromThis<FPruneState>
{
public:
    FPruneState();
    ~FPruneState();

    bool CreatePreset(
        const TSharedRef<FPruneLayoutContext>& Context,
        const FString& PresetName,
        const FString& Description,
        bool bGlobal,
        const TSet<FName>& HiddenCategories,
        const TArray<FName>& OrderedCategoryIds,
        FString& OutError);

    bool UpdatePreset(
        const TSharedRef<FPruneLayoutContext>& Context,
        const FString& PresetId,
        const FString& PresetName,
        const FString& Description,
        bool bGlobal,
        const TSet<FName>& HiddenCategories,
        const TArray<FName>& OrderedCategoryIds,
        FString& OutError);

    bool DeletePreset(const FString& PresetId);

    FString MakeSuggestedDuplicateName(
        const TSharedRef<FPruneLayoutContext>& Context,
        const FString& BaseName) const;

    bool ResolveSingleActiveFilter(
        const TSharedRef<FPruneLayoutContext>& Context,
        FPruneEditableFilter& OutFilter) const;

    bool BuildEditorData(
        const TSharedRef<FPruneLayoutContext>& Context,
        const FPruneEditableFilter& Filter,
        FPruneFilterEditorData& OutData);

    bool SaveNativeOverride(
        const TSharedRef<FPruneLayoutContext>& Context,
        const FPruneEditableFilter& Filter,
        const FString& Description,
        bool bGlobal,
        const TSet<FName>& HiddenCategories,
        const TArray<FName>& OrderedCategoryIds,
        FString& OutError);

    bool ResetNativeOverride(const FPruneEditableFilter& Filter);

    bool CanEditActiveFilter(
        const TSharedRef<FPruneLayoutContext>& Context) const;

    TArray<FName> GetActiveCategoryOrder(
        const TSharedRef<FPruneLayoutContext>& Context) const;

    TArray<FPruneManagedFilterInfo> GetManagedFilters(
        const TSharedRef<FPruneLayoutContext>& Context) const;
    bool ResolveManagedFilter(
        const TSharedRef<FPruneLayoutContext>& Context,
        const FString& OrderKey,
        FPruneEditableFilter& OutFilter) const;
    void SetFilterButtonOrder(
        const TSharedRef<FPruneLayoutContext>& Context,
        const TArray<FString>& OrderedKeys);

    bool HasCustomFilterButtonOrder(
        const TSharedRef<FPruneLayoutContext>& Context) const;
    void ApplyFilterBarPresentation(
        const TSharedRef<FPruneLayoutContext>& Context) const;

    void RegisterLayoutContext(const TSharedRef<FPruneLayoutContext>& Context);
    TArray<TSharedRef<FPruneLayoutContext>> GetLiveLayoutContexts();
    TSharedPtr<FPruneLayoutContext> FindLatestLayoutContextForDetailsView(
        const TSharedPtr<const IDetailsView>& DetailsView);

    void SyncNativeSectionsForContext(
        const TSharedRef<FPruneLayoutContext>& Context);

    /** Remove Prune-created sections and restore runtime native section edits. */
    void RemoveAllNativeSections();

    void LogCurrentCategories(
        const TSharedRef<FPruneLayoutContext>& Context) const;

private:
    struct FPresetData
    {
        FString Id;
        FString Name;
        FString Description;
        bool bGlobal = true;
        FName ClassName = NAME_None;
        TSet<FName> HiddenCategories;
        TArray<FName> OrderedCategoryIds;
    };

    struct FNativeOverrideData
    {
        FString Id;
        FName SectionName = NAME_None;
        FString DisplayName;
        FString Description;
        int32 SectionOrder = 0;
        bool bGlobal = false;
        FName ClassName = NAME_None;
        TSet<FName> AddedCategories;
        TSet<FName> RemovedCategories;
        TArray<FName> OrderedCategoryIds;

        // Runtime-only snapshots used to restore the effective native behavior.
        TMap<FString, bool> BaselineMembership;
        TSet<FName> TouchedClasses;
    };

    struct FObservedViewOrderState
    {
        TWeakPtr<const IDetailsView> DetailsView;
        FString Signature;
        bool bHadCustomOrder = false;
    };

    struct FSectionDefinition
    {
        FName Name = NAME_None;
        FText DisplayName;
        int32 Order = 0;
    };

    static FName MakeSectionName(const FString& PresetId);
    static FString MakePresetOrderKey(const FString& PresetId);
    static FString MakeNativeOrderKey(FName SectionName);
    static FString MakeBaselineKey(FName ClassName, FName CategoryId);

    void CompactContexts();
    void CompactObservedViews();

    bool DoesScopeApplyToClass(
        bool bGlobal,
        FName ScopeClassName,
        const UClass* ActorClass) const;

    bool IsCustomSectionName(FName SectionName, FString* OutPresetId = nullptr) const;
    const FPresetData* FindPresetBySectionName(FName SectionName) const;
    FPresetData* FindPresetBySectionName(FName SectionName);

    TArray<FSectionDefinition> CollectNativeSections(
        const TSharedRef<FPruneLayoutContext>& Context) const;

    const FNativeOverrideData* FindApplicableNativeOverride(
        FName SectionName,
        const UClass* ActorClass) const;
    FNativeOverrideData* FindApplicableNativeOverride(
        FName SectionName,
        const UClass* ActorClass);

    bool IsSectionIncludedForCategory(
        const UClass* ActorClass,
        FName SectionName,
        FName CategoryId) const;

    bool ValidateCustomPresetName(
        const TSharedRef<FPruneLayoutContext>& Context,
        const FString& CleanName,
        const FString& IgnorePresetId,
        FString& OutError) const;

    void RebuildAllCustomSections();
    void RebuildCustomSection(const FPresetData& Preset, int32 Order);
    void RemoveCustomSection(const FPresetData& Preset);

    void ApplyNativeOverridesForContext(
        const TSharedRef<FPruneLayoutContext>& Context);
    void ApplyNativeOverrideForContext(
        FNativeOverrideData& Override,
        const TSharedRef<FPruneLayoutContext>& Context);
    void CaptureNativeBaseline(
        FNativeOverrideData& Override,
        const TSharedRef<FPruneLayoutContext>& Context);
    void RestoreNativeOverrideRuntime(FNativeOverrideData& Override);
    void ClearDependentNativeBaselines(FName SectionName, const FString& IgnoreOverrideId);

    static TArray<FName> MergeCategoryOrder(
        const TArray<FName>& ExistingOrder,
        const TArray<FName>& CurrentOrder,
        const TArray<FName>& CurrentCategoryIds);

    bool TickActiveCategoryOrders(float DeltaTime);
    FString BuildActiveOrderSignature(
        const TSharedRef<FPruneLayoutContext>& Context,
        bool& bOutHasCustomOrder) const;

    void RefreshDetailsViews();
    void ResetLiveViewsToAll();
    void LoadPersistentState();
    void SavePersistentState() const;

    TMap<FString, FPresetData> PresetsById;
    TMap<FString, FNativeOverrideData> NativeOverridesById;
    TMap<FName, TArray<FString>> FilterButtonOrderByClass;
    TSet<FName> KnownCategoryIds;
    TArray<TWeakPtr<FPruneLayoutContext>> LayoutContexts;
    TArray<FObservedViewOrderState> ObservedViewOrders;
    FTSTicker::FDelegateHandle ActiveOrderTickerHandle;
};
