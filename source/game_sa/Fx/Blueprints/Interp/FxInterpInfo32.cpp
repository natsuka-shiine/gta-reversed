#include "StdInc.h"

#include "FxInterpInfo32.h"
#include "FxManager.h"

void FxInterpInfo32_c::InjectHooks() {
    RH_ScopedClass(FxInterpInfo32_c);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(GetVal, 0x4A89C0);
}

// 0x4A8990
FxInterpInfo32_c::FxInterpInfo32_c() : FxInterpInfo_c() {
    m_Keys = nullptr;
}

// 0x5C1B10
void FxInterpInfo32_c::Load(FILESTREAM file) {
    for (auto i = 0; i < m_nCount; i++) {
        ReadField<void>(file);
        ReadField<void>(file, "FX_INTERP_DATA:");

        m_bLooped  = ReadField<bool>(file);
        m_nNumKeys = ReadField<int8>(file);

        if (i == 0) {
            m_pTimes = g_fxMan.Allocate<uint16>(m_nNumKeys);
        }

        m_Keys[i] = g_fxMan.Allocate<int16>(m_nNumKeys);
        for (auto j = 0; j < m_nNumKeys; j++) {
            ReadField<void>(file, "FX_KEYFLOAT_DATA:");
            m_pTimes[j]     = uint16(ReadField<float>(file) * 256.f);
            m_Keys[i][j] = int16(ReadField<float>(file) * 1000.f);
        }
    }
}

// NOTSA
void FxInterpInfo32_c::Allocate(int32 count) {
    m_nCount  = count;
    m_Keys = g_fxMan.Allocate<int16*>(count);
}

// 0x4A89C0
void FxInterpInfo32_c::GetVal(float* outValues, float delta) {
    constexpr auto TIME_SCALE = 1.0f / 256.0f; // 0x859AA0
    constexpr auto KEY_SCALE  = 0.001f;        // 0x858CDC

    const auto count = (int8)m_nCount; // The game reads this as a signed byte

    if (m_nNumKeys == 1) {
        for (auto i = 0; i < count; i++) {
            outValues[i] = (float)m_Keys[i][0] * KEY_SCALE;
        }
        return;
    }

    if (m_bLooped) {
        const auto totalTime = (float)m_pTimes[m_nNumKeys - 1] * TIME_SCALE;
        delta -= (float)(int32)(delta / totalTime) * totalTime;
    }

    for (auto k = 1; k < m_nNumKeys; k++) {
        const auto time = (float)m_pTimes[k] * TIME_SCALE;
        if (delta < time) {
            const auto prevTime = (float)m_pTimes[k - 1] * TIME_SCALE;
            const auto t        = (delta - prevTime) / (time - prevTime);
            for (auto i = 0; i < count; i++) {
                const auto prev = (float)m_Keys[i][k - 1] * KEY_SCALE;
                const auto curr = (float)m_Keys[i][k] * KEY_SCALE;
                outValues[i] = (curr - prev) * t + prev;
            }
            return;
        }
    }

    // Past the last key
    for (auto i = 0; i < count; i++) {
        outValues[i] = (float)m_Keys[i][m_nNumKeys - 1] * KEY_SCALE;
    }
}
