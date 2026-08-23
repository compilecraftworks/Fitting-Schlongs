// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors
// Modified from SOS/TNG slot-correction work in Skyrim Fitting System, 2026.

#include "SkinningHooks.h"

#include "SlotCorrection.h"

#include <xbyak/xbyak.h>

namespace {
SKSE::Trampoline g_localTrampoline{"SOS/TNG slot correction skinning"};
std::once_flag g_installOnce;

struct CallSiteBranch {
  std::uint8_t opcode{0};
  std::uintptr_t target{0};
  bool valid{false};
  bool expected{false};

  [[nodiscard]] bool ChainsAsCall() const { return opcode == 0xE8; }
  [[nodiscard]] bool ChainsAsJump() const { return opcode == 0xE9; }
};

[[nodiscard]] std::uintptr_t RelocationAddress(
    const REL::ID a_seId, const REL::ID a_aeId,
    const std::uintptr_t a_seOffset, const std::uintptr_t a_aeOffset) {
  if (REL::Module::IsAE()) {
    return a_aeId.address() + a_aeOffset;
  }
  return a_seId.address() + a_seOffset;
}

[[nodiscard]] CallSiteBranch InspectCallSite(
    const std::uintptr_t a_hookAddress, const std::uintptr_t a_expectedTarget,
    std::string_view a_label) {
  const auto opcode = *reinterpret_cast<const std::uint8_t *>(a_hookAddress);
  if (opcode != 0xE8 && opcode != 0xE9) {
    logger::warn(
        "SOS/TNG slot correction hook '{}' found unexpected opcode {:02X} at {:X}",
        a_label, opcode, a_hookAddress);
    return {.opcode = opcode};
  }

  const auto displacement =
      *reinterpret_cast<const std::int32_t *>(a_hookAddress + 1);
  const auto target = a_hookAddress + 5 + displacement;
  if (opcode == 0xE8 && target == a_expectedTarget) {
    return {.opcode = opcode, .target = target, .valid = true, .expected = true};
  }

  logger::warn(
      "SOS/TNG slot correction hook '{}' call site appears pre-patched: opcode {:02X}, target {:X}, expected {:X}",
      a_label, opcode, target, a_expectedTarget);
  return {.opcode = opcode, .target = target, .valid = true};
}

bool InstallDontVanillaSkinHook() {
  auto &branchTrampoline = SKSE::GetTrampoline();

  const auto hookAddress =
      RelocationAddress(REL::ID(24232), REL::ID(24736), 0x302, 0x302);
  static REL::Relocation<std::uintptr_t> applyArmorAddon{
      RELOCATION_ID(17392, 17792)};
  const auto callSite =
      InspectCallSite(hookAddress, applyArmorAddon.address(), "vanilla block");
  if (!callSite.valid) {
    logger::warn(
        "Skipped SOS/TNG slot correction vanilla block hook");
    return false;
  }
  if (!callSite.expected) {
    logger::warn(
        "SOS/TNG slot correction vanilla block hook will chain the existing patched target {:X}",
        callSite.target);
  }

  struct Code : Xbyak::CodeGenerator {
    Code(std::uintptr_t a_resumeAddress, std::uintptr_t a_nextTarget,
         bool a_chainAsJump) {
      Xbyak::Label out;
      Xbyak::Label fNextTarget;
      Xbyak::Label fShouldBlockVanillaArmor;

      push(rcx);
      push(rdx);
      push(r9);
      push(r8);
      mov(rdx, r13);
      sub(rsp, 0x40);
      call(ptr[rip + fShouldBlockVanillaArmor]);
      add(rsp, 0x40);
      pop(r8);
      pop(r9);
      pop(rdx);
      pop(rcx);
      test(al, al);
      jnz(out);
      if (a_chainAsJump) {
        jmp(ptr[rip + fNextTarget]);
      } else {
        call(ptr[rip + fNextTarget]);
      }

      L(out);
      jmp(ptr[rip]);
      dq(a_resumeAddress);

      L(fNextTarget);
      dq(a_nextTarget);

      L(fShouldBlockVanillaArmor);
      dq(reinterpret_cast<std::uintptr_t>(stsc::ShouldBlockVanillaArmor));
    }
  };

  Code code{hookAddress + 0x5, callSite.target, callSite.ChainsAsJump()};
  auto *stub = g_localTrampoline.allocate(code);
  branchTrampoline.write_branch<5>(hookAddress, stub);
  logger::info("Installed SOS/TNG slot correction vanilla block hook");
  return true;
}

void InstallShimWornFlagsHookSE() {
  auto &branchTrampoline = SKSE::GetTrampoline();

  const auto hookAddress = REL::ID(24220).address() + 0x7C;
  static REL::Relocation<std::uintptr_t> getWornMask{RELOCATION_ID(15806,
                                                                   16044)};
  const auto callSite =
      InspectCallSite(hookAddress, getWornMask.address(), "SE worn mask");
  if (!callSite.valid || !callSite.ChainsAsCall()) {
    logger::warn("Skipped SOS/TNG slot correction worn-mask hook for SE");
    return;
  }

  struct Code : Xbyak::CodeGenerator {
    Code(std::uintptr_t a_resumeAddress, std::uintptr_t a_getWornMask) {
      Xbyak::Label suppressVanilla;
      Xbyak::Label out;
      Xbyak::Label fShouldOverrideSkinning;
      Xbyak::Label fGetWornMask;
      Xbyak::Label fGetCorrectedWornMask;

      push(rcx);
      mov(rcx, rsi);
      sub(rsp, 0x8);
      sub(rsp, 0x20);
      call(ptr[rip + fShouldOverrideSkinning]);
      add(rsp, 0x20);
      add(rsp, 0x8);
      pop(rcx);
      test(al, al);
      jnz(suppressVanilla);
      call(ptr[rip + fGetWornMask]);
      jmp(out);

      L(suppressVanilla);
      sub(rsp, 0x30);
      mov(ptr[rsp + 0x20], rcx);
      mov(ptr[rsp + 0x28], rdx);
      call(ptr[rip + fGetWornMask]);
      mov(r8d, eax);
      mov(rcx, ptr[rsp + 0x20]);
      mov(rdx, rsi);
      call(ptr[rip + fGetCorrectedWornMask]);
      mov(rdx, ptr[rsp + 0x28]);
      add(rsp, 0x30);

      L(out);
      jmp(ptr[rip]);
      dq(a_resumeAddress);

      L(fShouldOverrideSkinning);
      dq(reinterpret_cast<std::uintptr_t>(stsc::ShouldOverrideSkinning));

      L(fGetWornMask);
      dq(a_getWornMask);

      L(fGetCorrectedWornMask);
      dq(reinterpret_cast<std::uintptr_t>(stsc::GetCorrectedWornMask));
    }
  };

  Code code{hookAddress + 0x5, callSite.target};
  auto *stub = g_localTrampoline.allocate(code);
  branchTrampoline.write_branch<5>(hookAddress, stub);
  logger::info("Installed SOS/TNG slot correction worn-mask hook for SE");
}

void InstallShimWornFlagsHookAE() {
  auto &branchTrampoline = SKSE::GetTrampoline();

  const auto hookAddress = REL::ID(24724).address() + 0x80;
  static REL::Relocation<std::uintptr_t> getWornMask{RELOCATION_ID(15806,
                                                                   16044)};
  const auto callSite =
      InspectCallSite(hookAddress, getWornMask.address(), "AE worn mask");
  if (!callSite.valid || !callSite.ChainsAsCall()) {
    logger::warn("Skipped SOS/TNG slot correction worn-mask hook for AE");
    return;
  }

  struct Code : Xbyak::CodeGenerator {
    Code(std::uintptr_t a_resumeAddress, std::uintptr_t a_getWornMask) {
      Xbyak::Label suppressVanilla;
      Xbyak::Label out;
      Xbyak::Label fShouldOverrideSkinning;
      Xbyak::Label fGetWornMask;
      Xbyak::Label fGetCorrectedWornMask;

      push(rcx);
      mov(rcx, rbx);
      sub(rsp, 0x8);
      sub(rsp, 0x20);
      call(ptr[rip + fShouldOverrideSkinning]);
      add(rsp, 0x20);
      add(rsp, 0x8);
      pop(rcx);
      test(al, al);
      jnz(suppressVanilla);
      call(ptr[rip + fGetWornMask]);
      jmp(out);

      L(suppressVanilla);
      sub(rsp, 0x30);
      mov(ptr[rsp + 0x20], rcx);
      mov(ptr[rsp + 0x28], rdx);
      call(ptr[rip + fGetWornMask]);
      mov(r8d, eax);
      mov(rcx, ptr[rsp + 0x20]);
      mov(rdx, rbx);
      call(ptr[rip + fGetCorrectedWornMask]);
      mov(rdx, ptr[rsp + 0x28]);
      add(rsp, 0x30);

      L(out);
      jmp(ptr[rip]);
      dq(a_resumeAddress);

      L(fShouldOverrideSkinning);
      dq(reinterpret_cast<std::uintptr_t>(stsc::ShouldOverrideSkinning));

      L(fGetWornMask);
      dq(a_getWornMask);

      L(fGetCorrectedWornMask);
      dq(reinterpret_cast<std::uintptr_t>(stsc::GetCorrectedWornMask));
    }
  };

  Code code{hookAddress + 0x5, callSite.target};
  auto *stub = g_localTrampoline.allocate(code);
  branchTrampoline.write_branch<5>(hookAddress, stub);
  logger::info("Installed SOS/TNG slot correction worn-mask hook for AE");
}

void InstallCustomSkinHookSE() {
  auto &branchTrampoline = SKSE::GetTrampoline();

  const auto hookAddress = REL::ID(24231).address() + 0x81;
  static REL::Relocation<std::uintptr_t> visitWornItems{RELOCATION_ID(15856,
                                                                     16096)};
  const auto callSite =
      InspectCallSite(hookAddress, visitWornItems.address(), "SE custom skin");
  if (!callSite.valid || !callSite.ChainsAsCall()) {
    logger::warn("Skipped SOS/TNG slot correction custom skin hook for SE");
    return;
  }

  struct Code : Xbyak::CodeGenerator {
    Code(std::uintptr_t a_resumeAddress, std::uintptr_t a_visitWornItems) {
      Xbyak::Label skipAdditional;
      Xbyak::Label fApplyAdditionalDisplayArmors;
      Xbyak::Label fVisitWornItems;
      Xbyak::Label fVisitWornItemsWithGenitalFilter;
      Xbyak::Label fShouldOverrideSkinning;

      mov(r8, rbx);
      mov(r9, ptr[rip + fVisitWornItems]);
      sub(rsp, 0x20);
      call(ptr[rip + fVisitWornItemsWithGenitalFilter]);
      add(rsp, 0x20);

      push(rcx);
      push(rdx);
      mov(rcx, rbx);
      sub(rsp, 0x20);
      call(ptr[rip + fShouldOverrideSkinning]);
      add(rsp, 0x20);
      pop(rdx);
      pop(rcx);
      test(al, al);
      jz(skipAdditional);

      push(rdx);
      push(rcx);
      mov(rcx, rbx);
      mov(rdx, rdi);
      sub(rsp, 0x20);
      call(ptr[rip + fApplyAdditionalDisplayArmors]);
      add(rsp, 0x20);
      pop(rcx);
      pop(rdx);

      L(skipAdditional);
      jmp(ptr[rip]);
      dq(a_resumeAddress);

      L(fApplyAdditionalDisplayArmors);
      dq(reinterpret_cast<std::uintptr_t>(stsc::ApplyAdditionalDisplayArmors));

      L(fVisitWornItems);
      dq(a_visitWornItems);

      L(fVisitWornItemsWithGenitalFilter);
      dq(reinterpret_cast<std::uintptr_t>(
          stsc::VisitWornItemsWithGenitalFilter));

      L(fShouldOverrideSkinning);
      dq(reinterpret_cast<std::uintptr_t>(stsc::ShouldOverrideSkinning));
    }
  };

  Code code{hookAddress + 0x5, callSite.target};
  auto *stub = g_localTrampoline.allocate(code);
  branchTrampoline.write_branch<5>(hookAddress, stub);
  logger::info("Installed SOS/TNG slot correction custom skin hook for SE");
}

void InstallCustomSkinHookAE() {
  auto &branchTrampoline = SKSE::GetTrampoline();

  const auto hookAddress = REL::ID(24725).address() + 0x1EF;
  static REL::Relocation<std::uintptr_t> visitWornItems{RELOCATION_ID(15856,
                                                                     16096)};
  const auto callSite =
      InspectCallSite(hookAddress, visitWornItems.address(), "AE custom skin");
  if (!callSite.valid || !callSite.ChainsAsCall()) {
    logger::warn("Skipped SOS/TNG slot correction custom skin hook for AE");
    return;
  }

  struct Code : Xbyak::CodeGenerator {
    Code(std::uintptr_t a_resumeAddress, std::uintptr_t a_visitWornItems) {
      Xbyak::Label skipAdditional;
      Xbyak::Label fApplyAdditionalDisplayArmors;
      Xbyak::Label fVisitWornItems;
      Xbyak::Label fVisitWornItemsWithGenitalFilter;
      Xbyak::Label fShouldOverrideSkinning;

      mov(r8, rbx);
      mov(r9, ptr[rip + fVisitWornItems]);
      sub(rsp, 0x20);
      call(ptr[rip + fVisitWornItemsWithGenitalFilter]);
      add(rsp, 0x20);

      push(rcx);
      push(rdx);
      mov(rcx, rbx);
      sub(rsp, 0x20);
      call(ptr[rip + fShouldOverrideSkinning]);
      add(rsp, 0x20);
      pop(rdx);
      pop(rcx);
      test(al, al);
      jz(skipAdditional);

      push(rdx);
      push(rcx);
      mov(rcx, rbx);
      mov(rdx, r15);
      sub(rsp, 0x20);
      call(ptr[rip + fApplyAdditionalDisplayArmors]);
      add(rsp, 0x20);
      pop(rcx);
      pop(rdx);

      L(skipAdditional);
      jmp(ptr[rip]);
      dq(a_resumeAddress);

      L(fApplyAdditionalDisplayArmors);
      dq(reinterpret_cast<std::uintptr_t>(stsc::ApplyAdditionalDisplayArmors));

      L(fVisitWornItems);
      dq(a_visitWornItems);

      L(fVisitWornItemsWithGenitalFilter);
      dq(reinterpret_cast<std::uintptr_t>(
          stsc::VisitWornItemsWithGenitalFilter));

      L(fShouldOverrideSkinning);
      dq(reinterpret_cast<std::uintptr_t>(stsc::ShouldOverrideSkinning));
    }
  };

  Code code{hookAddress + 0x5, callSite.target};
  auto *stub = g_localTrampoline.allocate(code);
  branchTrampoline.write_branch<5>(hookAddress, stub);
  logger::info("Installed SOS/TNG slot correction custom skin hook for AE");
}
} // namespace

namespace stsc {
void InstallSkinningHooks() {
  std::call_once(g_installOnce, [] {
    if (REL::Module::IsVR()) {
      logger::warn("SOS/TNG slot correction hooks are disabled on VR");
      return;
    }

    if (g_localTrampoline.empty()) {
      g_localTrampoline.create(64 * 1024);
    }

    InstallDontVanillaSkinHook();
    if (REL::Module::IsAE()) {
      InstallShimWornFlagsHookAE();
      InstallCustomSkinHookAE();
    } else {
      InstallShimWornFlagsHookSE();
      InstallCustomSkinHookSE();
    }
  });
}
} // namespace stsc
