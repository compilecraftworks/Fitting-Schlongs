// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors

#include "Plugin.h"
#include "RuntimeSupport.h"
#include "SkinningHooks.h"
#include "SlotCorrection.h"

namespace {
bool g_hooksActive{false};

[[nodiscard]] bool IsSkyrimFittingSystemLoaded() {
  return GetModuleHandleA("SkyrimFittingSystem.dll") != nullptr;
}

void MessageHandler(SKSE::MessagingInterface::Message *a_message) {
  if (!a_message) {
    return;
  }

  switch (a_message->type) {
  case SKSE::MessagingInterface::kPostPostLoad:
    if (IsSkyrimFittingSystemLoaded()) {
      logger::warn(
          "SkyrimFittingSystem.dll is loaded; Fitting Schlongs will stay inactive to avoid duplicate skinning hooks");
      return;
    }
    g_hooksActive = stsc::InstallSkinningHooks();
    if (!g_hooksActive) {
      logger::critical("Fitting Schlongs hooks were not installed; the plugin will remain inactive");
    }
    break;
  case SKSE::MessagingInterface::kDataLoaded:
    if (IsSkyrimFittingSystemLoaded() || !g_hooksActive) {
      return;
    }
    stsc::SetGameDataLoaded(true);
    stsc::ApplyRuntimeArmorCorrections();
    stsc::RegisterEquipmentEventSink();
    stsc::QueuePlayerRefresh();
    break;
  case SKSE::MessagingInterface::kPreLoadGame:
    stsc::SetGameDataLoaded(false);
    stsc::InvalidateQueuedRefreshes();
    break;
  case SKSE::MessagingInterface::kPostLoadGame:
    if (IsSkyrimFittingSystemLoaded() || !g_hooksActive) {
      return;
    }
    stsc::SetGameDataLoaded(true);
    stsc::ApplyRuntimeArmorCorrections();
    stsc::InvalidateQueuedRefreshes();
    stsc::QueuePlayerRefresh();
    break;
  default:
    break;
  }
}
} // namespace

extern "C" DLLEXPORT bool SKSEAPI
SKSEPlugin_Load(const SKSE::LoadInterface *a_skse) {
  REL::Module::reset();
  SKSE::Init(a_skse);

  const auto runtime = stsc::CurrentGameVersion();
  if (!stsc::IsSupportedRuntime(runtime)) {
    logger::critical(
        "Unsupported Skyrim runtime {}.{}.{}.{}; supported runtimes are 1.5.97.0 and 1.6.1170.0",
        runtime.major, runtime.minor, runtime.patch, runtime.build);
    return false;
  }

  SKSE::AllocTrampoline(1 << 12);

  logger::info("{} build {}", stsc::plugin::NAME,
               stsc::plugin::VERSION_STRING);

  auto *messaging = SKSE::GetMessagingInterface();
  if (!messaging) {
    logger::critical("Failed to load messaging interface");
    return false;
  }

  messaging->RegisterListener("SKSE", MessageHandler);
  logger::info("{} loaded", stsc::plugin::NAME);
  return true;
}
