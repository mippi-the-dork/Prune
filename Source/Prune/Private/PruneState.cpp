// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneState.h"

#include "DetailCategoryBuilder.h"

DEFINE_LOG_CATEGORY_STATIC(LogPrune, Log, All);

void FPruneLayoutContext::UpdateFromFinalCategories(
    const TMap<FName, IDetailCategoryBuilder*>& InCategories,
    FName ExcludedCategoryId)
{
    Categories.Reset();
    CategoryBuilders.Reset();

    Categories.Reserve(InCategories.Num());
    CategoryBuilders.Reserve(InCategories.Num());

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

        FPruneCategoryInfo& Info = Categories.AddDefaulted_GetRef();
        Info.Id = CategoryId;
        Info.DisplayName = Category->GetDisplayName().IsEmpty()
            ? FText::FromName(CategoryId)
            : Category->GetDisplayName();
        CategoryBuilders.Add(CategoryId, Category);
    }

    // The menu is alphabetical on purpose. Other category-sort callbacks may
    // still run after Prune's discovery callback and refine the panel's visual
    // ordering. Prune never reads or rewrites that order.
    Categories.Sort(
        [](const FPruneCategoryInfo& A, const FPruneCategoryInfo& B)
        {
            return A.DisplayName.ToString() < B.DisplayName.ToString();
        });
}

TArray<FPruneCategoryInfo> FPruneLayoutContext::GetCategories() const
{
    return Categories;
}

int32 FPruneLayoutContext::GetHiddenCurrentCategoryCount(
    const FPruneState& State) const
{
    int32 Count = 0;

    for (const FPruneCategoryInfo& Category : Categories)
    {
        if (!State.IsCategoryVisible(Category.Id))
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

void FPruneLayoutContext::ApplyHiddenState(const FPruneState& State)
{
    for (const TPair<FName, IDetailCategoryBuilder*>& Entry : CategoryBuilders)
    {
        if (Entry.Value != nullptr)
        {
            Entry.Value->SetCategoryVisibility(
                State.IsCategoryVisible(Entry.Key));
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
        TEXT("Prune current finished category set: %d categories"),
        Categories.Num());

    for (const FPruneCategoryInfo& Category : Categories)
    {
        UE_LOG(
            LogPrune,
            Log,
            TEXT("  %s | Display: %s | Visible: %s"),
            *Category.Id.ToString(),
            *Category.DisplayName.ToString(),
            State.IsCategoryVisible(Category.Id) ? TEXT("Yes") : TEXT("No"));
    }
}

bool FPruneState::IsCategoryVisible(FName CategoryId) const
{
    return !HiddenCategories.Contains(CategoryId);
}

void FPruneState::SetCategoryVisible(FName CategoryId, bool bVisible)
{
    if (CategoryId.IsNone())
    {
        return;
    }

    if (bVisible)
    {
        HiddenCategories.Remove(CategoryId);
    }
    else
    {
        HiddenCategories.Add(CategoryId);
    }

    CompactContexts();

    for (const TWeakPtr<FPruneLayoutContext>& WeakContext : LayoutContexts)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = WeakContext.Pin())
        {
            Context->ApplyCategoryVisibility(CategoryId, bVisible);
        }
    }
}

void FPruneState::ShowAllCategories()
{
    if (HiddenCategories.IsEmpty())
    {
        return;
    }

    HiddenCategories.Reset();
    CompactContexts();

    for (const TWeakPtr<FPruneLayoutContext>& WeakContext : LayoutContexts)
    {
        if (const TSharedPtr<FPruneLayoutContext> Context = WeakContext.Pin())
        {
            Context->ShowAllCurrentCategories();
        }
    }
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

void FPruneState::CompactContexts()
{
    LayoutContexts.RemoveAll(
        [](const TWeakPtr<FPruneLayoutContext>& WeakContext)
        {
            return !WeakContext.IsValid();
        });
}
