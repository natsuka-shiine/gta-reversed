#include "StdInc.h"

#ifdef NOTSA_STANDALONE_DUMP_HOOKS_ONLY
#include "ReversibleHook/NullHook.h"
#else
#include "ReversibleHook/VirtualHook.h"
#endif
#include "ReversibleHook/ScriptCommandHook.h"
#include "RHManager.h"
#include "HookConstants.hpp"

static constexpr auto HOOKS_CHECK_INTERVAL = std::chrono::milliseconds{ 500 };

namespace ReversibleHooks {
#ifdef NOTSA_LIBRW
namespace {
struct VirtualHookTarget {
    std::string name;
    uint8*      fnGTA;
    void*       fnOur;
};
std::vector<VirtualHookTarget> s_VirtualHookTargets;
};

void RedirectOriginalsOfVirtualHooks() {
    using namespace Constants;

    uint32 numRedirected{}, numSkipped{};
    for (const auto& t : s_VirtualHookTargets) {
        if (t.fnGTA[0] == JUMP_OPCODE) { // Has a hook on it (or is one of ours from an earlier entry: more classes can have the same function)
            continue;
        }

        // Not if it's shorter than the jump, and something else follows right after it
        const auto IsPadding = [](uint8 b) { return b == 0xCC || b == 0x90; };
        auto       fits      = true;
        for (auto i = 0u; i + 1 < JUMP_OP_SIZE; i++) {
            if (t.fnGTA[i] == 0xC3 && !std::all_of(t.fnGTA + i + 1, t.fnGTA + JUMP_OP_SIZE, IsPadding)) { // `ret`
                fits = false;
                break;
            }
        }
        if (!fits) {
            NOTSA_LOG_DEBUG("Original of virtual `{}` at {} is too short to redirect", t.name, static_cast<void*>(t.fnGTA));
            numSkipped++;
            continue;
        }

        uint8      jmp[JUMP_OP_SIZE]{ JUMP_OPCODE };
        const auto rel = static_cast<int32>(reinterpret_cast<uintptr>(t.fnOur) - (reinterpret_cast<uintptr>(t.fnGTA) + JUMP_OP_SIZE));
        memcpy(jmp + 1, &rel, sizeof(rel));
        Utility::VirtualCopy(t.fnGTA, jmp, sizeof(jmp));
        numRedirected++;
    }
    NOTSA_LOG_INFO("Redirected the originals of {} virtual functions to ours ({} were too short for that)", numRedirected, numSkipped);
    s_VirtualHookTargets.clear();
}
#endif

void RHManager::CheckAll() {
    if (const auto now = HooksCheckClock::now(); now - m_LastHooksCheckTime > HOOKS_CHECK_INTERVAL) {
        m_LastHooksCheckTime = now;
        m_RootHookCategory->ForEachItem([](const auto item) {
            item->GetHook()->Check();
        });
    }
}

void RHManager::WriteHooksToFile(const std::filesystem::path& file) {
    const auto path = std::filesystem::weakly_canonical(file);
    if (std::ofstream of{ file }) {
        json::array_t arr;
        [&](this auto&& Self, const HookCategory& c) -> void {
            for (const auto item : c.Items()) {
                json j{};
                to_json(j, *item);
                arr.emplace_back(std::move(j));
            }
            for (const auto subcat : c.SubCategories()) {
                Self(*subcat);
            }
        }(*GetRootCategory());
        of << std::setw(4) << arr;
        NOTSA_LOG_INFO("Hooks written to `{}`", path.string());
    } else {
        NOTSA_LOG_ERR("Failed to open file `{}` for writing hooks!", path.string());
    }
}

void RHManager::InstallVirtual(
    std::string_view   category,
    std::string        fnName,
    Utility::VMTInfo   vmtInfoOur,
    void*              fnAddressOur,
    Utility::VMTInfo   vmtInfoGTA,
    void*              fnAddressGTA,
    HookInstallOptions opt
) {
#ifdef NOTSA_STANDALONE_DUMP_HOOKS_ONLY
    AddHookToCategory(category, opt, std::make_shared<ReversibleHook::NullHook>(
        std::move(fnName),
        fnAddressOur,
        fnAddressGTA
    ));
#else
    const auto idx = vmtInfoGTA.FindIndexOf(fnAddressGTA);
#ifdef NOTSA_LIBRW
    s_VirtualHookTargets.push_back({ fnName, static_cast<uint8*>(fnAddressGTA), *vmtInfoOur.GetEntryAddressAt(idx) }); // (Before the hook changes the entry)
#endif
    // We can't do `vmtInfoOur.FindIndexOf(fnAddressOur)` because `fnAddressOur` points to the thunk, while the address in the VMT is pointing to the actual function

    if (opt.Overrides) {
        AddHookToCategory(category, opt, std::make_shared<ReversibleHook::VirtualHook>(
            std::move(fnName),
            vmtInfoOur.GetEntryAddressAt(idx),
            vmtInfoGTA.GetEntryAddressAt(idx)
        ));
    } else {
        AddHookToCategory(category, opt, std::make_shared<ReversibleHook::VMTRedirectHook>(
            std::move(fnName),
            vmtInfoOur.GetEntryAddressAt(idx),
            vmtInfoGTA.GetEntryAddressAt(idx)
        ));
    }
#endif
}

void RHManager::AddHookToCategory(std::string_view path, HookInstallOptions opt, std::shared_ptr<ReversibleHook::TwoWayHook> hook) {
    GetRootCategory()->FindCategoryByPath(path, true)->AddItem(std::make_shared<HookCategoryItem>(
        std::move(hook),
        opt.Locked,
        opt.Reversed,
        opt.State,
        opt.InstallSrcLoc
    ));
}

#ifdef NOTSA_WITH_SCRIPT_COMMAND_HOOKS
void RHManager::InstallScriptCommand(std::string_view path, eScriptCommands cmd, HookInstallOptions opt) {
    AddHookToCategory(
        path,
        opt,
        std::make_shared<ReversibleHook::ScriptCommandHook>(cmd) // This can stay a regular hook even in standalone, because it doesn't access the game in any way
    );
}
#endif
}; // namespace ReversibleHooks
