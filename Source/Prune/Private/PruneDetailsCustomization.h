// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtrTemplates.h"

class AActor;
class FPruneLayoutContext;
class FPruneState;
class IDetailLayoutBuilder;

/** Stateless helper used by Prune's OnExtendActorDetails delegate. */
class FPruneDetailsCustomization final
{
public:
    static void ExtendActorDetails(
        TSharedRef<FPruneState> State,
        IDetailLayoutBuilder& DetailBuilder,
        const TArray<TWeakObjectPtr<AActor>>& SelectedActors);

};
