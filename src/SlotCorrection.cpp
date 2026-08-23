// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors
// Modified from SOS/TNG slot-correction work in Skyrim Fitting System, 2026.

#include "SlotCorrection.h"

#include <thread>

namespace RE {
InventoryChanges::IItemChangeVisitor::~IItemChangeVisitor() = default;
} // namespace RE

namespace {
struct CorrectionSet {
  bool active{false};
  bool revealGenitals{false};
  bool concealGenitals{false};
  std::unordered_set<RE::FormID> hiddenArmorFormIDs;
};

std::atomic_bool g_gameDataLoaded{false};
std::atomic_uint64_t g_refreshGeneration{0};
std::mutex g_actorRefreshMutex;
std::unordered_map<RE::FormID, std::uint64_t> g_actorRefreshGeneration;

class WornArmorVisitor final : public RE::InventoryChanges::IItemChangeVisitor {
public:
  RE::BSContainer::ForEachResult
  Visit(RE::InventoryEntryData *a_entryData) override {
    if (a_entryData && a_entryData->object) {
      if (auto *armor = a_entryData->object->As<RE::TESObjectARMO>()) {
        armors.insert(armor);
      }
    }
    return RE::BSContainer::ForEachResult::kContinue;
  }

  std::unordered_set<const RE::TESObjectARMO *> armors;
};

[[nodiscard]] bool IsActorRefreshable(RE::Actor *a_actor) {
  return a_actor && !a_actor->IsDeleted() && !a_actor->IsDisabled() &&
         a_actor->Is3DLoaded();
}

[[nodiscard]] std::uint64_t
QueueActorRefreshGeneration(const RE::FormID a_actorFormID) {
  std::lock_guard lock(g_actorRefreshMutex);
  return ++g_actorRefreshGeneration[a_actorFormID];
}

[[nodiscard]] bool
IsLatestActorRefreshGeneration(const RE::FormID a_actorFormID,
                               const std::uint64_t a_generation) {
  std::lock_guard lock(g_actorRefreshMutex);
  const auto it = g_actorRefreshGeneration.find(a_actorFormID);
  return it != g_actorRefreshGeneration.end() && it->second == a_generation;
}

void ClearQueuedActorRefreshes() {
  std::lock_guard lock(g_actorRefreshMutex);
  g_actorRefreshGeneration.clear();
}

[[nodiscard]] std::uint32_t BodySlotMask() {
  return static_cast<std::uint32_t>(
      std::to_underlying(RE::BGSBipedObjectForm::BipedObjectSlot::kBody));
}

[[nodiscard]] std::uint32_t GenitalSlotMask() {
  return static_cast<std::uint32_t>(std::to_underlying(
      RE::BGSBipedObjectForm::BipedObjectSlot::kModPelvisSecondary));
}

[[nodiscard]] std::uint32_t PelvisPrimarySlotMask() {
  return static_cast<std::uint32_t>(std::to_underlying(
      RE::BGSBipedObjectForm::BipedObjectSlot::kModPelvisPrimary));
}

[[nodiscard]] std::string CopyCString(const char *a_text) {
  if (!a_text || a_text[0] == '\0') {
    return {};
  }
  return a_text;
}

using GetPo3EditorIDFn = const char *(*)(std::uint32_t);

[[nodiscard]] std::string GetPo3EditorID(const RE::FormID a_formID) {
  static auto *module = [] {
    if (auto *loaded = GetModuleHandleA("po3_Tweaks.dll")) {
      return loaded;
    }
    return GetModuleHandleA("po3_Tweaks");
  }();
  static auto *function = reinterpret_cast<GetPo3EditorIDFn>(
      module ? GetProcAddress(module, "GetFormEditorID") : nullptr);
  return function ? CopyCString(function(a_formID)) : std::string{};
}

[[nodiscard]] std::string GetEditorID(const RE::TESForm *a_form) {
  if (!a_form) {
    return {};
  }

  auto editorID = CopyCString(a_form->GetFormEditorID());
  if (editorID.empty()) {
    editorID = GetPo3EditorID(a_form->GetFormID());
  }
  return editorID;
}

[[nodiscard]] std::string GetDisplayName(const RE::TESForm *a_form) {
  if (!a_form) {
    return {};
  }
  auto name = CopyCString(a_form->GetName());
  if (!name.empty()) {
    return name;
  }
  return GetEditorID(a_form);
}

[[nodiscard]] std::string GetPluginName(const RE::TESForm *a_form) {
  if (!a_form) {
    return {};
  }
  const auto *file = a_form->GetFile(0);
  return file ? std::string(file->GetFilename()) : std::string{};
}

[[nodiscard]] std::string ToLower(std::string a_value) {
  std::ranges::transform(a_value, a_value.begin(), [](const unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return a_value;
}

[[nodiscard]] bool IsAsciiAlpha(const unsigned char a_ch) {
  return (a_ch >= 'A' && a_ch <= 'Z') || (a_ch >= 'a' && a_ch <= 'z');
}

[[nodiscard]] bool IsAsciiUpper(const unsigned char a_ch) {
  return a_ch >= 'A' && a_ch <= 'Z';
}

[[nodiscard]] bool IsAsciiLower(const unsigned char a_ch) {
  return a_ch >= 'a' && a_ch <= 'z';
}

[[nodiscard]] bool IsAsciiDigit(const unsigned char a_ch) {
  return a_ch >= '0' && a_ch <= '9';
}

[[nodiscard]] bool IsAsciiAlphaNumeric(const unsigned char a_ch) {
  return IsAsciiAlpha(a_ch) || IsAsciiDigit(a_ch);
}

[[nodiscard]] std::string BuildSearchTextPart(const std::string_view a_value) {
  std::string output;
  output.reserve(a_value.size() * 2);

  unsigned char previous = 0;
  for (std::size_t index = 0; index < a_value.size(); ++index) {
    const auto ch = static_cast<unsigned char>(a_value[index]);
    const auto next =
        index + 1 < a_value.size()
            ? static_cast<unsigned char>(a_value[index + 1])
            : static_cast<unsigned char>(0);

    const bool splitCamelCase =
        IsAsciiUpper(ch) &&
        ((IsAsciiLower(previous) || IsAsciiDigit(previous)) ||
         (IsAsciiUpper(previous) && IsAsciiLower(next)));
    const bool splitAlphaDigit =
        IsAsciiAlphaNumeric(ch) && IsAsciiAlphaNumeric(previous) &&
        (IsAsciiDigit(ch) != IsAsciiDigit(previous));
    if (!output.empty() && (splitCamelCase || splitAlphaDigit)) {
      output.push_back(' ');
    }

    if (IsAsciiAlphaNumeric(ch)) {
      output.push_back(static_cast<char>(std::tolower(ch)));
    } else if (ch >= 0x80) {
      output.push_back(static_cast<char>(ch));
    } else if (!output.empty() && output.back() != ' ') {
      output.push_back(' ');
    }
    previous = ch;
  }

  if (!output.empty() && output.back() == ' ') {
    output.pop_back();
  }
  return output;
}

void AppendSearchText(std::string &a_output, const std::string &a_value) {
  const auto text = BuildSearchTextPart(a_value);
  if (text.empty()) {
    return;
  }
  if (!a_output.empty()) {
    a_output.push_back(' ');
  }
  a_output.append(text);
}

[[nodiscard]] bool ContainsSearchTerm(const std::string &a_text,
                                      const std::string_view a_term) {
  if (a_text.empty() || a_term.empty()) {
    return false;
  }

  const auto containsNonAscii = [](const std::string_view a_value) {
    return std::ranges::any_of(a_value, [](const unsigned char ch) {
      return ch >= 0x80;
    });
  };

  if (containsNonAscii(a_term)) {
    return a_text.find(a_term) != std::string::npos;
  }

  std::string paddedText;
  paddedText.reserve(a_text.size() + 2);
  paddedText.push_back(' ');
  paddedText.append(a_text);
  paddedText.push_back(' ');

  std::string paddedTerm;
  paddedTerm.reserve(a_term.size() + 2);
  paddedTerm.push_back(' ');
  paddedTerm.append(a_term);
  paddedTerm.push_back(' ');
  if (paddedText.find(paddedTerm) != std::string::npos) {
    return true;
  }

  if (a_term.size() < 4) {
    return false;
  }

  std::string prefixTerm;
  prefixTerm.reserve(a_term.size() + 1);
  prefixTerm.push_back(' ');
  prefixTerm.append(a_term);
  if (paddedText.find(prefixTerm) != std::string::npos) {
    return true;
  }

  static constexpr std::array kEmbeddedTerms{
      std::string_view{"bikini"},   std::string_view{"lingerie"},
      std::string_view{"monokini"}, std::string_view{"pantie"},
      std::string_view{"panties"},  std::string_view{"pantsu"},
      std::string_view{"panty"},    std::string_view{"panrty"},
      std::string_view{"swimsuit"}, std::string_view{"swimwear"},
      std::string_view{"tankini"},  std::string_view{"thong"},
      std::string_view{"thongs"}};
  if (std::ranges::find(kEmbeddedTerms, a_term) != kEmbeddedTerms.end()) {
    return a_text.find(a_term) != std::string::npos;
  }

  return false;
}

template <std::size_t N>
[[nodiscard]] bool ContainsAnySearchTerm(
    const std::string &a_text,
    const std::array<std::string_view, N> &a_terms) {
  return std::ranges::any_of(a_terms, [&](const auto term) {
    return ContainsSearchTerm(a_text, term);
  });
}

[[nodiscard]] std::string
BuildArmorClassificationText(const RE::TESObjectARMO *a_armor) {
  std::string text;
  if (!a_armor) {
    return text;
  }

  AppendSearchText(text, GetEditorID(a_armor));
  AppendSearchText(text, GetDisplayName(a_armor));
  for (const auto *keyword : a_armor->GetKeywords()) {
    AppendSearchText(text, GetEditorID(keyword));
  }
  return text;
}

[[nodiscard]] bool IsActorFemale(RE::Actor *a_actor) {
  const auto *actorBase = a_actor ? a_actor->GetActorBase() : nullptr;
  return actorBase && actorBase->IsFemale();
}

[[nodiscard]] bool HasKeywordEditorID(const RE::TESObjectARMO *a_armor,
                                      const std::string_view a_editorID) {
  if (!a_armor || a_editorID.empty()) {
    return false;
  }

  for (const auto *keyword : a_armor->GetKeywords()) {
    if (keyword && GetEditorID(keyword) == a_editorID) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool IsFormInList(const std::string_view a_editorID,
                                const RE::TESForm *a_form) {
  if (!a_form || a_editorID.empty()) {
    return false;
  }
  const auto *formList =
      RE::TESForm::LookupByEditorID<RE::BGSListForm>(std::string(a_editorID));
  return formList && formList->HasForm(a_form);
}

[[nodiscard]] std::uint64_t
GetArmorAddonSlotMask(const RE::TESObjectARMO *a_armor) {
  if (!a_armor) {
    return 0;
  }

  std::uint64_t slotMask = 0;
  for (const auto *armorAddon : a_armor->armorAddons) {
    if (armorAddon) {
      slotMask |= armorAddon->GetSlotMask().underlying();
    }
  }
  return slotMask;
}

[[nodiscard]] std::uint64_t
GetArmorDisplaySlotMask(const RE::TESObjectARMO *a_armor) {
  if (!a_armor) {
    return 0;
  }

  const auto armorSlotMask =
      static_cast<std::uint64_t>(a_armor->GetSlotMask().underlying());
  return armorSlotMask != 0 ? armorSlotMask : GetArmorAddonSlotMask(a_armor);
}

[[nodiscard]] bool IsSosTngGenitalArmor(const RE::TESObjectARMO *a_armor);
[[nodiscard]] bool
IsLikelyUpperOnlyBodyArmor(const RE::TESObjectARMO *a_armor);

[[nodiscard]] std::uint64_t
GetArmorCorrectionSlotMask(const RE::TESObjectARMO *a_armor) {
  auto slotMask = GetArmorDisplaySlotMask(a_armor);
  if (!a_armor || slotMask == 0) {
    return slotMask;
  }

  if (!IsSosTngGenitalArmor(a_armor) &&
      (slotMask & BodySlotMask()) != 0 &&
      (slotMask & GenitalSlotMask()) != 0 &&
      IsLikelyUpperOnlyBodyArmor(a_armor)) {
    slotMask &= ~static_cast<std::uint64_t>(GenitalSlotMask());
  }
  return slotMask;
}

[[nodiscard]] std::uint32_t
GetArmorConflictSlotMask(const RE::TESObjectARMO *a_armor) {
  if (!a_armor) {
    return 0;
  }
  return static_cast<std::uint32_t>(GetArmorCorrectionSlotMask(a_armor));
}

[[nodiscard]] bool HasArmorAddons(const RE::TESObjectARMO *a_armor) {
  if (!a_armor) {
    return false;
  }
  return std::ranges::any_of(a_armor->armorAddons,
                             [](const auto *a_armorAddon) {
                               return a_armorAddon != nullptr;
                             });
}

[[nodiscard]] bool IsSosTngGenitalArmor(const RE::TESObjectARMO *a_armor) {
  if (!a_armor) {
    return false;
  }

  const auto slotMask =
      GetArmorDisplaySlotMask(a_armor) | a_armor->GetSlotMask().underlying();
  if ((slotMask & GenitalSlotMask()) == 0) {
    return false;
  }

  if (HasKeywordEditorID(a_armor, "SOS_Genitals")) {
    return true;
  }

  const auto pluginName = ToLower(GetPluginName(a_armor));
  const auto editorID = ToLower(GetEditorID(a_armor));
  const auto displayName = ToLower(GetDisplayName(a_armor));
  if (editorID.find("genitalcover") != std::string::npos ||
      displayName.find("genital cover") != std::string::npos) {
    return false;
  }

  if (editorID.find("genital") != std::string::npos ||
      editorID.find("schlong") != std::string::npos ||
      displayName.find("genital") != std::string::npos ||
      displayName.find("schlong") != std::string::npos) {
    return true;
  }

  if (pluginName.find("schlongs of skyrim") != std::string::npos &&
      (editorID.find("genital") != std::string::npos ||
       editorID.find("schlong") != std::string::npos ||
       displayName.find("schlong") != std::string::npos)) {
    return true;
  }

  return pluginName.find("thenewgentleman") != std::string::npos &&
         editorID.starts_with("tng_genital") &&
         editorID.find("cover") == std::string::npos;
}

[[nodiscard]] bool IsGenitalRevealingArmor(RE::Actor *a_actor,
                                           const RE::TESObjectARMO *a_armor) {
  if (!a_armor) {
    return false;
  }

  const bool isFemale = IsActorFemale(a_actor);
  if (HasKeywordEditorID(a_armor, "SOS_Revealing") ||
      HasKeywordEditorID(a_armor, "TNG_Revealing") ||
      (isFemale && HasKeywordEditorID(a_armor, "TNG_RevealingOnlyWomen")) ||
      (!isFemale && HasKeywordEditorID(a_armor, "TNG_RevealingOnlyMen")) ||
      IsFormInList("SOS_RevealingArmors", a_armor)) {
    return true;
  }

  for (const auto *armorAddon : a_armor->armorAddons) {
    if (IsFormInList("SOS_RevealingArmors", armorAddon)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool IsGenitalCoveringArmor(RE::Actor *a_actor,
                                          const RE::TESObjectARMO *a_armor) {
  if (!a_armor) {
    return false;
  }

  if (HasKeywordEditorID(a_armor, "SOS_Concealing") ||
      HasKeywordEditorID(a_armor, "SOS_Underwear") ||
      HasKeywordEditorID(a_armor, "TNG_Covering") ||
      IsFormInList("SOS_ConcealingArmors", a_armor)) {
    return true;
  }

  for (const auto *armorAddon : a_armor->armorAddons) {
    if (IsFormInList("SOS_ConcealingArmors", armorAddon)) {
      return true;
    }
  }

  const bool isFemale = IsActorFemale(a_actor);
  return (isFemale &&
          HasKeywordEditorID(a_armor, "TNG_RevealingOnlyMen")) ||
         (!isFemale &&
          HasKeywordEditorID(a_armor, "TNG_RevealingOnlyWomen"));
}

[[nodiscard]] bool
IsLikelyGenitalConcealingPelvisArmor(const RE::TESObjectARMO *a_armor) {
  if (!a_armor ||
      (GetArmorDisplaySlotMask(a_armor) & PelvisPrimarySlotMask()) == 0) {
    return false;
  }

  if (HasKeywordEditorID(a_armor, "SOS_Underwear") ||
      HasKeywordEditorID(a_armor, "TNG_Underwear")) {
    return true;
  }

  static constexpr std::array kLowerBodyTerms{
      std::string_view{"bikini"},      std::string_view{"bot"},
      std::string_view{"bottom"},      std::string_view{"bottoms"},
      std::string_view{"bathing"},    std::string_view{"brief"},
      std::string_view{"briefs"},     std::string_view{"bustle"},
      std::string_view{"culotte"},    std::string_view{"culottes"},
      std::string_view{"dress"},      std::string_view{"boxer"},
      std::string_view{"boxers"},     std::string_view{"falda"},
      std::string_view{"fundoshi"},   std::string_view{"hotpants"},
      std::string_view{"inner"},
      std::string_view{"jean"},       std::string_view{"jeans"},
      std::string_view{"knicker"},
      std::string_view{"knickers"},   std::string_view{"legging"},
      std::string_view{"leggings"},   std::string_view{"lingerie"},
      std::string_view{"loincloth"},  std::string_view{"low"},
      std::string_view{"lower"},      std::string_view{"maillot"},
      std::string_view{"monokini"},   std::string_view{"one piece"},
      std::string_view{"pantie"},     std::string_view{"panties"},
      std::string_view{"pants"},      std::string_view{"pantsmp"},
      std::string_view{"pantsu"},     std::string_view{"panty"},
      std::string_view{"panrty"},     std::string_view{"pussy"},
      std::string_view{"short"},      std::string_view{"shorts"},
      std::string_view{"skirt"},      std::string_view{"string"},
      std::string_view{"strings"},    std::string_view{"swim"},
      std::string_view{"swimsuit"},   std::string_view{"swimwear"},
      std::string_view{"tankini"},    std::string_view{"thong"},
      std::string_view{"thongs"},     std::string_view{"trouser"},
      std::string_view{"trousers"},   std::string_view{"underpant"},
      std::string_view{"underpants"}, std::string_view{"underwear"},
      std::string_view{"undies"},     std::string_view{"uw"},
      std::string_view{"body suit"},  std::string_view{"bodysuit"},
      std::string_view{"cat suit"},   std::string_view{"catsuit"},
      std::string_view{"jump suit"},  std::string_view{"jumpsuit"},
      std::string_view{"leotard"},    std::string_view{"unitard"},
      std::string_view{"yoga"},
      std::string_view{"\xEB\xA0\x88\xEC\x98\xA4\xED\x83\x80\xEB\x93\x9C"},
      std::string_view{"\xEB\xA0\x88\xEA\xB9\x85\xEC\x8A\xA4"},
      std::string_view{"\xEB\x9E\x80\xEC\xA0\x9C\xEB\xA6\xAC"},
      std::string_view{"\xEB\xB0\x94\xEB\x94\x94\xEC\x88\x98\xED\x8A\xB8"},
      std::string_view{"\xEC\x9C\xA0\xEB\x8B\x88\xED\x83\x80\xEB\x93\x9C"},
      std::string_view{"\xEC\xBA\xA3\xEC\x88\x98\xED\x8A\xB8"},
      std::string_view{"\xEB\xB0\x94\xEC\xA7\x80"},
      std::string_view{"\xEB\xB9\x84\xED\x82\xA4\xEB\x8B\x88"},
      std::string_view{"\xEB\xB3\xB4\xEC\xA7\x80"},
      std::string_view{"\xEB\xB7\xB0\xEC\xA7\x80"},
      std::string_view{"\xEB\x93\x9C\xEB\xA0\x88\xEC\x8A\xA4"},
      std::string_view{"\xEB\xAA\xA8\xEB\x85\xB8\xED\x82\xA4\xEB\x8B\x88"},
      std::string_view{"\xEC\x88\x98\xEC\x98\x81\xEB\xB3\xB5"},
      std::string_view{"\xEC\x86\x8D\xEB\xB0\x94\xEC\xA7\x80"},
      std::string_view{"\xEC\x86\x8D\xEC\x98\xB7"},
      std::string_view{"\xEC\x8A\xA4\xEC\xBB\xA4\xED\x8A\xB8"},
      std::string_view{"\xEC\x87\xBC\xEC\xB8\xA0"},
      std::string_view{"\xEC\x88\x8F\xED\x8C\xAC\xEC\xB8\xA0"},
      std::string_view{"\xEC\xA0\x90\xED\x94\x84\xEC\x88\x98\xED\x8A\xB8"},
      std::string_view{"\xEC\x8A\xA4\xED\x8A\xB8\xEB\xA7\x81"},
      std::string_view{"\xEC\x8A\xA4\xED\x8A\xB8\xEB\xA7\x81\xEC\x8A\xA4"},
      std::string_view{"\xEC\x88\x8F"},
      std::string_view{"\xEC\x9A\x94\xEA\xB0\x80"},
      std::string_view{"\xEC\x9B\x90\xED\x94\xBC\xEC\x8A\xA4"},
      std::string_view{"\xEC\xB9\x98\xEB\xA7\x88"},
      std::string_view{"\xED\x8C\xAC\xEC\xB8\xA0"},
      std::string_view{"\xED\x8C\xAC\xED\x8B\xB0"},
      std::string_view{"\xED\x95\x98\xEC\x9D\x98"},
      std::string_view{"\xE4\xB8\x8B\xE8\xA3\x85"},
      std::string_view{"\xE4\xB8\x8B\xE8\xA1\xA3"},
      std::string_view{"\xE5\x86\x85\xE8\xA3\xA4"},
      std::string_view{"\xE5\xBA\x95\xE8\xA3\xA4"},
      std::string_view{"\xE5\x86\x85\xE8\xA1\xA3\xE4\xB8\x8B"},
      std::string_view{"\xE5\x86\x85\xE8\xA1\xA3 \xE4\xB8\x8B"},
      std::string_view{"\xE8\xBF\x9E\xE4\xBD\x93\xE8\xA1\xA3"},
      std::string_view{"\xE8\xBF\x9E\xE4\xBD\x93\xE7\xB4\xA7\xE8\xBA\xAB\xE8\xA1\xA3"},
      std::string_view{"\xE7\xB4\xA7\xE8\xBA\xAB\xE8\xBF\x9E\xE4\xBD\x93\xE8\xA1\xA3"},
      std::string_view{"\xE5\x85\xA8\xE8\xBA\xAB\xE7\xB4\xA7\xE8\xBA\xAB\xE8\xA1\xA3"},
      std::string_view{"\xE8\xBF\x9E\xE4\xBD\x93\xE8\xA3\xA4"},
      std::string_view{"\xE6\xB3\xB3\xE8\xA3\x85"},
      std::string_view{"\xE6\xB3\xB3\xE8\xA3\xA4"},
      std::string_view{"\xE8\xA3\xA4"},
      std::string_view{"\xE8\xA3\xA4\xE5\xAD\x90"},
      std::string_view{"\xE7\x9F\xAD\xE8\xA3\xA4"},
      std::string_view{"\xE7\x83\xAD\xE8\xA3\xA4"},
      std::string_view{"\xE7\x89\x9B\xE4\xBB\x94\xE8\xA3\xA4"},
      std::string_view{"\xE6\x89\x93\xE5\xBA\x95\xE8\xA3\xA4"},
      std::string_view{"\xE7\xB4\xA7\xE8\xBA\xAB\xE8\xA3\xA4"},
      std::string_view{"\xE8\xA3\x99"},
      std::string_view{"\xE8\xA3\x99\xE5\xAD\x90"},
      std::string_view{"\xE7\x9F\xAD\xE8\xA3\x99"},
      std::string_view{"\xE8\xBF\xB7\xE4\xBD\xA0\xE8\xA3\x99"},
      std::string_view{"\xE5\x8D\x8A\xE8\xBA\xAB\xE8\xA3\x99"},
      std::string_view{"\xE8\xBF\x9E\xE8\xA1\xA3\xE8\xA3\x99"},
      std::string_view{"\xE6\x97\x97\xE8\xA2\x8D"},
      std::string_view{"\xE6\xAF\x94\xE5\x9F\xBA\xE5\xB0\xBC\xE4\xB8\x8B\xE8\xA3\x85"},
      std::string_view{"\xE6\xAF\x94\xE5\x9F\xBA\xE5\xB0\xBC\xE4\xB8\x8B\xE8\xA1\xA3"}};

  return ContainsAnySearchTerm(BuildArmorClassificationText(a_armor),
                               kLowerBodyTerms);
}

[[nodiscard]] bool
IsLikelyUpperOnlyBodyArmor(const RE::TESObjectARMO *a_armor) {
  if (!a_armor || (GetArmorDisplaySlotMask(a_armor) & BodySlotMask()) == 0) {
    return false;
  }

  const auto text = BuildArmorClassificationText(a_armor);
  static constexpr std::array kExplicitUpperOnlyTerms{
      std::string_view{"bikini top"},
      std::string_view{"bikini upper"},
      std::string_view{"dress top"},
      std::string_view{"dress upper"},
      std::string_view{"swim top"},
      std::string_view{"swim upper"},
      std::string_view{"swimsuit top"},
      std::string_view{"swimsuit upper"},
      std::string_view{"upper bikini"},
      std::string_view{"upper dress"},
      std::string_view{"upper swim"},
      std::string_view{"upper swimsuit"},
      std::string_view{"\xEB\xB9\x84\xED\x82\xA4\xEB\x8B\x88 \xEC\x83\x81\xEC\x9D\x98"},
      std::string_view{"\xEB\x93\x9C\xEB\xA0\x88\xEC\x8A\xA4 \xEC\x83\x81\xEC\x9D\x98"},
      std::string_view{"\xEC\x88\x98\xEC\x98\x81\xEB\xB3\xB5 \xEC\x83\x81\xEC\x9D\x98"},
      std::string_view{"\xEC\x83\x81\xEC\x9D\x98 \xEB\xB9\x84\xED\x82\xA4\xEB\x8B\x88"},
      std::string_view{"\xEC\x83\x81\xEC\x9D\x98 \xEB\x93\x9C\xEB\xA0\x88\xEC\x8A\xA4"},
      std::string_view{"\xEC\x83\x81\xEC\x9D\x98 \xEC\x88\x98\xEC\x98\x81\xEB\xB3\xB5"},
      std::string_view{"\xE5\x86\x85\xE8\xA1\xA3\xE4\xB8\x8A"},
      std::string_view{"\xE5\x86\x85\xE8\xA1\xA3 \xE4\xB8\x8A"},
      std::string_view{"\xE6\xAF\x94\xE5\x9F\xBA\xE5\xB0\xBC\xE4\xB8\x8A\xE8\xA1\xA3"},
      std::string_view{"\xE6\xB3\xB3\xE8\xA3\x85\xE4\xB8\x8A\xE8\xA1\xA3"},
      std::string_view{"\xE6\xB3\xB3\xE8\xA1\xA3\xE4\xB8\x8A\xE8\xA1\xA3"}};
  if (ContainsAnySearchTerm(text, kExplicitUpperOnlyTerms)) {
    return true;
  }

  static constexpr std::array kLowerBodyBlockers{
      std::string_view{"bottom"},     std::string_view{"bottoms"},
      std::string_view{"boxer"},      std::string_view{"boxers"},
      std::string_view{"brief"},      std::string_view{"briefs"},
      std::string_view{"culotte"},    std::string_view{"culottes"},
      std::string_view{"dress"},      std::string_view{"falda"},
      std::string_view{"fundoshi"},   std::string_view{"hotpants"},
      std::string_view{"jean"},       std::string_view{"jeans"},
      std::string_view{"knicker"},    std::string_view{"knickers"},
      std::string_view{"legging"},    std::string_view{"leggings"},
      std::string_view{"loincloth"},  std::string_view{"low"},
      std::string_view{"lower"},      std::string_view{"maillot"},
      std::string_view{"monokini"},   std::string_view{"one piece"},
      std::string_view{"onepiece"},   std::string_view{"pantie"},
      std::string_view{"panties"},    std::string_view{"pants"},
      std::string_view{"pantsmp"},    std::string_view{"pantsu"},
      std::string_view{"panty"},      std::string_view{"panrty"},
      std::string_view{"short"},      std::string_view{"shorts"},
      std::string_view{"skirt"},      std::string_view{"thong"},
      std::string_view{"thongs"},     std::string_view{"trouser"},
      std::string_view{"trousers"},   std::string_view{"underpant"},
      std::string_view{"underpants"}, std::string_view{"undies"},
      std::string_view{"\xEB\xA0\x88\xEA\xB9\x85\xEC\x8A\xA4"},
      std::string_view{"\xEB\xB0\x94\xEC\xA7\x80"},
      std::string_view{"\xEB\x93\x9C\xEB\xA0\x88\xEC\x8A\xA4"},
      std::string_view{"\xEC\x86\x8D\xEB\xB0\x94\xEC\xA7\x80"},
      std::string_view{"\xEC\x8A\xA4\xEC\xBB\xA4\xED\x8A\xB8"},
      std::string_view{"\xEC\x87\xBC\xEC\xB8\xA0"},
      std::string_view{"\xEC\x88\x8F\xED\x8C\xAC\xEC\xB8\xA0"},
      std::string_view{"\xEC\x9B\x90\xED\x94\xBC\xEC\x8A\xA4"},
      std::string_view{"\xEC\xB9\x98\xEB\xA7\x88"},
      std::string_view{"\xED\x8C\xAC\xEC\xB8\xA0"},
      std::string_view{"\xED\x8C\xAC\xED\x8B\xB0"},
      std::string_view{"\xED\x95\x98\xEC\x9D\x98"},
      std::string_view{"\xED\x95\x98\xEC\xB2\xB4"},
      std::string_view{"\xE4\xB8\x8B\xE8\xA3\x85"},
      std::string_view{"\xE4\xB8\x8B\xE8\xA1\xA3"},
      std::string_view{"\xE5\x86\x85\xE8\xA3\xA4"},
      std::string_view{"\xE5\xBA\x95\xE8\xA3\xA4"},
      std::string_view{"\xE5\x86\x85\xE8\xA1\xA3\xE4\xB8\x8B"},
      std::string_view{"\xE5\x86\x85\xE8\xA1\xA3 \xE4\xB8\x8B"},
      std::string_view{"\xE6\xB3\xB3\xE8\xA3\xA4"},
      std::string_view{"\xE8\xA3\xA4"},
      std::string_view{"\xE8\xA3\xA4\xE5\xAD\x90"},
      std::string_view{"\xE7\x9F\xAD\xE8\xA3\xA4"},
      std::string_view{"\xE7\x83\xAD\xE8\xA3\xA4"},
      std::string_view{"\xE7\x89\x9B\xE4\xBB\x94\xE8\xA3\xA4"},
      std::string_view{"\xE6\x89\x93\xE5\xBA\x95\xE8\xA3\xA4"},
      std::string_view{"\xE7\xB4\xA7\xE8\xBA\xAB\xE8\xA3\xA4"},
      std::string_view{"\xE8\xA3\x99"},
      std::string_view{"\xE8\xA3\x99\xE5\xAD\x90"},
      std::string_view{"\xE7\x9F\xAD\xE8\xA3\x99"},
      std::string_view{"\xE8\xBF\xB7\xE4\xBD\xA0\xE8\xA3\x99"},
      std::string_view{"\xE5\x8D\x8A\xE8\xBA\xAB\xE8\xA3\x99"},
      std::string_view{"\xE8\xBF\x9E\xE8\xA1\xA3\xE8\xA3\x99"},
      std::string_view{"\xE6\x97\x97\xE8\xA2\x8D"},
      std::string_view{"\xE8\xBF\x9E\xE4\xBD\x93\xE8\xA1\xA3"},
      std::string_view{"\xE8\xBF\x9E\xE4\xBD\x93\xE7\xB4\xA7\xE8\xBA\xAB\xE8\xA1\xA3"},
      std::string_view{"\xE7\xB4\xA7\xE8\xBA\xAB\xE8\xA1\xA3"},
      std::string_view{"\xE7\xA4\xBC\xE6\x9C\x8D"}};
  if (ContainsAnySearchTerm(text, kLowerBodyBlockers)) {
    return false;
  }

  static constexpr std::array kUpperBodyTerms{
      std::string_view{"blouse"},    std::string_view{"bodice"},
      std::string_view{"bolero"},    std::string_view{"bra"},
      std::string_view{"bralette"},  std::string_view{"brassiere"},
      std::string_view{"breast"},    std::string_view{"bustier"},
      std::string_view{"camisole"},  std::string_view{"chest"},
      std::string_view{"crop"},
      std::string_view{"croptop"},   std::string_view{"halter"},
      std::string_view{"hoodie"},    std::string_view{"jacket"},
      std::string_view{"shirt"},     std::string_view{"sweater"},
      std::string_view{"top"},       std::string_view{"tube"},
      std::string_view{"upper"},
      std::string_view{"vest"},
      std::string_view{"\xEA\xB0\x80\xEC\x8A\xB4"},
      std::string_view{"\xEB\xB8\x8C\xEB\x9D\xBC"},
      std::string_view{"\xEB\xB8\x8C\xEB\x9E\x98\xEC\xA7\x80\xEC\x96\xB4"},
      std::string_view{"\xEB\xB8\x94\xEB\x9D\xBC\xEC\x9A\xB0\xEC\x8A\xA4"},
      std::string_view{"\xEC\x83\x81\xEC\x9D\x98"},
      std::string_view{"\xEC\x85\x94\xEC\xB8\xA0"},
      std::string_view{"\xEC\x9E\x90\xEC\xBC\x93"},
      std::string_view{"\xEC\x9E\xAC\xED\x82\xB7"},
      std::string_view{"\xEC\xA1\xB0\xEB\x81\xBC"},
      std::string_view{"\xED\x83\x91"},
      std::string_view{"\xED\x81\xAC\xEB\xA1\xAD"},
      std::string_view{"\xE4\xB8\x8A\xE8\xA3\x85"},
      std::string_view{"\xE4\xB8\x8A\xE8\xA1\xA3"},
      std::string_view{"\xE5\x86\x85\xE8\xA1\xA3\xE4\xB8\x8A"},
      std::string_view{"\xE5\x86\x85\xE8\xA1\xA3 \xE4\xB8\x8A"},
      std::string_view{"\xE5\xA4\xB9\xE5\x85\x8B"},
      std::string_view{"\xE5\xA4\x96\xE5\xA5\x97"},
      std::string_view{"\xE6\x96\x87\xE8\x83\xB8"},
      std::string_view{"\xE8\x83\xB8\xE7\xBD\xA9"},
      std::string_view{"\xE4\xB9\xB3\xE7\xBD\xA9"},
      std::string_view{"\xE6\x8A\xB9\xE8\x83\xB8"},
      std::string_view{"\xE8\x83\x8C\xE5\xBF\x83"},
      std::string_view{"\xE8\xA1\xAC\xE8\xA1\xAB"},
      std::string_view{"\xE5\x90\x8A\xE5\xB8\xA6"}};
  return ContainsAnySearchTerm(text, kUpperBodyTerms);
}

[[nodiscard]] bool IsCorrectionRelevantArmor(RE::Actor *a_actor,
                                             const RE::TESObjectARMO *a_armor) {
  if (!a_armor || IsSosTngGenitalArmor(a_armor)) {
    return false;
  }
  if (IsGenitalCoveringArmor(a_actor, a_armor) ||
      IsGenitalRevealingArmor(a_actor, a_armor)) {
    return false;
  }
  const auto slotMask = static_cast<std::uint32_t>(GetArmorDisplaySlotMask(a_armor));
  return (slotMask & BodySlotMask()) != 0 ||
         (slotMask & GenitalSlotMask()) != 0 ||
         IsLikelyGenitalConcealingPelvisArmor(a_armor);
}

[[nodiscard]] bool ShouldRevealGenitals(RE::Actor *a_actor,
                                        const RE::TESObjectARMO *a_armor) {
  if (!a_armor || IsSosTngGenitalArmor(a_armor) ||
      IsGenitalCoveringArmor(a_actor, a_armor)) {
    return false;
  }
  return IsGenitalRevealingArmor(a_actor, a_armor) ||
         IsLikelyUpperOnlyBodyArmor(a_armor);
}

[[nodiscard]] bool ShouldConcealGenitals(RE::Actor *a_actor,
                                         const RE::TESObjectARMO *a_armor) {
  if (!a_armor || IsSosTngGenitalArmor(a_armor) ||
      IsGenitalRevealingArmor(a_actor, a_armor)) {
    return false;
  }

  const auto slotMask =
      static_cast<std::uint32_t>(GetArmorDisplaySlotMask(a_armor));
  if ((slotMask & BodySlotMask()) != 0) {
    return !IsLikelyUpperOnlyBodyArmor(a_armor);
  }

  return (slotMask & GenitalSlotMask()) != 0 ||
         IsGenitalCoveringArmor(a_actor, a_armor) ||
         IsLikelyGenitalConcealingPelvisArmor(a_armor);
}

[[nodiscard]] std::unordered_set<const RE::TESObjectARMO *>
CollectEquippedArmors(RE::TESObjectREFR *a_target) {
  std::unordered_set<const RE::TESObjectARMO *> equipped;
  auto *inventory = a_target ? a_target->GetInventoryChanges() : nullptr;
  if (!inventory) {
    return equipped;
  }

  WornArmorVisitor visitor;
  inventory->VisitWornItems(visitor);
  return std::move(visitor.armors);
}

[[nodiscard]] CorrectionSet BuildCorrectionSet(RE::Actor *a_actor) {
  CorrectionSet set;
  if (!a_actor || !g_gameDataLoaded.load(std::memory_order_relaxed)) {
    return set;
  }

  const auto equippedArmors = CollectEquippedArmors(a_actor);
  bool genitalArmorEquipped = false;
  for (const auto *armor : equippedArmors) {
    if (IsSosTngGenitalArmor(armor)) {
      genitalArmorEquipped = true;
      continue;
    }
    set.revealGenitals = set.revealGenitals || ShouldRevealGenitals(a_actor, armor);
    set.concealGenitals =
        set.concealGenitals || ShouldConcealGenitals(a_actor, armor);
  }

  set.active = genitalArmorEquipped &&
               (set.revealGenitals || set.concealGenitals);
  if (set.concealGenitals && genitalArmorEquipped) {
    for (const auto *armor : equippedArmors) {
      if (IsSosTngGenitalArmor(armor)) {
        set.hiddenArmorFormIDs.insert(armor->GetFormID());
      }
    }
  }
  return set;
}

[[nodiscard]] bool ShouldHideArmor(const CorrectionSet &a_set,
                                   const RE::TESObjectARMO *a_armor) {
  return a_armor && a_set.hiddenArmorFormIDs.contains(a_armor->GetFormID());
}

[[nodiscard]] RE::TESObjectARMO *
GetEntryArmor(RE::InventoryEntryData *a_entryData) {
  if (!a_entryData || !a_entryData->object) {
    return nullptr;
  }
  return a_entryData->object->As<RE::TESObjectARMO>();
}

class HiddenGenitalFilterVisitor final
    : public RE::InventoryChanges::IItemChangeVisitor {
public:
  HiddenGenitalFilterVisitor(const CorrectionSet &a_correctionSet,
                             RE::InventoryChanges::IItemChangeVisitor &a_visitor)
      : correctionSet_(a_correctionSet), visitor_(a_visitor) {}

  RE::BSContainer::ForEachResult
  Visit(RE::InventoryEntryData *a_entryData) override {
    if (ShouldSkip(a_entryData)) {
      return RE::BSContainer::ForEachResult::kContinue;
    }
    return visitor_.Visit(a_entryData);
  }

  bool ShouldVisit(RE::InventoryEntryData *a_entryData,
                   RE::TESBoundObject *a_object) override {
    if (ShouldSkip(a_entryData)) {
      return false;
    }
    return visitor_.ShouldVisit(a_entryData, a_object);
  }

  RE::BSContainer::ForEachResult Unk_03(RE::InventoryEntryData *a_entryData,
                                        void *a_arg2,
                                        bool *a_arg3) override {
    if (ShouldSkip(a_entryData)) {
      if (a_arg3) {
        *a_arg3 = true;
      }
      return RE::BSContainer::ForEachResult::kContinue;
    }
    return visitor_.Unk_03(a_entryData, a_arg2, a_arg3);
  }

private:
  [[nodiscard]] bool ShouldSkip(RE::InventoryEntryData *a_entryData) const {
    return ShouldHideArmor(correctionSet_, GetEntryArmor(a_entryData));
  }

  const CorrectionSet &correctionSet_;
  RE::InventoryChanges::IItemChangeVisitor &visitor_;
};

[[nodiscard]] std::uint32_t
CollectVisibleWornSlotMask(RE::TESObjectREFR *a_target,
                           const CorrectionSet &a_correctionSet) {
  std::uint32_t slotMask = 0;
  for (const auto *armor : CollectEquippedArmors(a_target)) {
    if (ShouldHideArmor(a_correctionSet, armor)) {
      continue;
    }
    slotMask |= static_cast<std::uint32_t>(GetArmorCorrectionSlotMask(armor));
  }
  return slotMask;
}

void AddKeywordIfPresent(RE::TESObjectARMO *a_armor,
                         RE::BGSKeyword *a_keyword) {
  if (!a_armor || !a_keyword || a_armor->HasKeyword(a_keyword)) {
    return;
  }
  a_armor->AddKeyword(a_keyword);
}

void RemoveKeywordIfPresent(RE::TESObjectARMO *a_armor,
                            RE::BGSKeyword *a_keyword) {
  if (!a_armor || !a_keyword || !a_armor->HasKeyword(a_keyword)) {
    return;
  }
  a_armor->RemoveKeyword(a_keyword);
}

struct RuntimeKeywords {
  RE::BGSKeyword *sosRevealing{nullptr};
  RE::BGSKeyword *sosConcealing{nullptr};
  RE::BGSKeyword *sosUnderwear{nullptr};
  RE::BGSKeyword *tngRevealing{nullptr};
  RE::BGSKeyword *tngRevealingOnlyWomen{nullptr};
  RE::BGSKeyword *tngRevealingOnlyMen{nullptr};
  RE::BGSKeyword *tngCovering{nullptr};
  RE::BGSKeyword *tngUnderwear{nullptr};

  [[nodiscard]] bool HasAny() const {
    return sosRevealing || sosConcealing || sosUnderwear || tngRevealing ||
           tngRevealingOnlyWomen || tngRevealingOnlyMen || tngCovering ||
           tngUnderwear;
  }
};

[[nodiscard]] RuntimeKeywords LookupRuntimeKeywords() {
  return {
      .sosRevealing =
          RE::TESForm::LookupByEditorID<RE::BGSKeyword>("SOS_Revealing"),
      .sosConcealing =
          RE::TESForm::LookupByEditorID<RE::BGSKeyword>("SOS_Concealing"),
      .sosUnderwear =
          RE::TESForm::LookupByEditorID<RE::BGSKeyword>("SOS_Underwear"),
      .tngRevealing =
          RE::TESForm::LookupByEditorID<RE::BGSKeyword>("TNG_Revealing"),
      .tngRevealingOnlyWomen =
          RE::TESForm::LookupByEditorID<RE::BGSKeyword>(
              "TNG_RevealingOnlyWomen"),
      .tngRevealingOnlyMen =
          RE::TESForm::LookupByEditorID<RE::BGSKeyword>(
              "TNG_RevealingOnlyMen"),
      .tngCovering =
          RE::TESForm::LookupByEditorID<RE::BGSKeyword>("TNG_Covering"),
      .tngUnderwear =
          RE::TESForm::LookupByEditorID<RE::BGSKeyword>("TNG_Underwear")};
}

void ApplyRevealingKeywordOverride(RE::TESObjectARMO *a_armor,
                                   const RuntimeKeywords &a_keywords) {
  RemoveKeywordIfPresent(a_armor, a_keywords.sosConcealing);
  RemoveKeywordIfPresent(a_armor, a_keywords.sosUnderwear);
  RemoveKeywordIfPresent(a_armor, a_keywords.tngCovering);
  RemoveKeywordIfPresent(a_armor, a_keywords.tngUnderwear);
  AddKeywordIfPresent(a_armor, a_keywords.sosRevealing);
  AddKeywordIfPresent(a_armor, a_keywords.tngRevealing);
}

void RemoveRevealingKeywordConflicts(
    RE::TESObjectARMO *a_armor, const RuntimeKeywords &a_keywords) {
  RemoveKeywordIfPresent(a_armor, a_keywords.sosRevealing);
  RemoveKeywordIfPresent(a_armor, a_keywords.tngRevealing);
  RemoveKeywordIfPresent(a_armor, a_keywords.tngRevealingOnlyWomen);
  RemoveKeywordIfPresent(a_armor, a_keywords.tngRevealingOnlyMen);
}

void ApplyConcealingKeywordOverride(RE::TESObjectARMO *a_armor,
                                    const RuntimeKeywords &a_keywords) {
  RemoveRevealingKeywordConflicts(a_armor, a_keywords);
  AddKeywordIfPresent(a_armor, a_keywords.sosConcealing);
  AddKeywordIfPresent(a_armor, a_keywords.sosUnderwear);
  AddKeywordIfPresent(a_armor, a_keywords.tngCovering);
  AddKeywordIfPresent(a_armor, a_keywords.tngUnderwear);
}

namespace re {
enum class EquipFlag : std::uint8_t {
  kNone = 0,
  kNeedsUpdate = 1 << 0,
};

void SetEquipFlag(RE::AIProcess *a_process, EquipFlag a_flag) {
  using Func = void (*)(RE::AIProcess *, EquipFlag);
  static REL::Relocation<Func> func{RELOCATION_ID(38867, 39907)};
  return func(a_process, a_flag);
}

void UpdateEquipment(RE::AIProcess *a_process, RE::Actor *a_actor) {
  using Func = void (*)(RE::AIProcess *, RE::Actor *);
  static REL::Relocation<Func> func{RELOCATION_ID(38404, 39395)};
  return func(a_process, a_actor);
}
} // namespace re

class EquipmentEventSink final : public RE::BSTEventSink<RE::TESEquipEvent> {
public:
  static EquipmentEventSink *GetSingleton() {
    static EquipmentEventSink singleton;
    return std::addressof(singleton);
  }

  RE::BSEventNotifyControl
  ProcessEvent(const RE::TESEquipEvent *a_event,
               RE::BSTEventSource<RE::TESEquipEvent> *) override {
    if (!stsc::IsGameDataLoaded() || !a_event || a_event->baseObject == 0) {
      return RE::BSEventNotifyControl::kContinue;
    }

    const auto *armor =
        RE::TESForm::LookupByID<RE::TESObjectARMO>(a_event->baseObject);
    auto *actorRef = a_event->actor.get();
    auto *actor = actorRef ? actorRef->As<RE::Actor>() : nullptr;
    if (armor && actor && IsCorrectionRelevantArmor(actor, armor)) {
      stsc::QueueRefreshFor(actor);
    }
    return RE::BSEventNotifyControl::kContinue;
  }
};
} // namespace

namespace stsc {
void SetGameDataLoaded(const bool a_loaded) {
  g_gameDataLoaded.store(a_loaded, std::memory_order_relaxed);
}

bool IsGameDataLoaded() {
  return g_gameDataLoaded.load(std::memory_order_relaxed);
}

void ApplyRuntimeArmorCorrections() {
  auto *dataHandler = RE::TESDataHandler::GetSingleton();
  if (!dataHandler) {
    return;
  }

  const auto keywords = LookupRuntimeKeywords();
  if (!keywords.HasAny()) {
    logger::info(
        "SOS/TNG runtime keywords were not found; skinning-only correction remains active");
    return;
  }

  std::uint32_t revealingCount = 0;
  std::uint32_t bodyDefaultCount = 0;
  std::uint32_t pelvisConcealingCount = 0;
  for (auto *armor : dataHandler->GetFormArray<RE::TESObjectARMO>()) {
    if (!armor || IsSosTngGenitalArmor(armor)) {
      continue;
    }

    if (IsLikelyUpperOnlyBodyArmor(armor)) {
      ApplyRevealingKeywordOverride(armor, keywords);
      ++revealingCount;
      continue;
    }

    const auto slotMask = GetArmorDisplaySlotMask(armor);
    if ((slotMask & BodySlotMask()) != 0) {
      // Slot 32 already conceals by default in SOS/TNG. Only remove an
      // opposing ESP/KID revealing classification; do not turn ordinary body
      // armor into SOS/TNG underwear or add redundant covering keywords.
      RemoveRevealingKeywordConflicts(armor, keywords);
      ++bodyDefaultCount;
      continue;
    }

    if (IsLikelyGenitalConcealingPelvisArmor(armor)) {
      ApplyConcealingKeywordOverride(armor, keywords);
      ++pelvisConcealingCount;
    }
  }

  logger::info(
      "Applied SOS/TNG runtime armor corrections: {} upper revealing, {} slot-32 default covering, {} slot-49 concealing",
      revealingCount, bodyDefaultCount, pelvisConcealingCount);
}

void RegisterEquipmentEventSink() {
  static std::once_flag registered;
  std::call_once(registered, [] {
    auto *holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (!holder) {
      logger::warn("Failed to register SOS/TNG slot correction equip sink");
      return;
    }
    holder->AddEventSink<RE::TESEquipEvent>(EquipmentEventSink::GetSingleton());
    logger::info("Registered SOS/TNG slot correction equip sink");
  });
}

void InvalidateQueuedRefreshes() {
  ++g_refreshGeneration;
  ClearQueuedActorRefreshes();
}

void RefreshFor(RE::Actor *a_actor) {
  if (!IsGameDataLoaded() || !IsActorRefreshable(a_actor)) {
    return;
  }
  auto *process = a_actor->GetActorRuntimeData().currentProcess;
  if (!process) {
    return;
  }
  re::SetEquipFlag(process, re::EquipFlag::kNeedsUpdate);
  re::UpdateEquipment(process, a_actor);
}

void QueueRefreshFor(RE::Actor *a_actor) {
  if (!a_actor) {
    return;
  }
  const auto actorFormID = a_actor->GetFormID();
  if (actorFormID == 0) {
    return;
  }
  const auto generation = g_refreshGeneration.load(std::memory_order_relaxed);
  const auto actorGeneration = QueueActorRefreshGeneration(actorFormID);

  auto *taskInterface = SKSE::GetTaskInterface();
  if (!taskInterface) {
    logger::warn("Skipped SOS/TNG slot correction refresh: no task interface");
    return;
  }

  taskInterface->AddTask([actorFormID, generation, actorGeneration]() {
    if (g_refreshGeneration.load(std::memory_order_relaxed) != generation ||
        !IsLatestActorRefreshGeneration(actorFormID, actorGeneration)) {
      return;
    }
    auto *actor = RE::TESForm::LookupByID<RE::Actor>(actorFormID);
    stsc::RefreshFor(actor);
  });
}

void QueuePlayerRefresh() {
  QueueRefreshFor(RE::PlayerCharacter::GetSingleton());
}

bool ShouldOverrideSkinning(RE::TESObjectREFR *a_target) {
  auto *actor = a_target ? a_target->As<RE::Actor>() : nullptr;
  return BuildCorrectionSet(actor).active;
}

bool ShouldBlockVanillaArmor(RE::TESObjectARMO *a_armor,
                             RE::TESObjectREFR *a_target) {
  auto *actor = a_target ? a_target->As<RE::Actor>() : nullptr;
  const auto set = BuildCorrectionSet(actor);
  return ShouldHideArmor(set, a_armor);
}

std::uint32_t GetCorrectedWornMask(RE::InventoryChanges *a_inventory,
                                   RE::TESObjectREFR *a_target,
                                   const std::uint32_t a_baseWornMask) {
  (void)a_inventory;
  auto *actor = a_target ? a_target->As<RE::Actor>() : nullptr;
  const auto set = BuildCorrectionSet(actor);
  if (!set.active) {
    return a_baseWornMask;
  }
  return CollectVisibleWornSlotMask(a_target, set);
}

void VisitWornItemsWithGenitalFilter(
    RE::InventoryChanges *a_inventory,
    RE::InventoryChanges::IItemChangeVisitor *a_visitor,
    RE::TESObjectREFR *a_target, const std::uintptr_t a_visitWornItems) {
  using VisitWornItems = void (*)(RE::InventoryChanges *,
                                  RE::InventoryChanges::IItemChangeVisitor *);
  auto *visitWornItems = reinterpret_cast<VisitWornItems>(a_visitWornItems);
  if (!visitWornItems || !a_inventory || !a_visitor) {
    return;
  }

  auto *target = a_target ? a_target : a_inventory->owner;
  auto *actor = target ? target->As<RE::Actor>() : nullptr;
  const auto set = BuildCorrectionSet(actor);
  if (!set.active || set.hiddenArmorFormIDs.empty()) {
    visitWornItems(a_inventory, a_visitor);
    return;
  }

  HiddenGenitalFilterVisitor visitor{set, *a_visitor};
  visitWornItems(a_inventory, &visitor);
}

void ApplyAdditionalDisplayArmors(RE::Actor *, RE::ActorWeightModel *) {}
} // namespace stsc
