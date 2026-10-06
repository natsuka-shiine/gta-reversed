#include "StdInc.h"

#include "AEScriptAudioEntity.h"
#include "AESoundManager.h"
#include "AEAudioUtility.h"
#include "AEAudioHardware.h"
#include "AEAmbienceTrackManager.h"
#include "AEVehicleAudioEntity.h"

// 0x5074D0
CAEScriptAudioEntity::CAEScriptAudioEntity() : CAEAudioEntity() {
    m_nLastTimeHornPlayed = 0;
    field_7E = 0;
    field_7C = 0;
    m_Volume = 0.0f;
    m_Speed = 1.0f;
    field_7D = 0;
    field_8C = 0.0f;
}

// 0x5B9B60
void CAEScriptAudioEntity::Initialise() {
    for (auto& link : wavLinks) {
        link.Init();
    }
}

// 0x4EC150
void CAEScriptAudioEntity::Reset() {
    for (auto i = 0; i < MISSION_AUDIO_COUNT; i++) {
        ClearMissionAudio(i);
    }
    field_7C = 0;
    m_Entity = nullptr;
    field_7D = 0;
    field_8C = 2.0f;
}

// 0x0
void CAEScriptAudioEntity::AddAudioEvent(int32) {
    /* Android NOP */
}

// 0x4EC100
CVector* CAEScriptAudioEntity::AttachMissionAudioToPhysical(uint8 sampleId, CPhysical* physical) {
    auto& link = wavLinks[sampleId];
    link.m_pEntity   = physical;
    link.m_vPosition = CVector{-1000.0f, -1000.0f, -1000.0f};
    return &link.m_vPosition;
}

// 0x4EC040
void CAEScriptAudioEntity::ClearMissionAudio(uint8 sampleId) {
    if (sampleId < MISSION_AUDIO_COUNT) {
        AESoundManager.CancelSoundsInBankSlot((eSoundBankSlot)(SND_BANK_SLOT_MISSION1 + sampleId), true);

        auto& link = wavLinks[sampleId];
        link.m_pEntity   = nullptr;
        link.m_Sound     = nullptr;
        link.m_vPosition = CVector{-1000.0f, -1000.0f, -1000.0f};
    }
}

// 0x4EBFE0
bool CAEScriptAudioEntity::IsMissionAudioSampleFinished(uint8 sampleId) {
    if (sampleId > 3) {
        return true;
    }
    if (sampleId > 1) {
        return !AESoundManager.AreSoundsPlayingInBankSlot((eSoundBankSlot)(SND_BANK_SLOT_MISSION1 + sampleId));
    }
    return wavLinks[sampleId].m_Sound == nullptr;
}

// 0x4EBF60
int8 CAEScriptAudioEntity::GetMissionAudioLoadingStatus(uint8 sampleId) {
    if (sampleId < MISSION_AUDIO_COUNT) {
        const auto& link = wavLinks[sampleId];
        if (link.m_nBankId >= 0) {
            if (link.m_nBankSlotId >= 0) {
                return AEAudioHardware.GetSoundLoadingStatus(
                    (eSoundBank)link.m_nBankId,
                    (eSoundID)link.m_nBankSlotId,
                    (eSoundBankSlot)(SND_BANK_SLOT_MISSION1 + sampleId)
                );
            }
            return AEAudioHardware.GetSoundBankLoadingStatus(
                (eSoundBank)link.m_nBankId,
                (eSoundBankSlot)(SND_BANK_SLOT_MISSION1 + sampleId)
            );
        }
    }
    return 1;
}

// 0x4EC020
int32 CAEScriptAudioEntity::GetMissionAudioEvent(uint8 sampleId) {
    return wavLinks[sampleId].m_nAudioEvent;
}

// 0x4EC0C0
void CAEScriptAudioEntity::SetMissionAudioPosition(uint8 sampleId, CVector& posn) {
    auto& link = wavLinks[sampleId];
    link.m_vPosition = posn;
    link.m_pEntity   = nullptr;
}

// 0x4EC4D0
CVector* CAEScriptAudioEntity::GetMissionAudioPosition(uint8 sampleId) {
    auto& link = wavLinks[sampleId];
    if (link.m_pEntity) {
        return &link.m_pEntity->GetPosition();
    }
    if (link.m_vPosition != CVector{-1000.0f, -1000.0f, -1000.0f} && !link.m_vPosition.IsZero()) {
        return &link.m_vPosition;
    }
    return nullptr;
}

// 0x4EC6D0
void CAEScriptAudioEntity::PlayMissionBankSound(eAudioEvents eventId, CVector& posn, CPhysical* physical, int16 sfxId, uint8 linkId, uint8 a7, float volume, float maxDistance, float speed) {
    if (linkId < 2 || linkId >= MISSION_AUDIO_COUNT) {
        return;
    }
    if (a7 && AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(eventId, this)) {
        return;
    }

    const auto  bankSlot = (eSoundBankSlot)(SND_BANK_SLOT_MISSION1 + linkId);
    const auto& link     = wavLinks[linkId];
    if (!AEAudioHardware.IsSoundBankLoaded((eSoundBank)link.m_nBankId, bankSlot)) {
        return;
    }

    bool bFrontend = false;
    const auto pos = [&]() -> CVector {
        if (physical) {
            return physical->GetPosition();
        }
        if (posn == CVector{-1000.0f, -1000.0f, -1000.0f} || posn.IsZero()) {
            bFrontend = true;
            return CVector{0.0f, 1.0f, 0.0f};
        }
        if (posn == CVector{-1.0f, 0.0f, 0.0f} || posn == CVector{1.0f, 0.0f, 0.0f}) {
            bFrontend = true;
        }
        return posn;
    }();

    auto& sound = m_tempSound;
    sound.Initialise(
        bankSlot,
        (eSoundID)link.m_nBankSlotId,
        this,
        pos,
        GetDefaultVolume(eventId) + volume,
        maxDistance,
        speed
    );
    sound.m_Flags = (uint16)(SOUND_IS_CANCELLABLE | SOUND_REQUEST_UPDATES);
    sound.SetFlags(SOUND_FRONT_END, bFrontend);
    if (physical) {
        sound.SetFlags(SOUND_LIFESPAN_TIED_TO_PHYSICAL_ENTITY, true);
        sound.RegisterWithPhysicalEntity(physical);
    }
    sound.m_Event = eventId;

    AESoundManager.RequestNewSound(&sound);
}

// event eAudioEvents
// 0x4EC550
void CAEScriptAudioEntity::PlayResidentSoundEvent(eSoundBankSlot slot, eSoundBank bank, eSoundID sfx, eAudioEvents event, CVector& posn, CPhysical* physical, float vol, float speed, int16 playPosn, float maxDistance) {
    if (!AEAudioHardware.IsSoundBankLoaded(bank, slot)) {
        return;
    }

    bool bFrontend = false;
    const auto volume = GetDefaultVolume(static_cast<eAudioEvents>(event)) + vol;
    CVector pos = [&] {
        if (physical) {
            return physical->GetPosition();
        } else if (posn == -1000.0f || posn.IsZero()) {
            bFrontend = true;
            return CVector{0.0f, 1.0f, 0.0f};
        } else {
            return posn;
        }
    }();

    AESoundManager.PlaySound({
        .BankSlotID         = slot,
        .SoundID            = sfx,
        .AudioEntity        = this,
        .Pos                = pos,
        .Volume             = volume,
        .RollOffFactor      = maxDistance,
        .Speed              = speed,
        .Doppler            = 1.0f,
        .FrameDelay         = 0,
        .Flags              = SOUND_START_PERCENTAGE | SOUND_REQUEST_UPDATES | SOUND_IS_CANCELLABLE | (bFrontend ? SOUND_FRONT_END : 0u),
        .FrequencyVariance  = 0.0f,
        .PlayTime           = playPosn,
        .RegisterWithEntity = physical,
        .EventID            = (eAudioEvents)(event),
    });
}

// 0x4EC270
void CAEScriptAudioEntity::PlayLoadedMissionAudio(uint8 sampleId) {
    if (sampleId > 3) {
        return;
    }

    auto& link = wavLinks[sampleId];
    if (link.m_nBankId < 0 || link.m_nBankSlotId < 0) {
        return;
    }
    if (GetMissionAudioLoadingStatus(sampleId) != 1) {
        return;
    }

    bool  bDuck = false;
    float volume;
    if (link.m_nAudioEvent == AE_SCRIPT_SLOT_USE_CUSTOM) {
        volume = -100.0f;
    } else {
        volume = GetDefaultVolume((eAudioEvents)link.m_nAudioEvent);
        if (sampleId < 2) {
            bDuck = true;
            if (volume == -128.0f) {
                volume = 6.0f;
            }
        }
    }

    bool bFrontend = false;
    const auto pos = [&]() -> CVector {
        if (const auto physical = link.m_pEntity) {
            return physical->GetPosition();
        }
        if (link.m_vPosition == CVector{-1000.0f, -1000.0f, -1000.0f} || link.m_vPosition.IsZero()) {
            bFrontend = true;
            return CVector{0.0f, 1.0f, 0.0f};
        }
        return link.m_vPosition;
    }();

    CAESound sound;
    sound.Initialise(
        (eSoundBankSlot)(SND_BANK_SLOT_MISSION1 + sampleId),
        (eSoundID)link.m_nBankSlotId,
        this,
        pos,
        volume,
        2.0f
    );
    sound.m_Flags = (uint16)(SOUND_IS_CANCELLABLE | SOUND_REQUEST_UPDATES | SOUND_PLAY_PHYSICALLY);
    sound.SetFlags(SOUND_FRONT_END, bFrontend);
    sound.SetFlags(SOUND_IS_DUCKABLE, bDuck);
    sound.SetFlags(SOUND_IS_COMPRESSABLE, bDuck);
    sound.SetFlags(SOUND_SMOOTH_DUCKING, bDuck);

    link.m_Sound = AESoundManager.RequestNewSound(&sound);
}

// 0x4EC190
void CAEScriptAudioEntity::PreloadMissionAudio(uint8 slotId, int32 sampleId) {
    if (slotId < MISSION_AUDIO_COUNT && IsMissionAudioSampleFinished(slotId)) {
        auto& link = wavLinks[slotId];

        // `GetBankAndSoundFromScriptSlotAudioEvent` wants a `eSoundBankS32`, while the link stores the ids as plain ints
        eSoundBankS32 bankId{(eSoundBank)link.m_nBankId};
        int32         soundId = link.m_nBankSlotId;
        if (CAEAudioUtility::GetBankAndSoundFromScriptSlotAudioEvent((eAudioEvents)sampleId, bankId, soundId, slotId)) {
            link.m_nBankId     = bankId.get_underlying();
            link.m_nBankSlotId = soundId;

            const auto bankSlot = (eSoundBankSlot)(SND_BANK_SLOT_MISSION1 + slotId);
            if (soundId < 0) {
                AEAudioHardware.LoadSoundBank((eSoundBank)link.m_nBankId, bankSlot);
            } else {
                AEAudioHardware.LoadSound((eSoundBank)link.m_nBankId, (eSoundID)soundId, bankSlot);
            }

            link.m_nAudioEvent = sampleId;
            link.m_pEntity     = nullptr;
            link.m_vPosition   = CVector{-1000.0f, -1000.0f, -1000.0f};
        }
    }
}

// 0x4ECCF0
void CAEScriptAudioEntity::ProcessMissionAudioEvent(eAudioEvents eventId, CVector& posn, CPhysical* physical, float volume, float speed) {
    // NOTE: `PlaySpecialMissionAmbienceTrack` takes a track id, the function's parameter type is just misleading.
    const auto PlayAmbienceTrack = [](int32 trackId) {
        AEAmbienceTrackManager.PlaySpecialMissionAmbienceTrack((eAudioEvents)trackId);
    };
    const auto CancelEvent = [&](eAudioEvents event) {
        if (physical) {
            AESoundManager.CancelSoundsOfThisEventPlayingForThisEntityAndPhysical(event, this, physical);
        } else {
            AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(event, this);
        }
    };
    const auto AddDoorEvent = [&](eAudioEvents event, float doorSpeed) {
        m_GarageAudio.AddAudioEvent(event, physical ? physical->GetPosition() : posn, 0.0f, doorSpeed);
    };
    const auto PlayCollisionSound = [&](eSoundID sfx, float sfxSpeed = 1.0f, int16 playPosn = 0) {
        PlayResidentSoundEvent(SND_BANK_SLOT_COLLISIONS, SND_BANK_GENRL_COLLISIONS, sfx, eventId, posn, physical, 0.0f, sfxSpeed, playPosn, 1.0f);
    };
    const auto PlayWeaponSound = [&](eSoundID sfx, float sfxVolume = 0.0f) {
        PlayResidentSoundEvent(SND_BANK_SLOT_WEAPON_GEN, SND_BANK_GENRL_WEAPONS, sfx, eventId, posn, physical, sfxVolume, 1.0f, 0, 1.0f);
    };
    const auto PlayCraneSound = [&](eSoundID sfx, float sfxVolume = 0.0f, float sfxSpeed = 1.0f) {
        PlayResidentSoundEvent(SND_BANK_SLOT_PLAYER_ENGINE_P, SND_BANK_GENRL_CRANE_P, sfx, eventId, posn, physical, sfxVolume, sfxSpeed, 0, 2.5f);
    };

    switch (eventId) {
    case AE_CRANE_WINCH_MOVE: // 0x68
        if (field_7D) {
            if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntityAndPhysical(eventId, this, physical)) {
                PlayCraneSound(1, volume, speed);
            }
            m_Speed               = speed;
            m_Volume              = GetDefaultVolume(eventId) + volume;
            m_nLastTimeHornPlayed = CTimer::GetTimeInMS();
        }
        break;
    case AE_SCRIPT_DISABLE_HELI_AUDIO: // 0x3E8
        CAEVehicleAudioEntity::DisableHelicoptors();
        break;
    case AE_SCRIPT_ENABLE_HELI_AUDIO: // 0x3E9
        CAEVehicleAudioEntity::EnableHelicoptors();
        break;
    case AE_SCRIPT_CEILING_VENT_LAND: // 0x3EA
        PlayCollisionSound(64, 0.79f, 35);
        break;
    case AE_SCRIPT_CLAXON_START: // 0x3ED
        PlayMissionBankSound(eventId, posn, physical, 1, 2);
        break;
    case AE_SCRIPT_CLAXON_STOP: // 0x3EE
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_CLAXON_START, this);
        break;
    case AE_SCRIPT_BLAST_DOOR_SLIDE_START: // 0x3EF
        PlayMissionBankSound(eventId, posn, physical, 0, 2);
        break;
    case AE_SCRIPT_BLAST_DOOR_SLIDE_STOP: // 0x3F0
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_BLAST_DOOR_SLIDE_START, this);
        break;
    case AE_SCRIPT_BONNET_DENT:              // 0x3F1
    case AE_SCRIPT_CAR_SMASH_CAR:            // 0x474
    case AE_SCRIPT_MAGNET_VEHICLE_COLLISION: // 0x47C
        PlayCollisionSound((eSoundID)CAEAudioUtility::GetRandomNumberInRange(20, 28));
        break;
    case AE_SCRIPT_BASKETBALL_BOUNCE: // 0x3F2
        PlayMissionBankSound(eventId, posn, physical, (int16)CAEAudioUtility::GetRandomNumberInRange(0, 2), 3);
        break;
    case AE_SCRIPT_BASKETBALL_HIT_HOOP: // 0x3F3
        PlayMissionBankSound(eventId, posn, physical, 3, 3);
        break;
    case AE_SCRIPT_BASKETBALL_SCORE: // 0x3F4
        if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(eventId, this)) {
            PlayMissionBankSound(eventId, posn, physical, 4, 3);
        }
        break;
    case AE_SCRIPT_POOL_HIT_WHITE: // 0x3F6
        PlayMissionBankSound(eventId, posn, physical, 10, 3);
        break;
    case AE_SCRIPT_POOL_HIT_CUSHION: // 0x3F8
        PlayMissionBankSound(eventId, posn, physical, 8, 3);
        break;
    case AE_SCRIPT_POOL_BALL_POT: // 0x3F9
        PlayMissionBankSound(eventId, posn, physical, (int16)CAEAudioUtility::GetRandomNumberInRange(3, 5), 3);
        break;
    case AE_SCRIPT_POOL_CHALK_CUE: // 0x3FA
        PlayMissionBankSound(eventId, posn, physical, 7, 3);
        break;
    case AE_SCRIPT_CRANE_ENTER: // 0x3FB
        if (!AEAudioHardware.IsSoundBankLoaded(SND_BANK_GENRL_CRANE_P, SND_BANK_SLOT_PLAYER_ENGINE_P)) {
            if (AESoundManager.AreSoundsPlayingInBankSlot(SND_BANK_SLOT_PLAYER_ENGINE_P)) {
                AESoundManager.CancelSoundsInBankSlot(SND_BANK_SLOT_PLAYER_ENGINE_P, false);
            }
            AEAudioHardware.LoadSoundBank(SND_BANK_GENRL_CRANE_P, SND_BANK_SLOT_PLAYER_ENGINE_P);
        }
        m_Physical = physical;
        field_7D   = 1;
        break;
    case AE_SCRIPT_CRANE_MOVE_START: // 0x3FC
        if (field_7D && !AESoundManager.AreSoundsOfThisEventPlayingForThisEntityAndPhysical(eventId, this, physical)) {
            PlayCraneSound(1);
        }
        break;
    case AE_SCRIPT_CRANE_MOVE_STOP: // 0x3FD
        if (field_7D) {
            CancelEvent(AE_SCRIPT_CRANE_MOVE_START);
            PlayCraneSound(2);
        }
        break;
    case AE_SCRIPT_CRANE_EXIT: // 0x3FE
        if (field_7D) {
            AESoundManager.CancelSoundsInBankSlot(SND_BANK_SLOT_PLAYER_ENGINE_P, true);
            PlayCraneSound(3);
            m_Physical = nullptr;
            field_7D   = 0;
        }
        break;
    case AE_SCRIPT_VIDEO_POKER_PAYOUT: // 0x401
        PlayMissionBankSound(eventId, posn, physical, 1, 3);
        break;
    case AE_SCRIPT_VIDEO_POKER_BUTTON: // 0x402
        PlayMissionBankSound(eventId, posn, physical, 0, 3);
        break;
    case AE_SCRIPT_WHEEL_OF_FORTUNE_CLACKER: // 0x403
        PlayCollisionSound(19, 1.0f, 70);
        break;
    case AE_SCRIPT_KEYPAD_BEEP: // 0x404
        if (AEAudioHardware.IsSoundBankLoaded(SND_BANK_SCRIPT_KEYPAD, SND_BANK_SLOT_MISSION3)) {
            PlayMissionBankSound(eventId, posn, physical, 0, 2);
        } else if (AEAudioHardware.IsSoundBankLoaded(SND_BANK_SCRIPT_UNCLE_SAM, SND_BANK_SLOT_MISSION3)) {
            PlayMissionBankSound(eventId, posn, physical, 3, 2);
        }
        break;
    case AE_SCRIPT_KEYPAD_PASS: // 0x405
        PlayMissionBankSound(eventId, posn, physical, 2, 2);
        break;
    case AE_SCRIPT_KEYPAD_FAIL: // 0x406
        PlayMissionBankSound(eventId, posn, physical, 1, 2);
        break;
    case AE_SCRIPT_SHOOTING_RANGE_TARGET_SHATTER: // 0x407
        PlayMissionBankSound(eventId, posn, physical, (int16)CAEAudioUtility::GetRandomNumberInRange(2, 6), 3);
        break;
    case AE_SCRIPT_SHOOTING_RANGE_TARGET_DROP: // 0x408
        PlayMissionBankSound(eventId, posn, physical, 1, 3);
        break;
    case AE_SCRIPT_SHOOTING_RANGE_TARGET_MOVE_START: // 0x409
        PlayMissionBankSound(eventId, posn, physical, 0, 3);
        break;
    case AE_SCRIPT_SHOOTING_RANGE_TARGET_MOVE_STOP: // 0x40A
        CancelEvent(AE_SCRIPT_SHOOTING_RANGE_TARGET_MOVE_START);
        break;
    case AE_SCRIPT_SHUTTER_DOOR_START: // 0x40B
    case AE_SCRIPT_GARAGE_DOOR_START:  // 0x481
        AddDoorEvent(AE_GARAGE_DOOR_OPENING, 1.0f);
        break;
    case AE_SCRIPT_SHUTTER_DOOR_STOP:      // 0x40C
    case AE_SCRIPT_GARAGE_DOOR_STOP:       // 0x482
    case AE_SCRIPT_SHUTTER_DOOR_SLOW_STOP: // 0x48E
        AddDoorEvent(AE_GARAGE_DOOR_OPENED, 1.0f);
        break;
    case AE_SCRIPT_PARACHUTE_OPEN: // 0x40F
        PlayWeaponSound(65);
        break;
    case AE_SCRIPT_DUAL_SHOOT: // 0x411
        PlayMissionBankSound(eventId, posn, physical, 8, 3);
        break;
    case AE_SCRIPT_DUAL_THRUST:         // 0x412
    case AE_SCRIPT_BEE_BUZZ:            // 0x48F
    case AE_SCRIPT_TEMPEST_SHIELD_GLOW: // 0x499
        PlayMissionBankSound(eventId, posn, physical, 0, 3, 1);
        m_nLastTimeHornPlayed = CTimer::GetTimeInMS();
        break;
    case AE_SCRIPT_DUAL_EXPLOSION_SHORT: // 0x413
        PlayMissionBankSound(eventId, posn, physical, 2, 3, 1);
        break;
    case AE_SCRIPT_DUAL_EXPLOSION_LONG: // 0x414
        PlayMissionBankSound(eventId, posn, physical, 1, 3, 1);
        break;
    case AE_SCRIPT_DUAL_MENU_SELECT: // 0x415
        PlayMissionBankSound(eventId, posn, physical, 5, 3);
        break;
    case AE_SCRIPT_DUAL_MENU_DESELECT: // 0x416
        PlayMissionBankSound(eventId, posn, physical, 4, 3);
        break;
    case AE_SCRIPT_DUAL_GAME_OVER: // 0x417
        PlayMissionBankSound(eventId, posn, physical, 3, 3, 1);
        break;
    case AE_SCRIPT_DUAL_PICKUP_LIGHT: // 0x418
        PlayMissionBankSound(eventId, posn, physical, 7, 3, 1);
        break;
    case AE_SCRIPT_DUAL_PICKUP_DARK: // 0x419
        PlayMissionBankSound(eventId, posn, physical, 6, 3, 1);
        break;
    case AE_SCRIPT_DUAL_TOUCH_DARK: // 0x41A
        PlayMissionBankSound(eventId, posn, physical, 9, 3, 1);
        break;
    case AE_SCRIPT_DUAL_TOUCH_LIGHT: // 0x41B
        PlayMissionBankSound(eventId, posn, physical, 10, 3, 1);
        break;
    case AE_SCRIPT_AMMUNATION_BUY_WEAPON: // 0x41C
    case AE_SCRIPT_SHOP_BUY:              // 0x41E
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PURCHASE_WEAPON, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_AMMUNATION_BUY_WEAPON_DENIED: // 0x41D
    case AE_SCRIPT_SHOP_BUY_DENIED:              // 0x41F
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_NO_CASH, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_RACE_321: // 0x420
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RACE_321, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_RACE_GO: // 0x421
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RACE_GO, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_PART_MISSION_COMPLETE: // 0x422
    case AE_SCRIPT_CHECKPOINT_GREEN:      // 0x472
    case AE_SCRIPT_PROPERTY_PURCHASED:    // 0x47D
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PART_MISSION_COMPLETE, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_GOGO_PLAYER_FIRE: // 0x423
        PlayMissionBankSound(eventId, posn, physical, 5, 3);
        break;
    case AE_SCRIPT_GOGO_ENEMY_FIRE: // 0x424
        PlayMissionBankSound(eventId, posn, physical, 2, 3);
        break;
    case AE_SCRIPT_GOGO_EXPLOSION:     // 0x425
    case AE_SCRIPT_BANDIT_INSERT_COIN: // 0x43F
        PlayMissionBankSound(eventId, posn, physical, 3, 3);
        break;
    case AE_SCRIPT_GOGO_TRACK_START: // 0x426
        PlayAmbienceTrack(153);
        break;
    case AE_SCRIPT_GOGO_TRACK_STOP:          // 0x427
    case AE_SCRIPT_DUAL_TRACK_STOP:          // 0x42D
    case AE_SCRIPT_BEE_TRACK_STOP:           // 0x435
    case AE_SCRIPT_AWARD_TRACK_STOP:         // 0x44A
    case AE_SCRIPT_OTB_TRACK_STOP:           // 0x477
    case AE_SCRIPT_TEMPEST_TRACK_STOP:       // 0x49E
    case AE_SCRIPT_DRIVING_AWARD_TRACK_STOP: // 0x4A0
    case AE_SCRIPT_BIKE_AWARD_TRACK_STOP:    // 0x4A2
    case AE_SCRIPT_PILOT_AWARD_TRACK_STOP:   // 0x4A4
        AEAmbienceTrackManager.StopSpecialMissionAmbienceTrack();
        break;
    case AE_SCRIPT_GOGO_SELECT: // 0x428
        PlayMissionBankSound(eventId, posn, physical, 6, 3);
        break;
    case AE_SCRIPT_GOGO_ACCEPT: // 0x429
        PlayMissionBankSound(eventId, posn, physical, 0, 3);
        break;
    case AE_SCRIPT_GOGO_DECLINE: // 0x42A
    case AE_SCRIPT_BEE_ACCEPT:   // 0x432
        PlayMissionBankSound(eventId, posn, physical, 1, 3);
        break;
    case AE_SCRIPT_GOGO_GAME_OVER: // 0x42B
        PlayMissionBankSound(eventId, posn, physical, 4, 3, 1);
        break;
    case AE_SCRIPT_DUAL_TRACK_START: // 0x42C
        PlayAmbienceTrack(150);
        break;
    case AE_SCRIPT_BEE_ZAP: // 0x42E
        PlayMissionBankSound(eventId, posn, physical, 6, 3);
        break;
    case AE_SCRIPT_BEE_PICKUP: // 0x42F
        PlayMissionBankSound(eventId, posn, physical, 4, 3);
        break;
    case AE_SCRIPT_BEE_DROP: // 0x430
        PlayMissionBankSound(eventId, posn, physical, 2, 3);
        break;
    case AE_SCRIPT_BEE_SELECT: // 0x431
        PlayMissionBankSound(eventId, posn, physical, 5, 3);
        break;
    case AE_SCRIPT_BEE_DECLINE: // 0x433
        PlayMissionBankSound(eventId, posn, physical, 1, 3, 0, 0.0f, 2.0f, 0.79f);
        break;
    case AE_SCRIPT_BEE_TRACK_START: // 0x434
        PlayAmbienceTrack(141);
        break;
    case AE_SCRIPT_BEE_GAME_OVER:     // 0x436
    case AE_SCRIPT_TEMPEST_GAME_OVER: // 0x49A
        PlayMissionBankSound(eventId, posn, physical, 3, 3, 1);
        break;
    case AE_SCRIPT_FREEZER_OPEN: // 0x437
        PlayMissionBankSound(eventId, posn, physical, 2, 2, 1, 0.0f, 3.0f);
        break;
    case AE_SCRIPT_FREEZER_CLOSE: // 0x438
        PlayMissionBankSound(eventId, posn, physical, 1, 2, 1, 0.0f, 3.0f);
        break;
    case AE_SCRIPT_MEAT_TRACK_START: // 0x439
        PlayMissionBankSound(eventId, posn, physical, 3, 2, 1, 0.0f, 3.0f);
        PlayMissionBankSound(eventId, posn, physical, 0, 2, 0, 0.0f, 3.0f);
        break;
    case AE_SCRIPT_MEAT_TRACK_STOP: // 0x43A
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_MEAT_TRACK_START, this);
        PlayMissionBankSound(eventId, posn, physical, 4, 2, 1, 0.0f, 3.0f);
        break;
    case AE_SCRIPT_ROULETTE_ADD_CASH: // 0x43B
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_SELECT, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_ROULETTE_REMOVE_CASH: // 0x43C
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_BACK, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_ROULETTE_NO_CASH: // 0x43D
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_ERROR, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_ROULETTE_SPIN: // 0x43E
        PlayMissionBankSound(eventId, posn, physical, 0, 3);
        m_nLastTimeHornPlayed = CTimer::GetTimeInMS();
        field_7C              = 0;
        break;
    case AE_SCRIPT_BANDIT_WHEEL_STOP: // 0x440
        CancelEvent(AE_SCRIPT_BANDIT_WHEEL_START);
        PlayMissionBankSound(eventId, posn, physical, 2, 3);
        break;
    case AE_SCRIPT_BANDIT_WHEEL_START: // 0x441
        PlayMissionBankSound(eventId, posn, physical, 0, 3);
        break;
    case AE_SCRIPT_BANDIT_PAYOUT: // 0x442
        PlayMissionBankSound(eventId, posn, physical, 1, 3);
        break;
    case AE_SCRIPT_BIKE_PACKER_CLUNK: // 0x447
        PlayCollisionSound(65, 1.0f, 16);
        break;
    case AE_SCRIPT_AWARD_TRACK_START: // 0x449
        PlayAmbienceTrack(145);
        break;
    case AE_SCRIPT_MESH_GATE_OPEN_START: // 0x44C
        if (field_7E) {
            PlayCollisionSound(69, 1.0f, 63);
        } else {
            PlayCollisionSound(70, 1.0f, 62);
        }
        field_7E = (field_7E + 1) % 2;
        break;
    case AE_SCRIPT_MESH_GATE_OPEN_STOP: // 0x44D
        if (field_7E) {
            PlayCollisionSound(69, 0.94f, 33);
        } else {
            PlayCollisionSound(70, 1.0f, 25);
        }
        field_7E = (field_7E + 1) % 2;
        break;
    case AE_SCRIPT_OGLOC_DOORBELL:              // 0x44E
    case AE_SCRIPT_DA_NANG_CONTAINER_OPEN:      // 0x456
    case AE_SCRIPT_STINGER_FIRE:                // 0x465
    case AE_SCRIPT_MECHANIC_ATTACH_CAR_BOMB:    // 0x480
    case AE_SCRIPT_CAT2_SECURITY_ALARM:         // 0x483
        PlayMissionBankSound(eventId, posn, physical, 0, 2);
        break;
    case AE_SCRIPT_OGLOC_WINDOW_RATTLE_BANG:    // 0x44F
    case AE_SCRIPT_STINGER_RELOAD:              // 0x450
    case AE_SCRIPT_DA_NANG_HEAVY_DOOR_OPEN:     // 0x457
    case AE_SCRIPT_MECHANIC_SLIDE_OUT:          // 0x47F
    case AE_SCRIPT_CAT2_WOODEN_DOOR_BREACH:     // 0x484
    case AE_SCRIPT_VERTICAL_BIRD_ALARM_START:   // 0x489
        PlayMissionBankSound(eventId, posn, physical, 1, 2);
        break;
    case AE_SCRIPT_HEAVY_DOOR_START: // 0x451
        PlayMissionBankSound(eventId, posn, physical, 2, 2);
        break;
    case AE_SCRIPT_SHOOT_CONTROLS: // 0x453
        if (AEAudioHardware.IsSoundBankLoaded(SND_BANK_SCRIPT_BLACK_PROJECT, SND_BANK_SLOT_MISSION3)) {
            PlayMissionBankSound(eventId, posn, physical, 3, 2);
        } else if (AEAudioHardware.IsSoundBankLoaded(SND_BANK_SCRIPT_UNCLE_SAM, SND_BANK_SLOT_MISSION3)) {
            PlayMissionBankSound(eventId, posn, physical, 4, 2);
        }
        break;
    case AE_SCRIPT_CARGO_PLANE_DOOR_START: // 0x454
    case AE_SCRIPT_HEAVY_GATE_START:       // 0x466
        PlayMissionBankSound(eventId, posn, physical, 1, 2);
        PlayMissionBankSound(eventId, posn, physical, 0, 2);
        break;
    case AE_SCRIPT_CARGO_PLANE_DOOR_STOP: // 0x455
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_CARGO_PLANE_DOOR_START, this);
        PlayMissionBankSound(eventId, posn, physical, 2, 2);
        break;
    case AE_SCRIPT_GYM_BIKE_START: // 0x459
        PlayMissionBankSound(eventId, posn, physical, 0, 3, 1, -18.0f, 2.0f, 1.0f);
        field_8C = 1.0f;
        break;
    case AE_SCRIPT_GYM_BIKE_STOP:            // 0x45A
    case AE_SCRIPT_GYM_RUNNING_MACHINE_STOP: // 0x45F
        field_8C = 2.0f;
        break;
    case AE_SCRIPT_GYM_BOXING_BELL:   // 0x45B
    case AE_SCRIPT_TEMPEST_EXPLOSION: // 0x494
        PlayMissionBankSound(eventId, posn, physical, 2, 3);
        break;
    case AE_SCRIPT_GYM_INCREASE_DIFFICULTY: // 0x45C
    case AE_SCRIPT_OTB_NO_CASH:             // 0x486
        PlayMissionBankSound(eventId, posn, physical, 3, 3);
        break;
    case AE_SCRIPT_GYM_REST_WEIGHTS: // 0x45D
    case AE_SCRIPT_TEMPEST_SELECT:   // 0x49C
        PlayMissionBankSound(eventId, posn, physical, 7, 3);
        break;
    case AE_SCRIPT_GYM_RUNNING_MACHINE_START: // 0x45E
        PlayMissionBankSound(eventId, posn, physical, 1, 3, 1, -18.0f, 2.0f, 1.0f);
        field_8C = 1.0f;
        break;
    case AE_SCRIPT_OTB_BET_ZERO:       // 0x460
    case AE_SCRIPT_RESTAURANT_CJ_EAT:  // 0x490
        PlayMissionBankSound(eventId, posn, physical, 0, 3);
        break;
    case AE_SCRIPT_OTB_INCREASE_BET:    // 0x461
    case AE_SCRIPT_RESTAURANT_CJ_PUKE:  // 0x491
    case AE_SCRIPT_TEMPEST_ENEMY_SHOOT: // 0x493
        PlayMissionBankSound(eventId, posn, physical, 1, 3);
        break;
    case AE_SCRIPT_OTB_LOSE: // 0x462
        PlayMissionBankSound(eventId, posn, physical, 2, 3, 1);
        break;
    case AE_SCRIPT_OTB_PLACE_BET:     // 0x463
    case AE_SCRIPT_TEMPEST_HIGHLIGHT: // 0x49B
        PlayMissionBankSound(eventId, posn, physical, 4, 3);
        break;
    case AE_SCRIPT_OTB_WIN: // 0x464
        PlayMissionBankSound(eventId, posn, physical, 5, 3, 1);
        break;
    case AE_SCRIPT_HEAVY_GATE_STOP: // 0x467
        AESoundManager.CancelSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_HEAVY_GATE_START, this);
        PlayMissionBankSound(eventId, posn, physical, 2, 2);
        break;
    case AE_SCRIPT_VERTICAL_BIRD_LIFT_START: // 0x468
        PlayMissionBankSound(eventId, posn, physical, 2, 2);
        PlayMissionBankSound(eventId, posn, physical, 0, 2);
        break;
    case AE_SCRIPT_VERTICAL_BIRD_LIFT_STOP: // 0x469
        CancelEvent(AE_SCRIPT_VERTICAL_BIRD_LIFT_START);
        PlayMissionBankSound(eventId, posn, physical, 3, 2);
        break;
    case AE_SCRIPT_PUNCH_PED: // 0x46A
        PlayWeaponSound(58);
        PlayWeaponSound(40);
        break;
    case AE_SCRIPT_AMMUNATION_GUN_COLLISION: // 0x46B
        PlayCollisionSound(33);
        PlayCollisionSound(50, 0.79f, 22);
        break;
    case AE_SCRIPT_CAMERA_SHOT: // 0x46C
        if (physical) {
            AudioEngine.ReportWeaponEvent(AE_WEAPON_FIRE, WEAPON_CAMERA, physical);
        } else {
            PlayResidentSoundEvent(SND_BANK_SLOT_WEAPON_GEN, SND_BANK_GENRL_WEAPONS, 45, eventId, posn, nullptr, 0.0f, 1.0f, 0, 1.0f);
        }
        break;
    case AE_SCRIPT_BUY_CAR_MOD: // 0x46D
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_BUY_CAR_MOD, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_BUY_CAR_RESPRAY: // 0x46E
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_CAR_RESPRAY, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_BASEBALL_BAT_HIT_PED: // 0x46F
        PlayWeaponSound(34, 5.0f);
        PlayWeaponSound(40);
        break;
    case AE_SCRIPT_STAMP_PED: // 0x470
        PlayWeaponSound(82, -3.0f);
        break;
    case AE_SCRIPT_CHECKPOINT_AMBER: // 0x471
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PART_MISSION_COMPLETE, 0.0f, 1.12f);
        break;
    case AE_SCRIPT_CHECKPOINT_RED: // 0x473
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PART_MISSION_COMPLETE, 0.0f, 1.26f);
        break;
    case AE_SCRIPT_CAR_SMASH_GATE: // 0x475
        PlayCollisionSound((eSoundID)CAEAudioUtility::GetRandomNumberInRange(20, 28));
        PlayCollisionSound(65);
        break;
    case AE_SCRIPT_OTB_TRACK_START: // 0x476
        PlayAmbienceTrack(160);
        break;
    case AE_SCRIPT_PED_HIT_WATER_SPLASH: // 0x478
        if (physical) {
            AudioEngine.ReportWaterSplash(physical, -6.0f, false);
        } else {
            AudioEngine.ReportWaterSplash(posn, -6.0f);
        }
        break;
    case AE_SCRIPT_RESTAURANT_TRAY_COLLISION: // 0x479
        PlayCollisionSound(19, 1.0f, 65);
        break;
    case AE_SCRIPT_SWEETS_HORN: // 0x47B
        if (!AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(eventId, this)) {
            PlayResidentSoundEvent(SND_BANK_SLOT_HORN_AND_SIREN, SND_BANK_GENRL_HORN, 7, eventId, posn, physical, 0.0f, 1.0f, 0, 1.0f);
            m_nLastTimeHornPlayed = CTimer::GetTimeInMS();
        }
        break;
    case AE_SCRIPT_PICKUP_STANDARD: // 0x47E
        AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_PICKUP_MONEY, 0.0f, 1.0f);
        break;
    case AE_SCRIPT_MINITANK_FIRE: // 0x485
        if (physical) {
            AudioEngine.ReportWeaponEvent(AE_WEAPON_FIRE, WEAPON_M4, physical);
        }
        break;
    case AE_SCRIPT_EXPLOSION: // 0x487
        m_ExplosionAudio.AddAudioEvent(AE_EXPLOSION, physical ? physical->GetPosition() : posn, 0.0f);
        break;
    case AE_SCRIPT_VERTICAL_BIRD_ALARM_STOP: // 0x48A
        CancelEvent(AE_SCRIPT_VERTICAL_BIRD_ALARM_START);
        break;
    case AE_SCRIPT_PED_COLLAPSE: // 0x48B
        PlayResidentSoundEvent(SND_BANK_SLOT_FOOTSTEPS_GENERIC, SND_BANK_FEET_GENERIC, 0, eventId, posn, physical, 0.0f, 1.0f, 0, 1.0f);
        break;
    case AE_SCRIPT_AIR_HORN: { // 0x48C
        CVector left{ -1.0f, 0.0f, 0.0f };
        PlayMissionBankSound(eventId, left, nullptr, 0, 3);
        CVector right{ 1.0f, 0.0f, 0.0f };
        PlayMissionBankSound(eventId, right, nullptr, 1, 3);
        break;
    }
    case AE_SCRIPT_SHUTTER_DOOR_SLOW_START: // 0x48D
        AddDoorEvent(AE_GARAGE_DOOR_OPENING, 0.79f);
        break;
    case AE_SCRIPT_TEMPEST_PLAYER_SHOOT: // 0x492
        PlayMissionBankSound(eventId, posn, physical, 6, 3);
        break;
    case AE_SCRIPT_TEMPEST_PICKUP1: // 0x495
        PlayMissionBankSound(eventId, posn, physical, 5, 3, 0, 0.0f, 2.0f, 0.67f);
        break;
    case AE_SCRIPT_TEMPEST_PICKUP2: // 0x496
        PlayMissionBankSound(eventId, posn, physical, 5, 3, 0, 0.0f, 2.0f, 0.79f);
        break;
    case AE_SCRIPT_TEMPEST_PICKUP3: // 0x497
        PlayMissionBankSound(eventId, posn, physical, 5, 3);
        break;
    case AE_SCRIPT_TEMPEST_WARP: // 0x498
        PlayMissionBankSound(eventId, posn, physical, 8, 3, 1, 0.0f, 2.0f, 0.38f);
        break;
    case AE_SCRIPT_TEMPEST_TRACK_START: // 0x49D
        PlayAmbienceTrack(172);
        break;
    case AE_SCRIPT_DRIVING_AWARD_TRACK_START: // 0x49F
        PlayAmbienceTrack(149);
        break;
    case AE_SCRIPT_BIKE_AWARD_TRACK_START: // 0x4A1
        PlayAmbienceTrack(142);
        break;
    case AE_SCRIPT_PILOT_AWARD_TRACK_START: // 0x4A3
        PlayAmbienceTrack(161);
        break;
    case AE_SCRIPT_PED_DEATH_CRUNCH: // 0x4A5
        if (CLocalisation::Blood() && physical && physical->GetIsTypePed()) {
            physical->AsPed()->GetAE().AddAudioEvent(AE_PED_CRUNCH, 0.0f, 1.0f, physical, SURFACE_DEFAULT, 0, 0);
        }
        break;
    case AE_SCRIPT_SPANK: // 0x4A6
        PlayWeaponSound((eSoundID)CAEAudioUtility::GetRandomNumberInRange(78, 80));
        break;
    default:
        break;
    }
}

// 0x4EE960
void CAEScriptAudioEntity::ReportMissionAudioEvent(eAudioEvents eventId, CPhysical* physical, float volume, float speed) {
    CVector posn{ -1000.0f, -1000.0f, -1000.0f };
    ProcessMissionAudioEvent(eventId, posn, physical, volume, speed);
}

// 0x4EE940
void CAEScriptAudioEntity::ReportMissionAudioEvent(eAudioEvents eventId, CVector& posn) {
    ProcessMissionAudioEvent(eventId, posn, nullptr);
}

// 0x4EC970
void CAEScriptAudioEntity::UpdateParameters(CAESound* sound, int16 curPlayPos) {
    CVector posn{-1000.0f, -1000.0f, -1000.0f};
    if (!sound) {
        return;
    }

    for (auto i = 0; i < MISSION_AUDIO_COUNT; i++) {
        auto& link = wavLinks[i];
        if (sound == link.m_Sound) {
            if (curPlayPos == -1) {
                link.m_Sound = nullptr;
                return;
            }
            if (link.m_pEntity) {
                sound->SetPosition(link.m_pEntity->GetPosition());
            }
        } else {
            const auto now = CTimer::GetTimeInMS();
            switch ((eAudioEvents)sound->m_Event) {
            case AE_CRANE_WINCH_MOVE:
                if (now > m_nLastTimeHornPlayed + 300) {
                    sound->StopSoundAndForget();
                    m_nLastTimeHornPlayed = 0;
                    PlayResidentSoundEvent(
                        SND_BANK_SLOT_PLAYER_ENGINE_P,
                        SND_BANK_GENRL_CRANE_P,
                        2,
                        AE_SCRIPT_CRANE_MOVE_STOP,
                        sound->m_CurrPos,
                        sound->m_PhysicalEntity->AsPhysical(),
                        -12.0f,
                        1.0f,
                        0,
                        2.5f
                    );
                } else {
                    sound->m_Volume = m_Volume;
                    sound->m_Speed  = m_Speed;
                }
                break;
            case AE_SCRIPT_DUAL_THRUST:
            case AE_SCRIPT_BEE_BUZZ:
            case AE_SCRIPT_TEMPEST_SHIELD_GLOW:
                if (now > m_nLastTimeHornPlayed + 300) {
                    sound->StopSoundAndForget();
                    m_nLastTimeHornPlayed = 0;
                }
                break;
            case AE_SCRIPT_ROULETTE_SPIN:
                if (now > m_nLastTimeHornPlayed + 4500) {
                    if (sound->m_Volume > -40.0f) {
                        sound->m_Volume -= 0.1f;
                    } else {
                        sound->StopSoundAndForget();
                        m_nLastTimeHornPlayed = 0;
                    }

                    if (sound->m_Speed > 0.0f) {
                        sound->m_Speed = std::max(sound->m_Speed - 0.001f, 0.0f);
                    }
                }

                if (now > m_nLastTimeHornPlayed + 4800 && field_7C == 0) {
                    PlayMissionBankSound(AE_SCRIPT_ROULETTE_BALL_BOUNCING, posn, nullptr, (int16)CAEAudioUtility::GetRandomNumberInRange(1, 3), 3);
                    field_7C = 1;
                }
                break;
            case AE_SCRIPT_GYM_BIKE_START:
            case AE_SCRIPT_GYM_RUNNING_MACHINE_START: {
                auto target = GetDefaultVolume((eAudioEvents)sound->m_Event);
                if (field_8C == 1.0f) {
                    if (sound->m_Volume < target) {
                        sound->m_Volume = std::min(sound->m_Volume + 0.1f, target);
                    }
                } else if (field_8C == 2.0f) {
                    target -= 18.0f;
                    if (sound->m_Volume <= target) {
                        sound->StopSoundAndForget();
                    } else {
                        sound->m_Volume = std::max(sound->m_Volume - 0.1f, target);
                    }
                }
                break;
            }
            case AE_SCRIPT_SWEETS_HORN:
                if (now > m_nLastTimeHornPlayed + 500) {
                    sound->StopSoundAndForget();
                    m_nLastTimeHornPlayed = 0;
                }
                break;
            default:
                break;
            }
        }
    }
}

// 0x4EC900
void CAEScriptAudioEntity::Service() {
    CVector posn = {-1000.0f, -1000.0f, -1000.0f};
    if (!m_Physical)
        return;
    if (AESoundManager.AreSoundsOfThisEventPlayingForThisEntity(AE_SCRIPT_CRANE_ENTER, this) != 0)
        return;

    PlayResidentSoundEvent(SND_BANK_SLOT_PLAYER_ENGINE_P, SND_BANK_GENRL_CRANE_P, 0, AE_SCRIPT_CRANE_ENTER, posn, m_Physical, 0.0f, 1.0f, 0, 2.5f);
}

void CAEScriptAudioEntity::InjectHooks() {
    RH_ScopedVirtualClass(CAEScriptAudioEntity, 0x862E58, 1);
    RH_ScopedCategory("Audio/Entities");

    RH_ScopedInstall(Constructor, 0x5074D0);
    RH_ScopedInstall(Initialise, 0x5B9B60);
    RH_ScopedInstall(Service, 0x4EC900);
    RH_ScopedInstall(Reset, 0x4EC150);
    RH_ScopedInstall(GetMissionAudioLoadingStatus, 0x4EBF60);
    RH_ScopedInstall(IsMissionAudioSampleFinished, 0x4EBFE0);
    RH_ScopedInstall(GetMissionAudioEvent, 0x4EC020);
    RH_ScopedInstall(ClearMissionAudio, 0x4EC040);
    RH_ScopedInstall(SetMissionAudioPosition, 0x4EC0C0);
    RH_ScopedInstall(AttachMissionAudioToPhysical, 0x4EC100);
    RH_ScopedInstall(PreloadMissionAudio, 0x4EC190);
    RH_ScopedInstall(PlayLoadedMissionAudio, 0x4EC270);
    RH_ScopedInstall(GetMissionAudioPosition, 0x4EC4D0);
    RH_ScopedInstall(PlayResidentSoundEvent, 0x4EC550);
    RH_ScopedInstall(PlayMissionBankSound, 0x4EC6D0);
    RH_ScopedInstall(ProcessMissionAudioEvent, 0x4ECCF0);
    RH_ScopedOverloadedInstall(ReportMissionAudioEvent, "1", 0x4EE960, void (CAEScriptAudioEntity::*)(eAudioEvents, CPhysical*, float, float));
    RH_ScopedOverloadedInstall(ReportMissionAudioEvent, "2", 0x4EE940, void (CAEScriptAudioEntity::*)(eAudioEvents, CVector&));
    RH_ScopedVMTInstall(UpdateParameters, 0x4EC970);
}

CAEScriptAudioEntity* CAEScriptAudioEntity::Constructor() {
    this->CAEScriptAudioEntity::CAEScriptAudioEntity();
    return this;
}
