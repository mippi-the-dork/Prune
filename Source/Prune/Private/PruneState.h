// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FPruneState;
class IDetailCategoryBuilder;

struct FPruneCategoryInfo
{
    FName Id = NAME_None;
    FText DisplayName;
};

/**
 * One user-facing category menu entry.
 *
 * Unreal may expose more than one internal category ID with the same visible
 * label. Prune presents those IDs as one row while continuing to store and
 * apply visibility against the real internal IDs.
 */
struct FPruneCategoryGroupInfo
{
    FText DisplayName;
    TArray<FName> Ids;
};

/** Lightweight data used by the preset menu. */
struct FPrunePresetSummary
{
    FString Id;
    FText Name;
};

/**
 * Per-Details-layout live category bridge.
 *
 * The widget owns this context for exactly as long as the corresponding Actor
 * Details layout exists. Prune's shared state keeps only weak references, so a
 * destroyed/rebuilt Details layout cannot leave permanent raw category-builder
 * pointers behind.
 */
class FPruneLayoutContext : public TSharedFromThis<FPruneLayoutContext>
{
public:
    FPruneLayoutContext(FString InSelectionKey, FText InSelectionLabel);

    void UpdateFromFinalCategories(
        const TMap<FName, IDetailCategoryBuilder*>& InCategories,
        FName ExcludedCategoryId);

    TArray<FPruneCategoryGroupInfo> GetCategoryGroups() const;
    int32 GetHiddenCurrentCategoryCount(const FPruneState& State) const;

    const FString& GetSelectionKey() const { return SelectionKey; }
    const FText& GetSelectionLabel() const { return SelectionLabel; }

    void ApplyCategoryVisibility(FName CategoryId, bool bVisible);
    void ApplyCategoryGroupVisibility(
        const TArray<FName>& CategoryIds,
        bool bVisible);
    void ApplyHiddenState(const FPruneState& State);
    void ShowAllCurrentCategories();

    void LogCurrentCategories(const FPruneState& State) const;

private:
    FString SelectionKey;
    FText SelectionLabel;
    TArray<FPruneCategoryInfo> RawCategories;
    TArray<FPruneCategoryGroupInfo> CategoryGroups;
    TMap<FName, IDetailCategoryBuilder*> CategoryBuilders;
};

/**
 * Persistent per-user Prune state.
 *
 * Exact Actor-class selections are stored in EditorPerProjectUserSettings.ini.
 * Blueprint classes use the generating Blueprint asset path, so recompiles do
 * not create a new persistent identity. Mixed-class selections remain
 * session-only because they are selection compositions rather than classes.
 *
 * Presets are class-agnostic deny-lists of internal category IDs. Unknown/new
 * categories therefore remain visible unless a preset explicitly names them.
 */
class FPruneState : public TSharedFromThis<FPruneState>
{
public:
    FPruneState();

    bool IsCategoryVisible(
        const FString& SelectionKey,
        FName CategoryId) const;

    bool AreAllCategoriesVisible(
        const FString& SelectionKey,
        const TArray<FName>& CategoryIds) const;

    bool AreAnyCategoriesVisible(
        const FString& SelectionKey,
        const TArray<FName>& CategoryIds) const;

    bool IsSelectionCustom(const FString& SelectionKey) const;
    FText GetSelectionModeLabel(const FString& SelectionKey) const;
    FString GetActivePresetId(const FString& SelectionKey) const;

    void SetCategoriesVisible(
        const FString& SelectionKey,
        const TArray<FName>& CategoryIds,
        bool bVisible);

    void ShowAllCategories(const FString& SelectionKey);

    TArray<FPrunePresetSummary> GetPresets() const;
    bool ApplyPreset(
        const FString& SelectionKey,
        const FString& PresetId);
    bool SaveCurrentAsPreset(
        const FString& SelectionKey,
        const FString& PresetName,
        FString& OutError);
    bool DeletePreset(const FString& PresetId);

    void RegisterLayoutContext(const TSharedRef<FPruneLayoutContext>& Context);
    void LogCurrentCategories(const TSharedRef<FPruneLayoutContext>& Context) const;

private:
    struct FSelectionState
    {
        TSet<FName> HiddenCategories;
        FString ActivePresetId;
    };

    struct FPresetData
    {
        FString Id;
        FString Name;
        TSet<FName> HiddenCategories;
    };

    static bool IsPersistentSelectionKey(const FString& SelectionKey);

    void CompactContexts();
    void ApplySelectionStateToLiveContexts(const FString& SelectionKey);
    void LoadPersistentState();
    void SavePersistentState() const;

    TMap<FString, FSelectionState> SelectionStates;
    TMap<FString, FPresetData> PresetsById;
    TArray<TWeakPtr<FPruneLayoutContext>> LayoutContexts;
};
