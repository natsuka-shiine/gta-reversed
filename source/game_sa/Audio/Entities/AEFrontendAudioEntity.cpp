#include "StdInc.h"

#include "AEFrontendAudioEntity.h"

#include "AETwinLoopSoundEntity.h"
#include "AEAudioHardware.h"
#include "AESoundManager.h"
#include "AEAudioUtility.h"
#include "AESound.h"

// 0x5B9AB0
void CAEFrontendAudioEntity::Initialise() {
    m_bAmplifierWakeUp = false;
    m_pAmplifierWakeUp = nullptr;

    AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_FRONTEND_MENU, SND_BANK_SLOT_FRONTEND_MENU);
    AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME);
    AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_BULLET_PASS_1, SND_BANK_SLOT_BULLET_PASS);

    m_nbLoadingTuneSeed = CAEAudioUtility::GetRandomNumberInRange(0, 3);
    AEAudioHardware.LoadSound(SND_BANK_GENRL_LOADING, 2 * m_nbLoadingTuneSeed + 0, SND_BANK_SLOT_COLLISIONS);
    AEAudioHardware.LoadSound(SND_BANK_GENRL_LOADING, 2 * m_nbLoadingTuneSeed + 1, SND_BANK_SLOT_WEAPON_GEN);
}

// 0x4DD440
void CAEFrontendAudioEntity::Reset() {
    m_nLastFrameGeneral_or_nFrameCount = 0;
    m_nLastFrameMissionComplete        = 0;
    m_nLastFrameBulletPass             = 0;
    m_BulletPassCount                  = 0;
    m_f7E                              = -1;
    AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_SCANNER_NOISE_START, this);
}

// 0x4DD4A0
void CAEFrontendAudioEntity::AddAudioEvent(eAudioEvents event, float fVolumeBoost, float fSpeed) {
    // Commonly used flag combinations
    constexpr uint32 FLAGS_BASE              = SOUND_IS_DUCKABLE | SOUND_IS_PAUSABLE | SOUND_PLAY_PHYSICALLY | SOUND_IS_CANCELLABLE | SOUND_FRONT_END; // 0x11B
    constexpr uint32 FLAGS_STEREO            = FLAGS_BASE | SOUND_FORCED_FRONT;                                                                         // 0x111B
    constexpr uint32 FLAGS_STEREO_ROLLED_OFF = FLAGS_STEREO | SOUND_ROLLED_OFF;                                                                         // 0x151B
    constexpr uint32 FLAGS_STEREO_UPDATED    = SOUND_FORCED_FRONT | SOUND_IS_DUCKABLE | SOUND_PLAY_PHYSICALLY | SOUND_REQUEST_UPDATES | SOUND_IS_CANCELLABLE | SOUND_FRONT_END; // 0x110F

    const CVector LEFT{ -1.0f, 0.0f, 0.0f }, RIGHT{ +1.0f, 0.0f, 0.0f }, FRONT{ 0.0f, 1.0f, 0.0f };

    const auto volume = GetDefaultVolume(event) + fVolumeBoost;

    const auto Play = [&](eSoundBankSlot slot, int32 sfx, CVector pos, uint32 flags, float speed, int32 eventId = AE_UNDEFINED, float freqVariance = 0.0f) {
        return AESoundManager.PlaySound({
            .BankSlotID        = slot,
            .SoundID           = (eSoundID)sfx,
            .AudioEntity       = this,
            .Pos               = pos,
            .Volume            = volume,
            .RollOffFactor     = 1.0f,
            .Speed             = speed,
            .Doppler           = 1.0f,
            .FrameDelay        = 0,
            .Flags             = flags,
            .FrequencyVariance = freqVariance,
            .PlayTime          = 0,
            .EventID           = eventId,
        });
    };
    // Play a pair of sounds, one on the left and one on the right
    const auto PlayStereo = [&](eSoundBankSlot slot, int32 sfxLeft, int32 sfxRight, uint32 flags, float speed, int32 eventId = AE_UNDEFINED) {
        Play(slot, sfxLeft, LEFT, flags, speed, eventId);
        Play(slot, sfxRight, RIGHT, flags, speed, eventId);
    };
    const auto IsGameBankLoaded = [] {
        return AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_FRONTEND_GAME, SND_BANK_SLOT_FRONTEND_GAME);
    };
    const auto IsMenuBankLoaded = [] {
        return AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_FRONTEND_MENU, SND_BANK_SLOT_FRONTEND_MENU);
    };
    // Check (and update) the frame counter, so that sounds aren't played too frequently
    const auto CheckFrameDelay = [](uint32& lastFrame) {
        if (CTimer::GetFrameCounter() < lastFrame + 5) {
            return false;
        }
        lastFrame = CTimer::GetFrameCounter();
        return true;
    };
    const auto ProcessBulletPass = [&](CVector pos) {
        if (!AEAudioHardware.IsSoundBankLoaded(m_BulletPassBank, SND_BANK_SLOT_BULLET_PASS)) {
            return;
        }
        if (CTimer::GetFrameCounter() < m_nLastFrameBulletPass + 5) {
            return;
        }
        if (m_BulletPassCount <= 10) {
            m_nLastFrameBulletPass = CTimer::GetFrameCounter();
            Play(
                SND_BANK_SLOT_BULLET_PASS,
                CAEAudioUtility::GetRandomNumberInRange(0, 2),
                pos,
                SOUND_REQUEST_UPDATES | SOUND_IS_CANCELLABLE | SOUND_FRONT_END,
                1.0f,
                event,
                0.03125f
            );
            m_BulletPassCount++;
        } else if (!AESoundManager.AreSoundsPlayingInBankSlot(SND_BANK_SLOT_BULLET_PASS)) { // Load the next bank (once all the sounds have finished playing)
            m_BulletPassBank = (eSoundBank)(m_BulletPassBank + 1);
            if (m_BulletPassBank > SND_BANK_GENRL_BULLET_PASS_LAST) {
                m_BulletPassBank = SND_BANK_GENRL_BULLET_PASS_FIRST;
            }
            AEAudioHardware.LoadSoundBank(m_BulletPassBank, SND_BANK_SLOT_BULLET_PASS);
            m_BulletPassCount = 0;
        }
    };

    switch (event) {
    case AE_FRONTEND_START: { // 0x4DD566
        if (IsGameBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 25, 26, FLAGS_STEREO_ROLLED_OFF, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_SELECT: { // 0x4DD5F6
        if (IsMenuBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_MENU, 6, 7, FLAGS_STEREO_ROLLED_OFF, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_BACK: { // 0x4DD6D6
        if (IsMenuBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_MENU, 0, 1, FLAGS_STEREO_ROLLED_OFF, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_HIGHLIGHT: { // 0x4DD766
        if (IsMenuBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_MENU, 4, 5, FLAGS_STEREO, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_ERROR: { // 0x4DD7F6
        if (IsMenuBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_MENU, 2, 3, FLAGS_STEREO_ROLLED_OFF, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_NOISE_TEST: { // 0x4DD886
        if (IsMenuBankLoaded()) {
            Play(SND_BANK_SLOT_FRONTEND_MENU, 8, FRONT, FLAGS_BASE, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_PICKUP_WEAPON:
    case AE_FRONTEND_CAR_FIT_BOMB_TIMED:
    case AE_FRONTEND_CAR_FIT_BOMB_BOOBY_TRAPPED:
    case AE_FRONTEND_CAR_FIT_BOMB_REMOTE_CONTROLLED:
    case AE_FRONTEND_PURCHASE_WEAPON: {
        if (IsGameBankLoaded() && CheckFrameDelay(m_nLastFrameGeneral_or_nFrameCount)) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 27, 28, FLAGS_STEREO, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_PICKUP_MONEY:
    case AE_FRONTEND_PICKUP_HEALTH:
    case AE_FRONTEND_PICKUP_ADRENALINE:
    case AE_FRONTEND_PICKUP_BODY_ARMOUR: {
        if (IsGameBankLoaded() && CheckFrameDelay(m_nLastFrameGeneral_or_nFrameCount)) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 16, 17, FLAGS_STEREO, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_PICKUP_INFO:
    case AE_FRONTEND_DISPLAY_INFO: {
        if (IsGameBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 14, 15, FLAGS_STEREO_ROLLED_OFF, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_PICKUP_DRUGS:
    case AE_FRONTEND_PICKUP_COLLECTABLE1:
    case AE_FRONTEND_PART_MISSION_COMPLETE: {
        if (IsGameBankLoaded() && CheckFrameDelay(m_nLastFrameMissionComplete)) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 18, 19, FLAGS_STEREO_ROLLED_OFF, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_CAR_NO_CASH:
    case AE_FRONTEND_CAR_IS_HOT:
    case AE_FRONTEND_CAR_ALREADY_RIGGED: {
        if (IsGameBankLoaded() && CheckFrameDelay(m_nLastFrameGeneral_or_nFrameCount)) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 27, 28, FLAGS_STEREO, 0.8409f); // Same as the pickup sound, but lower pitch. `fSpeed` is ignored
        }
        break;
    }
    case AE_FRONTEND_CAR_RESPRAY: {
        if (AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_WEAPONS, SND_BANK_SLOT_WEAPON_GEN)) {
            const auto leftIsHigher = CAEAudioUtility::ResolveProbability(0.5f);
            Play(SND_BANK_SLOT_WEAPON_GEN, SND_GENRL_WEAPONS_SPRAY_PAINT, LEFT, FLAGS_STEREO_UPDATED, leftIsHigher ? 1.1892101f : 1.0f, AE_FRONTEND_CAR_RESPRAY);
            Play(SND_BANK_SLOT_WEAPON_GEN, SND_GENRL_WEAPONS_SPRAY_PAINT, RIGHT, FLAGS_STEREO_UPDATED, leftIsHigher ? 1.0f : 1.1892101f, AE_FRONTEND_CAR_RESPRAY);
            m_nLastTimeCarRespray = CTimer::GetTimeInMS();
        }
        break;
    }
    case AE_FRONTEND_BULLET_PASS_LEFT_REAR: {
        ProcessBulletPass({ -0.1f, -1.0f, 0.0f });
        break;
    }
    case AE_FRONTEND_BULLET_PASS_LEFT_FRONT: {
        ProcessBulletPass({ -0.1f, +1.0f, 0.0f });
        break;
    }
    case AE_FRONTEND_BULLET_PASS_RIGHT_REAR: {
        ProcessBulletPass({ +0.1f, -1.0f, 0.0f });
        break;
    }
    case AE_FRONTEND_BULLET_PASS_RIGHT_FRONT: {
        ProcessBulletPass({ +0.1f, +1.0f, 0.0f });
        break;
    }
    case AE_FRONTEND_WAKEUP_AMPLIFIER: { // 0x4DE1D8
        if (!m_bAmplifierWakeUp && IsGameBankLoaded()) {
            m_pAmplifierWakeUp = Play(SND_BANK_SLOT_FRONTEND_GAME, 8, FRONT, FLAGS_BASE | SOUND_REQUEST_UPDATES, fSpeed);
            m_bAmplifierWakeUp = m_pAmplifierWakeUp != nullptr;
        }
        break;
    }
    case AE_FRONTEND_TIMER_COUNT: { // 0x4DE26D
        if (AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_TIMER_COUNT, this)) {
            m_nLatestTimerCount = CTimer::GetTimeInMS();
        } else if (IsGameBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 4, 5, FLAGS_STEREO_UPDATED, fSpeed, AE_FRONTEND_TIMER_COUNT);
            m_nLatestTimerCount = CTimer::GetTimeInMS();
        }
        break;
    }
    case AE_FRONTEND_RADIO_RETUNE_START: { // 0x4DE387
        if (!IsGameBankLoaded()) {
            break;
        }
        // There are 2 instances of the same sound, one that's used when the game is paused [and thus has to be unpausable]
        const auto PlayRetune = [&](CAETwinLoopSoundEntity& retune, uint32 flags) {
            if (retune.IsActive()) {
                return;
            }
            retune.Initialise(SND_BANK_SLOT_FRONTEND_GAME, 2, 1, this, 200, 650, -1, -1);
            retune.PlayTwinLoopSound(FRONT, volume, 1.0f, 1.0f, 1.0f, (eSoundEnvironment)flags);
        };
        if (CTimer::GetIsPaused()) {
            PlayRetune(m_objRetunePaused, SOUND_IS_DUCKABLE | SOUND_MUSIC_MASTERED | SOUND_IS_PAUSABLE | SOUND_PLAY_PHYSICALLY | SOUND_IS_CANCELLABLE | SOUND_FRONT_END); // 0x15B
        } else {
            PlayRetune(m_objRetune, SOUND_IS_DUCKABLE | SOUND_MUSIC_MASTERED | SOUND_PLAY_PHYSICALLY | SOUND_IS_CANCELLABLE | SOUND_FRONT_END); // 0x14B
        }
        break;
    }
    case AE_FRONTEND_RADIO_RETUNE_STOP: { // 0x4DE48A
        auto& retune = CTimer::GetIsPaused()
            ? m_objRetunePaused
            : m_objRetune;
        if (retune.IsActive()) {
            retune.StopSoundAndForget();
        }
        break;
    }
    case AE_FRONTEND_RADIO_RETUNE_STOP_PAUSED: { // 0x4DE4AF
        if (m_objRetunePaused.IsActive()) {
            m_objRetunePaused.StopSoundAndForget();
        }
        break;
    }
    case AE_FRONTEND_RADIO_CLICK_ON: { // 0x4DE4CC
        if (IsGameBankLoaded()) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, 23, FRONT, FLAGS_BASE, 1.0f);
        }
        break;
    }
    case AE_FRONTEND_RADIO_CLICK_OFF: { // 0x4DE537
        if (IsGameBankLoaded()) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, 23, FRONT, FLAGS_BASE, 0.8909f);
        }
        break;
    }
    case AE_FRONTEND_FIRE_FAIL_SNIPERRIFFLE: { // 0x4DE5A2
        if (IsGameBankLoaded()) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, 10, FRONT, SOUND_IS_CANCELLABLE | SOUND_FRONT_END, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_FIRE_FAIL_ROCKET: { // 0x4DE61A
        if (IsGameBankLoaded()) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, 11, FRONT, SOUND_IS_CANCELLABLE | SOUND_FRONT_END, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_RACE_321: { // 0x4DE7D4
        if (IsGameBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 6, 7, FLAGS_STEREO_ROLLED_OFF, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_RACE_GO: { // 0x4DE894
        if (IsGameBankLoaded()) {
            PlayStereo(SND_BANK_SLOT_FRONTEND_GAME, 12, 13, FLAGS_STEREO_ROLLED_OFF, fSpeed);
        }
        break;
    }
    case AE_FRONTEND_BUY_CAR_MOD: { // 0x4DDE1F
        if (IsGameBankLoaded()) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, 9, FRONT, FLAGS_BASE, 1.0f, AE_UNDEFINED, 0.05883f);
        }
        break;
    }
    case AE_FRONTEND_SCANNER_NOISE_START: { // 0x4DEA21
        if (IsGameBankLoaded() && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_SCANNER_NOISE_START, this)) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, 3, FRONT, SOUND_IS_DUCKABLE | SOUND_IS_CANCELLABLE | SOUND_FRONT_END, fSpeed, AE_FRONTEND_SCANNER_NOISE_START);
        }
        break;
    }
    case AE_FRONTEND_SCANNER_NOISE_STOP: {
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_SCANNER_NOISE_START, this);
        break;
    }
    case AE_FRONTEND_SCANNER_CLICK: {
        if (IsGameBankLoaded() && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_SCANNER_CLICK, this)) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, 24, FRONT, SOUND_IS_DUCKABLE | SOUND_IS_CANCELLABLE | SOUND_FRONT_END, fSpeed, AE_FRONTEND_SCANNER_CLICK);
        }
        break;
    }
    case AE_FRONTEND_LOADING_TUNE_START: {
        const auto sfxLeft = 2 * m_nbLoadingTuneSeed, sfxRight = 2 * m_nbLoadingTuneSeed + 1;
        if (   AEAudioHardware.IsSoundLoaded(SND_BANK_GENRL_LOADING, sfxLeft, SND_BANK_SLOT_COLLISIONS)
            && AEAudioHardware.IsSoundLoaded(SND_BANK_GENRL_LOADING, sfxRight, SND_BANK_SLOT_WEAPON_GEN)
            && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_LOADING_TUNE_START, this)
        ) {
            Play(SND_BANK_SLOT_COLLISIONS, sfxLeft, LEFT, FLAGS_STEREO, fSpeed, AE_FRONTEND_LOADING_TUNE_START);
            Play(SND_BANK_SLOT_WEAPON_GEN, sfxRight, RIGHT, FLAGS_STEREO, fSpeed, AE_FRONTEND_LOADING_TUNE_START);
        }
        break;
    }
    case AE_FRONTEND_LOADING_TUNE_STOP: {
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_LOADING_TUNE_START, this);
        break;
    }
    case AE_MISSILE_LOCK: { // 0x4DE9AB
        if (IsGameBankLoaded() && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_MISSILE_LOCK, this)) {
            Play(SND_BANK_SLOT_FRONTEND_GAME, 13, FRONT, SOUND_IS_DUCKABLE | SOUND_FRONT_END, fSpeed, AE_MISSILE_LOCK);
        }
        break;
    }
    default:
        break;
    }
}

// 0x4DD480
bool CAEFrontendAudioEntity::IsRadioTuneSoundActive() {
    return CTimer::GetIsPaused()
        ? m_objRetunePaused.IsActive()
        : m_objRetune.IsActive();
}

// 0x4DD470
bool CAEFrontendAudioEntity::IsLoadingTuneActive() {
    return AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_FRONTEND_LOADING_TUNE_START, this);
}

// 0x4DEDA0
void CAEFrontendAudioEntity::UpdateParameters(CAESound* sound, int16 curPlayPos) {
    if (!sound) {
        return;
    }

    // Pans bullet-pass sounds left/right and sweeps them front<->rear as `curPlayPos` runs 0..350
    const auto BulletSound = [&](float x, float y) {
        if (curPlayPos >= 0 && curPlayPos <= 350) {
            sound->SetPosition({ x, y, 0.0f });
        }
    };

    switch (sound->m_Event) {
    case AE_FRONTEND_CAR_RESPRAY:
        if (curPlayPos > 0 && CTimer::GetTimeInMS() > m_nLastTimeCarRespray + 1900) {
            sound->StopSoundAndForget();
        }
        break;
    case AE_FRONTEND_BULLET_PASS_LEFT_REAR:
        BulletSound(-0.1f, 2.0f * (float)curPlayPos / 350.0f - 1.0f);
        break;
    case AE_FRONTEND_BULLET_PASS_LEFT_FRONT:
        BulletSound(-0.1f, 1.0f - 2.0f * (float)curPlayPos / 350.0f);
        break;
    case AE_FRONTEND_BULLET_PASS_RIGHT_REAR:
        BulletSound(+0.1f, 2.0f * (float)curPlayPos / 350.0f - 1.0f);
        break;
    case AE_FRONTEND_BULLET_PASS_RIGHT_FRONT:
        BulletSound(+0.1f, 1.0f - 2.0f * (float)curPlayPos / 350.0f);
        break;
    case AE_FRONTEND_TIMER_COUNT:
        if (curPlayPos > 0 && CTimer::GetTimeInMS() > m_nLatestTimerCount + 100) {
            sound->StopSoundAndForget();
        }
        break;
    default:
        break;
    }

    if (sound == m_pAmplifierWakeUp && curPlayPos == -1) {
        m_pAmplifierWakeUp = nullptr;
        m_bAmplifierWakeUp = false;
    }
}

void CAEFrontendAudioEntity::InjectHooks() {
    RH_ScopedVirtualClass(CAEFrontendAudioEntity, 0x862E54, 1);
    RH_ScopedCategory("Audio/Entities");

    RH_ScopedInstall(Initialise, 0x5B9AB0);
    RH_ScopedInstall(Reset, 0x4DD440);
    RH_ScopedInstall(AddAudioEvent, 0x4DD4A0);
    RH_ScopedInstall(IsRadioTuneSoundActive, 0x4DD480);
    RH_ScopedInstall(IsLoadingTuneActive, 0x4DD470);
    RH_ScopedVMTInstall(UpdateParameters, 0x4DEDA0);
}
