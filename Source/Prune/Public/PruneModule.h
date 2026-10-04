// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#pragma once

#include "Delegates/Delegate.h"
#include "Modules/ModuleInterface.h"
#include "Templates/SharedPointer.h"

class FPruneState;

/**
 * Prune extends Unreal's stock Actor Details customization through the public
 * OnExtendActorDetails hook exposed by the DetailCustomizations module.
 */
class FPruneModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // Live Details views can retain Slate callbacks into this module.
    virtual bool SupportsDynamicReloading() override
    {
        return false;
    }

private:
    TSharedPtr<FPruneState> State;
    FDelegateHandle ActorDetailsExtensionHandle;
};
