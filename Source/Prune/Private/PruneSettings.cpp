// Copyright Mippithedork 2026, Inc. All Rights Reserved.

#include "PruneSettings.h"

UPruneSettings::UPruneSettings()
    : ReorderDragColor(FLinearColor(0.0f, 0.162029f, 0.745404f, 1.0f))
    , ReorderDraggedContentColor(FLinearColor::White)
    , ReorderDropLineColor(FLinearColor(0.0f, 0.162029f, 0.745404f, 1.0f))
    , ReorderDropLineThickness(2.0f)
    , FilterEditorDefaultWidth(560.0f)
    , FilterEditorDefaultHeight(640.0f)
    , bShowFilterDescriptionTooltips(true)
    , FilterManagerDefaultWidth(680.0f)
    , FilterManagerDefaultHeight(620.0f)
{
}

const UPruneSettings* UPruneSettings::Get()
{
    return GetDefault<UPruneSettings>();
}
