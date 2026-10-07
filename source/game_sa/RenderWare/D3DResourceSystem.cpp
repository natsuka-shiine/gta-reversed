/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#ifdef NOTSA_NO_ORIGINAL_CODE
/*
* The original keeps a pool of released textures and index buffers of RenderWare's Direct3D 9 driver to reuse (and frees some of them
* each frame). That's not reversed yet. This is the same without the pool: nothing is kept, so there's nothing to tidy up either.
*/
void   D3DResourceSystem::CancelBuffering() {}
uint32 D3DResourceSystem::GetTotalIndexDataSize() { return 0; }
uint32 D3DResourceSystem::GetTotalPixelsSize() { return 0; }
void   D3DResourceSystem::Init() {}
void   D3DResourceSystem::SetUseD3DResourceBuffering(bool) {}
void   D3DResourceSystem::Shutdown() {}
void   D3DResourceSystem::TidyUpD3DIndexBuffers(uint32) {}
void   D3DResourceSystem::TidyUpD3DTextures(uint32) {}

int32 D3DResourceSystem::CreateIndexBuffer(uint32 numIndices, uint32 format, void** ppIndexBuffer) {
    return GetD3D9Device()->CreateIndexBuffer(numIndices * (format == D3DFMT_INDEX32 ? 4 : 2), D3DUSAGE_WRITEONLY, (D3DFORMAT)format, D3DPOOL_MANAGED, reinterpret_cast<IDirect3DIndexBuffer9**>(ppIndexBuffer), nullptr);
}

int32 D3DResourceSystem::CreateTexture(int32 width, int32 height, uint32 format, void** ppTexture) {
    return GetD3D9Device()->CreateTexture(width, height, 1, 0, (D3DFORMAT)format, D3DPOOL_MANAGED, reinterpret_cast<IDirect3DTexture9**>(ppTexture), nullptr);
}

void D3DResourceSystem::DestroyIndexBuffer(void* pIndexBuffer) {
    if (pIndexBuffer) {
        static_cast<IDirect3DIndexBuffer9*>(pIndexBuffer)->Release();
    }
}

void D3DResourceSystem::DestroyTexture(void* texture) {
    if (texture) {
        static_cast<IDirect3DTexture9*>(texture)->Release();
    }
}
#else

// 0x730900
void D3DResourceSystem::CancelBuffering() {
    plugin::Call<0x730900>();
}

// 0x7307F0
uint32 D3DResourceSystem::GetTotalIndexDataSize() {
    return plugin::CallAndReturn<uint32, 0x7307F0>();
}

// 0x730660
uint32 D3DResourceSystem::GetTotalPixelsSize() {
    return plugin::CallAndReturn<uint32, 0x730660>();
}

// 0x730830
void D3DResourceSystem::Init() {
    plugin::Call<0x730830>();
}

// 0x730AC0
void D3DResourceSystem::SetUseD3DResourceBuffering(bool bUse) {
    ZoneScoped;

    plugin::Call<0x730AC0, bool>(bUse);
}

// 0x730A00
void D3DResourceSystem::Shutdown() {
    plugin::Call<0x730A00>();
}

// 0x730740
void D3DResourceSystem::TidyUpD3DIndexBuffers(uint32 count) {
    plugin::Call<0x730740, uint32>(count);
}

// 0x7305E0
void D3DResourceSystem::TidyUpD3DTextures(uint32 count) {
    plugin::Call<0x7305E0, uint32>(count);
}

// 0x7306A0
int32 D3DResourceSystem::CreateIndexBuffer(uint32 numIndices, uint32 format, void** ppIndexBuffer) {
    return plugin::CallAndReturn<int32, 0x7306A0, uint32, uint32, void**>(numIndices, format, ppIndexBuffer);
}

// 0x730510
int32 D3DResourceSystem::CreateTexture(int32 width, int32 height, uint32 format, void** ppTexture) {
    return plugin::CallAndReturn<int32, 0x730510, int32, int32, uint32, void**>(width, height, format, ppTexture);
}

// 0x730D30
void D3DResourceSystem::DestroyIndexBuffer(void* pIndexBuffer) {
    plugin::Call<0x730D30, void*>(pIndexBuffer);
}

// 0x730B70
void D3DResourceSystem::DestroyTexture(void* texture) {
    plugin::Call<0x730B70, void*>(texture);
}
#endif // NOTSA_NO_ORIGINAL_CODE
