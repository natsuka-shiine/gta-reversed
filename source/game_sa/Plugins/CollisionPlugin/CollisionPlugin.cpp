#include "StdInc.h"

#include "CollisionPlugin.h"

auto& gCollisionPluginOffset = StaticRef<RwInt32, 0x9689DC>();

static RwStream* ClumpCollisionStreamRead(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);

void CCollisionPlugin::InjectHooks() {
    RH_ScopedClass(CCollisionPlugin);
    RH_ScopedCategory("Plugins");

    RH_ScopedInstall(PluginAttach, 0x41B310);
    RH_ScopedInstall(SetModelInfo, 0x41B350);
    RH_ScopedGlobalInstall(ClumpCollisionStreamRead, 0x41B1D0);
}

// internal
// 0x41B1A0
static void* ClumpCollisionConstructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return object;
}

// internal
// 0x41B1C0
static void* ClumpCollisionDestructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return object;
}

// internal
// 0x41B1B0
static void* ClumpCollisionCopyConstructor(void* dstObject, const void* srcObject, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return dstObject;
}

// internal
// 0x41B1D0
static RwStream* ClumpCollisionStreamRead(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    CMemoryMgr::LockScratchPad(); // NOTE: No-op on Windows, present on Android
    RwStreamRead(stream, &PC_Scratch, binaryLength);

    auto* const cm = new CColModel();

    auto* const buffer   = reinterpret_cast<uint8*>(&PC_Scratch[0]);
    const auto  fourCC   = *reinterpret_cast<uint32*>(&buffer[0]);
    const auto  fileSize = *reinterpret_cast<uint32*>(&buffer[4]);
    constexpr auto HEADER_SIZE = 32u;
    switch (fourCC) {
    case MakeFourCC("COLL"):
        CFileLoader::LoadCollisionModel(&buffer[HEADER_SIZE], *cm);
        break;
    case MakeFourCC("COL2"):
        CFileLoader::LoadCollisionModelVer2(&buffer[HEADER_SIZE], fileSize - 24, *cm, nullptr);
        break;
    case MakeFourCC("COL3"):
        CFileLoader::LoadCollisionModelVer3(&buffer[HEADER_SIZE], fileSize - 24, *cm, nullptr);
        break;
    default: // Headerless data
        CFileLoader::LoadCollisionModel(&buffer[0], *cm);
        break;
    }

    cm->MakeMultipleAlloc();
    CCollisionPlugin::ms_currentModel->SetColModel(cm, true);
    CCollisionPlugin::ms_currentModel->bOwnsCollisionModel = true;

    CMemoryMgr::ReleaseScratchPad();
    return stream;
}

// internal
// 0x41B2F0
static RwStream* ClumpCollisionStreamWrite(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return stream;
}

// internal
// 0x41B300
static RwInt32 ClumpCollisionGetSize(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    return -1;
}

// 0x41B310
bool CCollisionPlugin::PluginAttach() {
    // 0x9689DC unused
    gCollisionPluginOffset = RpClumpRegisterPlugin(
        0,
        rwID_COLLISIONPLUGIN,
        ClumpCollisionConstructor,
        ClumpCollisionDestructor,
        ClumpCollisionCopyConstructor
    );

    RpClumpRegisterPluginStream(
        rwID_COLLISIONPLUGIN,
        ClumpCollisionStreamRead,
        ClumpCollisionStreamWrite,
        ClumpCollisionGetSize
    );

    return TRUE;
}

// 0x41B350
void CCollisionPlugin::SetModelInfo(CClumpModelInfo* modelInfo) {
    ms_currentModel = modelInfo;
}
