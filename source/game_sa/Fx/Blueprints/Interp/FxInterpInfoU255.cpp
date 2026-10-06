#include "StdInc.h"

#include "FxInterpInfoU255.h"
#include "FxManager.h"

void FxInterpInfoU255_c::InjectHooks() {
    RH_ScopedClass(FxInterpInfoU255_c);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(GetVal, 0x4A8800);
}

// 0x4A87D0
FxInterpInfoU255_c::FxInterpInfoU255_c() : FxInterpInfo_c() {
    m_Keys = nullptr;
}

// 0x5C18F0
void FxInterpInfoU255_c::Load(FILESTREAM file) {
    for (auto i = 0; i < m_nCount; i++) {
        ReadField<void>(file);
        ReadField<void>(file, "FX_INTERP_DATA:");

        m_bLooped  = ReadField<bool>(file);
        m_nNumKeys = ReadField<int8>(file);

        if (i == 0) {
            m_pTimes = g_fxMan.Allocate<uint16>(m_nNumKeys);
        }

        m_Keys[i] = g_fxMan.Allocate<uint16>(m_nNumKeys);
        for (auto j = 0; j < m_nNumKeys; j++) {
            ReadField<void>(file, "FX_KEYFLOAT_DATA:");
            m_pTimes[j] = uint16(ReadField<float>(file) * 256.f);
            m_Keys[i][j] = uint16(ReadField<float>(file) * 256.f);
        }
    }
}

// NOTSA
void FxInterpInfoU255_c::Allocate(int32 count) {
    m_nCount = count;
    m_Keys = g_fxMan.Allocate<uint16*>(count);
}

// 0x4A8800
void FxInterpInfoU255_c::GetVal(float* outValues, float delta) {
    constexpr auto SCALE = 1.0f / 256.0f; // 0x859AA0 - Used for both times and keys

    const auto count = (int8)m_nCount; // The game reads this as a signed byte

    if (m_nNumKeys == 1) {
        for (auto i = 0; i < count; i++) {
            outValues[i] = (float)m_Keys[i][0] * SCALE;
        }
        return;
    }

    if (m_bLooped) {
        const auto totalTime = (float)m_pTimes[m_nNumKeys - 1] * SCALE;
        delta -= (float)(int32)(delta / totalTime) * totalTime;
    }

    for (auto k = 1; k < m_nNumKeys; k++) {
        const auto time = (float)m_pTimes[k] * SCALE;
        if (delta < time) {
            const auto prevTime = (float)m_pTimes[k - 1] * SCALE;
            for (auto i = 0; i < count; i++) {
                const auto prev = (float)m_Keys[i][k - 1] * SCALE;
                const auto curr = (float)m_Keys[i][k] * SCALE;
                outValues[i] = (curr - prev) * (delta - prevTime) / (time - prevTime) + prev;
            }
            return;
        }
    }

    // Past the last key
    for (auto i = 0; i < count; i++) {
        outValues[i] = (float)m_Keys[i][m_nNumKeys - 1] * SCALE;
    }
}
