// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IDetailCategoryBuilder;
class IDetailsView;

struct FPruneCategoryInfo
{
    FName Id = NAME_None;
    FText DisplayName;
};

/**
 * One user-facing category row in the preset editor.
 *
 * Unreal can expose multiple internal IDs with one display label. Prune edits
 * those IDs as one row while preserving the real IDs in the saved preset.
 */
struct FPruneCategoryGroupInfo
{
    FText DisplayName;
    TArray<FName> Ids;
};

struct FPrunePresetSummary
{
    FString Id;
    FText Name;
};

/**
 * One live Actor Details layout.
 *
 * The layout context owns only descriptive category data and a weak reference
 * to the Details view. It never owns the engine category builders.
 */
class FPruneLayoutContext : public TSharedFromThis<FPruneLayoutContext>
{
public:
    FPruneLayoutContext(
        FText InSelectionLabel,
        TSharedPtr<const IDetailsView> InDetailsView);

    void UpdateFromFinalCategories(
        const TMap<FName, IDetailCategoryBuilder*>& InCategories);

    TArray<FPruneCategoryGroupInfo> GetCategoryGroups() const;
    TArray<FName> GetCategoryIds() const;

    const FText& GetSelectionLabel() const { return SelectionLabel; }
    TSharedPtr<const IDetailsView> GetDetailsView() const
    {
        return DetailsView.Pin();
    }

    void LogCurrentCategories() const;

private:
    FText SelectionLabel;
    TWeakPtr<const IDetailsView> DetailsView;
    TArray<FPruneCategoryInfo> RawCategories;
    TArray<FPruneCategoryGroupInfo> CategoryGroups;
};

/**
 * Persistent Prune preset state.
 *
 * Presets remain class-agnostic deny-lists of internal category IDs. They are
 * exposed to the Details panel as native Property Sections. Whenever Prune
 * discovers a new category, it adds that category to every preset section
 * unless the preset explicitly hides that ID. This preserves the original
 * "unknown categories default visible" rule while using Unreal's native
 * section-filter buttons as the actual preset UI.
 */
class FPruneState : public TSharedFromThis<FPruneState>
{
public:
    FPruneState();
    ~FPruneState();

    TArray<FPrunePresetSummary> GetPresets() const;
    TSet<FName> GetPresetHiddenCategories(const FString& PresetId) const;

    bool CreatePreset(
        const FString& PresetName,
        const TSet<FName>& HiddenCategories,
        FString& OutError);

    bool UpdatePreset(
        const FString& PresetId,
        const FString& PresetName,
        const TSet<FName>& HiddenCategories,
        FString& OutError);

    bool DeletePreset(const FString& PresetId);

    void RegisterLayoutContext(const TSharedRef<FPruneLayoutContext>& Context);
    TArray<TSharedRef<FPruneLayoutContext>> GetLiveLayoutContexts();
    void SyncNativeSectionsForContext(
        const TSharedRef<FPruneLayoutContext>& Context);

    void RemoveAllNativeSections();
    void LogCurrentCategories(
        const TSharedRef<FPruneLayoutContext>& Context) const;

private:
    struct FPresetData
    {
        FString Id;
        FString Name;
        TSet<FName> HiddenCategories;
    };

    static FName MakeSectionName(const FString& PresetId);

    void CompactContexts();
    void RebuildAllNativeSections();
    void RebuildNativeSection(const FPresetData& Preset, int32 Order);
    void RefreshDetailsViews();
    void ResetLiveViewsToAll();
    void LoadPersistentState();
    void SavePersistentState() const;

    TMap<FString, FPresetData> PresetsById;
    TSet<FName> KnownCategoryIds;
    TArray<TWeakPtr<FPruneLayoutContext>> LayoutContexts;
};
