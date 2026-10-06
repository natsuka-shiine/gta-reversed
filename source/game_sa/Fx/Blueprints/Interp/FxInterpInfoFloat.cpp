#include "StdInc.h"

#include "FxInterpInfoFloat.h"
#include "FxManager.h"

void FxInterpInfoFloat_c::InjectHooks() {
    RH_ScopedClass(FxInterpInfoFloat_c);
    RH_ScopedCategory("Fx");

    RH_ScopedOverloadedInstall(GetVal, "Integrate", 0x4A85C0, float(FxInterpInfoFloat_c::*)(int32, float, float));
    RH_ScopedOverloadedInstall(GetVal, "Values", 0x4A8470, void(FxInterpInfoFloat_c::*)(float*, float));
}

// 0x4A8440
FxInterpInfoFloat_c::FxInterpInfoFloat_c() : FxInterpInfo_c() {
    m_Keys = nullptr;
}

// 0x5C16F0
void FxInterpInfoFloat_c::Load(FILESTREAM file) {
    for (auto i = 0; i < m_nCount; i++) {
        ReadField<void>(file);
        ReadField<void>(file, "FX_INTERP_DATA:");

        m_bLooped  = ReadField<bool>(file);
        m_nNumKeys = ReadField<int8>(file);

        if (i == 0) {
            m_pTimes = g_fxMan.Allocate<uint16>(m_nNumKeys);
        }

        m_Keys[i] = g_fxMan.Allocate<float>(m_nNumKeys);
        for (auto j = 0; j < m_nNumKeys; j++) {
            ReadField<void>(file, "FX_KEYFLOAT_DATA:");
            m_pTimes[j] = uint16(ReadField<float>(file) * 256.f);
            m_Keys[i][j] = ReadField<float>(file);
        }
    }
}

// NOTSA
void FxInterpInfoFloat_c::Allocate(int32 count) {
    m_nCount = count;
    m_Keys = g_fxMan.Allocate<float*>(count);
}

// 0x4A85C0
// Integrates the (piecewise linear) curve of `attrib` over the interval [time - deltaTime, time]
float FxInterpInfoFloat_c::GetVal(int32 attrib, float time, float deltaTime) {
    constexpr auto TIME_SCALE = 1.0f / 256.0f; // 0x859AA0

    const auto keys    = m_Keys[attrib];
    const auto numKeys = (int32)m_nNumKeys;

    if (numKeys == 1) {
        return deltaTime * keys[0];
    }

    auto result  = 0.0f;
    auto currT   = time - deltaTime; // Start of the interval
    auto currVal = time;             // Value of the curve at `currT` (The game really does initialize it to `time`, but it's unused in that case)

    // Find the first key that's after the interval's start, and calculate the value there
    auto k = 0;
    for (; k < numKeys; k++) {
        const auto keyT = (float)m_pTimes[k] * TIME_SCALE;
        if (currT < keyT) {
            if (k > 0) {
                const auto prevT = (float)m_pTimes[k - 1] * TIME_SCALE;
                currVal = (currT - prevT) / (keyT - prevT) * (keys[k] - keys[k - 1]) + keys[k - 1];
            } else {
                currVal = keys[k];
            }
            break;
        }
    }

    if (k == numKeys) { // Interval starts after the last key
        return deltaTime * keys[k - 1];
    }

    // Now sum up the area (trapezoids) up to `time`
    for (; k < numKeys; k++) {
        const auto keyT = (float)m_pTimes[k] * TIME_SCALE;
        if (keyT == time) {
            result += ((keys[k] - currVal) * 0.5f + currVal) * (keyT - currT);
            break;
        }
        if (keyT < time) {
            result += ((keys[k] - currVal) * 0.5f + currVal) * (keyT - currT);
            currT   = keyT;
            currVal = keys[k];
            continue;
        }
        if (keyT > time) {
            const auto prevT  = (float)m_pTimes[k - 1] * TIME_SCALE;
            const auto endVal = (time - prevT) / (keyT - prevT) * (keys[k] - keys[k - 1]) + keys[k - 1];
            result += ((endVal - currVal) * 0.5f + currVal) * (time - currT);
            break;
        }
    }

    if (k == numKeys) { // Interval ends after the last key
        result += ((keys[k - 1] - currVal) * 0.5f + currVal) * ((float)m_pTimes[k - 1] * TIME_SCALE - currT);
    }

    return result;
}

// 0x4A8470
void FxInterpInfoFloat_c::GetVal(float* outValues, float delta) {
    constexpr auto TIME_SCALE = 1.0f / 256.0f; // 0x859AA0

    const auto count = (int8)m_nCount; // The game reads this as a signed byte

    if (m_nNumKeys == 1) {
        for (auto i = 0; i < count; i++) {
            outValues[i] = m_Keys[i][0];
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
                const auto prev = m_Keys[i][k - 1];
                const auto curr = m_Keys[i][k];
                outValues[i] = (curr - prev) * t + prev;
            }
            return;
        }
    }

    // Past the last key
    for (auto i = 0; i < count; i++) {
        outValues[i] = m_Keys[i][m_nNumKeys - 1];
    }
}
