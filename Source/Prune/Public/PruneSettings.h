// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PruneSettings.generated.h"

UCLASS(Config = Prune, DefaultConfig, meta = (DisplayName = "Prune"))
class PRUNE_API UPruneSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPruneSettings();

    virtual FName GetSectionName() const override { return TEXT("Prune"); }
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

    UPROPERTY(EditAnywhere, Config, Category = "Filter Editor|Reordering", meta = (DisplayName = "Reorder Drag Color", ToolTip = "Background color used on the category row currently being dragged in the Prune filter editor."))
    FLinearColor ReorderDragColor;

    UPROPERTY(EditAnywhere, Config, Category = "Filter Editor|Reordering", meta = (DisplayName = "Dragged Content Color", ToolTip = "Text, checkbox, and reorder-grip color used on the category row currently being dragged."))
    FLinearColor ReorderDraggedContentColor;

    UPROPERTY(EditAnywhere, Config, Category = "Filter Editor|Reordering", meta = (DisplayName = "Drop Line Color", ToolTip = "Color of the insertion line that previews where a reordered category will be placed."))
    FLinearColor ReorderDropLineColor;

    UPROPERTY(EditAnywhere, Config, Category = "Filter Editor|Reordering", meta = (DisplayName = "Drop Line Thickness", ClampMin = "1.0", ClampMax = "8.0", UIMin = "1.0", UIMax = "5.0", ToolTip = "Thickness in Slate units of the category reorder insertion line."))
    float ReorderDropLineThickness;

    UPROPERTY(EditAnywhere, Config, Category = "Filter Editor|Window", meta = (DisplayName = "Default Width", ClampMin = "420.0", ClampMax = "1600.0", UIMin = "480.0", UIMax = "1000.0", ToolTip = "Default width of newly opened New Filter and Edit Filter windows. The window remains freely resizable."))
    float FilterEditorDefaultWidth;

    UPROPERTY(EditAnywhere, Config, Category = "Filter Editor|Window", meta = (DisplayName = "Default Height", ClampMin = "420.0", ClampMax = "1600.0", UIMin = "480.0", UIMax = "1000.0", ToolTip = "Default height of newly opened New Filter and Edit Filter windows. The window remains freely resizable."))
    float FilterEditorDefaultHeight;

    UPROPERTY(EditAnywhere, Config, Category = "Filter Bar", meta = (DisplayName = "Show Description Tooltips", ToolTip = "When enabled, Prune descriptions are shown when hovering their filter buttons in Actor Details."))
    bool bShowFilterDescriptionTooltips;

    UPROPERTY(EditAnywhere, Config, Category = "Filter Manager|Window", meta = (DisplayName = "Default Width", ClampMin = "480.0", ClampMax = "1600.0", UIMin = "560.0", UIMax = "1100.0", ToolTip = "Default width of the Prune Filter Manager window. The window remains freely resizable."))
    float FilterManagerDefaultWidth;

    UPROPERTY(EditAnywhere, Config, Category = "Filter Manager|Window", meta = (DisplayName = "Default Height", ClampMin = "420.0", ClampMax = "1600.0", UIMin = "480.0", UIMax = "1000.0", ToolTip = "Default height of the Prune Filter Manager window. The window remains freely resizable."))
    float FilterManagerDefaultHeight;

    static const UPruneSettings* Get();
};
