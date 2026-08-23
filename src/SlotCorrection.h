// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors

#pragma once

#include <cstdint>

namespace RE {
class ActorWeightModel;
}

namespace stsc {
void SetGameDataLoaded(bool a_loaded);
[[nodiscard]] bool IsGameDataLoaded();

void ApplyRuntimeArmorCorrections();
void RegisterEquipmentEventSink();
void InvalidateQueuedRefreshes();
void QueueRefreshFor(RE::Actor *a_actor);
void QueuePlayerRefresh();
void RefreshFor(RE::Actor *a_actor);

[[nodiscard]] bool ShouldOverrideSkinning(RE::TESObjectREFR *a_target);
[[nodiscard]] bool ShouldBlockVanillaArmor(RE::TESObjectARMO *a_armor,
                                           RE::TESObjectREFR *a_target);
[[nodiscard]] std::uint32_t
GetCorrectedWornMask(RE::InventoryChanges *a_inventory,
                     RE::TESObjectREFR *a_target,
                     std::uint32_t a_baseWornMask);
void VisitWornItemsWithGenitalFilter(
    RE::InventoryChanges *a_inventory,
    RE::InventoryChanges::IItemChangeVisitor *a_visitor,
    RE::TESObjectREFR *a_target, std::uintptr_t a_visitWornItems);
void ApplyAdditionalDisplayArmors(RE::Actor *a_actor,
                                  RE::ActorWeightModel *a_actorWeightModel);
} // namespace stsc
