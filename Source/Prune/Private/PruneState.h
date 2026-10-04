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
    void UpdateFromFinalCategories(
        const TMap<FName, IDetailCategoryBuilder*>& InCategories,
        FName ExcludedCategoryId);

    TArray<FPruneCategoryInfo> GetCategories() const;
    int32 GetHiddenCurrentCategoryCount(const FPruneState& State) const;

    void ApplyCategoryVisibility(FName CategoryId, bool bVisible);
    void ApplyHiddenState(const FPruneState& State);
    void ShowAllCurrentCategories();

    void LogCurrentCategories(const FPruneState& State) const;

private:
    TArray<FPruneCategoryInfo> Categories;
    TMap<FName, IDetailCategoryBuilder*> CategoryBuilders;
};

class FPruneState : public TSharedFromThis<FPruneState>
{
public:
    bool IsCategoryVisible(FName CategoryId) const;
    void SetCategoryVisible(FName CategoryId, bool bVisible);
    void ShowAllCategories();

    const TSet<FName>& GetHiddenCategories() const { return HiddenCategories; }

    void RegisterLayoutContext(const TSharedRef<FPruneLayoutContext>& Context);
    void LogCurrentCategories(const TSharedRef<FPruneLayoutContext>& Context) const;

private:
    void CompactContexts();

    TSet<FName> HiddenCategories;
    TArray<TWeakPtr<FPruneLayoutContext>> LayoutContexts;
};
