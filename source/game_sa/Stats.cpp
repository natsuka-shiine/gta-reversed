/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Stats.h"
#include "MenuSystem.h"
#include "Hud.h"
#include "TagManager.h"
#include "StuntJumpManager.h"

// 0x69F7E0 - Converts GXT text into ASCII, result is stored in a static buffer (0xC1BDD0)
static const char* GxtCharToAscii(const GxtChar* text, uint8 start) {
    static auto& s_Buffer = StaticRef<char[256]>(0xC1BDD0);

    if (start > 0) {
        text += start;
    }

    int32 i = 0;
    if (text) {
        for (; i < 255 && *text; text++, i++) {
            const uint8 c = *text;
            uint8       out;
            if (c < 0x80) {
                out = c;
            } else if (c <= 0x83) {
                out = c + 0x40;
            } else if (c <= 0x8D) {
                out = c + 0x42;
            } else if (c <= 0x91) {
                out = c + 0x44;
            } else if (c <= 0x95) {
                out = c + 0x47;
            } else if (c <= 0x9A) {
                out = c + 0x49;
            } else if (c <= 0xA4) {
                out = c + 0x4B;
            } else if (c <= 0xA8) {
                out = c + 0x4D;
            } else if (c <= 0xCC) {
                out = c + 0x50;
            } else if (c == 0xCD) {
                out = 0xD1;
            } else if (c == 0xCE) {
                out = 0xF1;
            } else if (c == 0xCF) {
                out = 0xBF;
            } else {
                out = '#';
            }
            s_Buffer[i] = (char)out;
        }
    }
    s_Buffer[i] = '\0';
    return s_Buffer;
}

// 0x719240 - CFont::FilterOutTokensFromString
static void FilterOutTokensFromString(GxtChar* text) {
    // NOTE: No bounds checking in the original either
    GxtChar temp[256];
    {
        auto* dst = temp;
        for (const auto* src = text; *src; src++) {
            *dst++ = *src;
        }
        *dst = 0;
    }

    auto* out = text;
    for (const auto* src = temp; *src; src++) {
        if (*src == '~') { // Skip the whole `~token~`
            do {
                src++;
            } while (*src != '~');
        } else {
            *out++ = *src;
        }
    }
    *out = 0;
}

void CStats::InjectHooks() {
    RH_ScopedClass(CStats);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Init, 0x55C0C0);
    RH_ScopedOverloadedInstall(GetStatValue, "-OG", 0x558E40, float(*)(eStats));
    RH_ScopedInstall(SetStatValue, 0x55A070);
    RH_ScopedInstall(IsStatFloat, 0x558E30);
    /*RH_ScopedInstall(GetFullFavoriteRadioStationList, 0x558F90); - different return type*/
    RH_ScopedInstall(FindCriminalRatingNumber, 0x559080);
    RH_ScopedInstall(GetPercentageProgress, 0x5591E0);
    RH_ScopedInstall(PopulateFavoriteRadioStationList, 0x558EC0);
    RH_ScopedInstall(FindMostFavoriteRadioStation, 0x558FA0);
    RH_ScopedInstall(FindLeastFavoriteRadioStation, 0x559010);
    RH_ScopedInstall(BuildStatLine, 0x559230);
    RH_ScopedInstall(CheckForStatsMessage, 0x559760);
    RH_ScopedInstall(LoadStatUpdateConditions, 0x559860);
    RH_ScopedInstall(GetFatAndMuscleModifier, 0x559AF0);
    RH_ScopedInstall(FindCriminalRatingString, 0x55A210);
    RH_ScopedInstall(ConstructStatLine, 0x55A780);
    RH_ScopedInstall(UpdateRespectStat, 0x55BC50);
    RH_ScopedInstall(UpdateSexAppealStat, 0x55BF20);
    RH_ScopedInstall(UpdateFatAndMuscleStats, 0x55C470);
    RH_ScopedInstall(UpdateStatsWhenCycling, 0x55C780);
    RH_ScopedInstall(UpdateStatsWhenSwimming, 0x55C990);
    RH_ScopedInstall(UpdateStatsWhenDriving, 0x55CAC0);
    RH_ScopedInstall(UpdateStatsWhenFlying, 0x55CC00);
    RH_ScopedInstall(UpdateStatsWhenWeaponHit, 0x55CEB0);
    RH_ScopedInstall(UpdateStatsOnRespawn, 0x55CFC0);
    RH_ScopedInstall(UpdateStatsAddToHealth, 0x55D030);
    RH_ScopedInstall(ConvertToMins, 0x559540);
    RH_ScopedInstall(ConvertToSecs, 0x559560);
    RH_ScopedInstall(SafeToShowThisStat, 0x559590);
    RH_ScopedInstall(CheckForThreshold, 0x5595F0);
    RH_ScopedInstall(IsStatCapped, 0x559630);
    RH_ScopedInstall(LoadActionReactionStats, 0x5599B0);
    RH_ScopedInstall(FindMaxNumberOfGroupMembers, 0x559A50);
    RH_ScopedInstall(ProcessReactionStatsOnDecrement, 0x559730);
    RH_ScopedInstall(DecrementStat, 0x559FA0);
    RH_ScopedInstall(SetNewRecordStat, 0x55C410);
    RH_ScopedInstall(RegisterFastestTime, 0x55A0B0);
    RH_ScopedInstall(RegisterBestPosition, 0x55A160);
    RH_ScopedInstall(ProcessReactionStatsOnIncrement, 0x55B900);
    RH_ScopedInstall(DisplayScriptStatUpdateMessage, 0x55B980);
    RH_ScopedInstall(IncrementStat, 0x55C180);
    RH_ScopedInstall(UpdateStatsWhenSprinting, 0x55C660);
    RH_ScopedInstall(UpdateStatsWhenRunning, 0x55C6F0);
    RH_ScopedInstall(UpdateStatsWhenOnMotorBike, 0x55CD60);
    RH_ScopedInstall(UpdateStatsWhenFighting, 0x55CFA0);
    RH_ScopedInstall(ModifyStat, 0x55D090);

    // unused
    RH_ScopedInstall(GetStatID, 0x558DE0);
    RH_ScopedInstall(GetTimesMissionAttempted, 0x558E70);
    RH_ScopedInstall(RegisterMissionAttempted, 0x558E80);
    RH_ScopedInstall(RegisterMissionPassed, 0x558EA0);

    RH_ScopedInstall(Load, 0x5D3BF0);
    RH_ScopedInstall(Save, 0x5D3B40);

    RH_ScopedGlobalInstall(GxtCharToAscii, 0x69F7E0);
    RH_ScopedGlobalInstall(FilterOutTokensFromString, 0x719240);
}

// 0x55C0C0
void CStats::Init() {
    std::ranges::fill(StatTypesFloat, 0.0f);
    std::ranges::fill(StatTypesInt, 0);
    std::ranges::fill(PedsKilledOfThisType, 0);
    std::ranges::fill(TimesMissionAttempted, 0);

    bStatUpdateMessageDisplayed = false;
    CTimer::SetTimeInMS(0);
    std::ranges::fill(LastMissionPassedName, 0);
    m_SprintStaminaCounter = 0;
    m_CycleStaminaCounter = 0;
    m_SwimStaminaCounter = 0;
    m_DrivingCounter = 0;
    m_FlyingCounter = 0;
    m_BoatCounter = 0;
    m_BikeCounter = 0;
    m_FatCounter = 0;
    m_RunningCounter = 0;

    StatTypesFloat[STAT_MAX_HEALTH] = 569.0f;
    CheckForStatsMessage();
    StatTypesFloat[STAT_STAMINA] = 100.0f;
    CheckForStatsMessage();

    PopulateFavoriteRadioStationList();
    LoadActionReactionStats();
    LoadStatUpdateConditions();
}

// 0x558E40
float CStats::GetStatValue(eStats stat) {
    if (!IsStatFloat(stat)) { // int32
        assert(stat >= FIRST_INT_STAT);

        return static_cast<float>(StatTypesInt[stat - FIRST_INT_STAT]);
    }

    return StatTypesFloat[stat];
}

// 0x55A070
void CStats::SetStatValue(eStats stat, float value) {
    if (IsStatFloat(stat)) {
        StatTypesFloat[stat] = value;
    } else { // int32
        assert(stat >= FIRST_INT_STAT);

        StatTypesInt[stat - FIRST_INT_STAT] = static_cast<int32>(value);
    }
    CheckForStatsMessage();
}

// 0x558E30
bool CStats::IsStatFloat(eStats stat) {
    return stat < FIRST_UNUSED_STAT;
}

// 0x558EC0
bool CStats::PopulateFavoriteRadioStationList() {
    const auto listenTimes = AudioEngine.GetRadioStationListenTimes();
    bool allZero = true;
    for (auto&& [i, v] : rngv::enumerate(FavoriteRadioStationList)) {
        v = listenTimes[i];
        if (v != 0) {
            allZero = false;
        }
    }
    return allZero;
}

// 0x558FA0
eRadioID CStats::FindMostFavoriteRadioStation() {
    int32 mostTime = 0;
    int32 station  = 1;
    for (int32 i = 1; i < 13; i++) {
        if (FavoriteRadioStationList[i] > mostTime) {
            mostTime = FavoriteRadioStationList[i];
            station  = i;
        }
    }
    return (eRadioID)station;
}

// 0x559010
int32 CStats::FindLeastFavoriteRadioStation() {
    int32 leastTime = FavoriteRadioStationList[1];
    int32 station   = 1;
    for (int32 i = 1; i < 13; i++) {
        if (FavoriteRadioStationList[i] < leastTime) {
            leastTime = FavoriteRadioStationList[i];
            station   = i;
        }
    }
    return station;
}

// 0x559080
int32 CStats::FindCriminalRatingNumber() {
    CPlayerInfo* playerInfo = FindPlayerPed()->GetPlayerInfoForThisPlayerPed();

    auto value = (int32)(
        GetStatValue(STAT_TOTAL_LEGITIMATE_KILLS)
        - (GetStatValue(STAT_TIMES_BUSTED) - GetStatValue(STAT_NUMBER_OF_HOSPITAL_VISITS)) * 3.0f
        + (GetStatValue(STAT_HIGHEST_FIREFIGHTER_MISSION_LEVEL) + GetStatValue(STAT_HIGHEST_PARAMEDIC_MISSION_LEVEL)) * 10.0f
        + int32((float)playerInfo->m_nMoney / 5000.0f)
        + GetStatValue(STAT_PLANES_HELICOPTERS_DESTROYED) * 30.0f
        + GetStatValue(STAT_TOTAL_FIRES_EXTINGUISHED)
        + GetStatValue(STAT_CRIMINALS_KILLED_ON_VIGILANTE_MISSION)
        + GetStatValue(STAT_PEOPLE_SAVED_IN_AN_AMBULANCE)
    );

    if (CCheat::m_bHasPlayerCheated || GetStatValue(STAT_TIMES_CHEATED) > 0.0f) {
        value -= 10 * (int32)GetStatValue(STAT_TIMES_CHEATED);

        value = std::max(value, -10000);
    } else {
        value = std::max(value, 0);
    }

    float bulletsFired = GetStatValue(STAT_BULLETS_FIRED);

    if (bulletsFired >= 100.0f) {
        value += (int32)(500 * (GetStatValue(STAT_BULLETS_THAT_HIT) / bulletsFired));
    }

    return value + (int32)(10 * GetPercentageProgress());
}

// 0x5591E0
float CStats::GetPercentageProgress() {
    return std::min(StatTypesFloat[STAT_PROGRESS_MADE] / 187.0f * 100.0f, 100.0f);
}

// 0x559230
void CStats::BuildStatLine(char* line, void* pValue1, int32 metrics, void* pValue2, int32 type) {
    if (!line) {
        return;
    }

    gString2[0] = '\0';

    const auto i1 = [&] { return *static_cast<int32*>(pValue1); };
    const auto f1 = [&] { return *static_cast<float*>(pValue1); };
    const auto i2 = [&] { return *static_cast<int32*>(pValue2); };
    const auto f2 = [&] { return *static_cast<float*>(pValue2); };

    if (type == 1) { // Time (minutes:seconds)
        sprintf_s(gString2, i2() >= 10 ? "%d:%d" : "%d:0%d", i1(), i2());
    } else if (pValue2) { // `value1` out of `value2`
        switch (metrics) {
        case 0: {
            const auto v1 = i1(), v2 = i2();
            sprintf_s(gString2, " %d %s %d", v1, GxtCharToAscii(TheText.Get("FEST_OO"), 0), v2);
            break;
        }
        case 1: {
            const auto v1 = f1(), v2 = f2();
            sprintf_s(gString2, "%.2f %s %.2f", v1, GxtCharToAscii(TheText.Get("FEST_OO"), 0), v2);
            break;
        }
        case 3: {
            const auto v1 = f1(), v2 = f2();
            sprintf_s(gString2, "$%.2f %s $%.2f", v1, GxtCharToAscii(TheText.Get("FEST_OO"), 0), v2);
            break;
        }
        }
    } else if (pValue1) {
        switch (metrics) {
        case 0:
            sprintf_s(gString2, "%d", i1());
            break;
        case 1:
            sprintf_s(gString2, "%.2f", f1());
            break;
        case 2:
            sprintf_s(gString2, "%0.2f%%", f1());
            break;
        case 3:
            sprintf_s(gString2, "$%.2f", f1());
            break;
        case 4:
            sprintf_s(gString2, "%d|", i1());
            break;
        case 5: {
            const auto v = i1();
            if (CLocalisation::Metric()) {
                // BUG (OG): A double is passed to `%d` (kept as is)
                sprintf_s(gString2, "%dkgs", (int32)((float)v * 0.4536f)); // NOTE: Original passes a double to `%d` here
            } else {
                sprintf_s(gString2, "%dlbs", v);
            }
            break;
        }
        case 6: {
            const auto v = f1();
            sprintf_s(gString2, "%.2f %s", v, GxtCharToAscii(TheText.Get("ST_MILE"), 0));
            break;
        }
        case 7:
            sprintf_s(gString2, "%.2fm", f1());
            break;
        case 8:
            sprintf_s(gString2, "%.2fft", f1() * 3.3333333f);
            break;
        case 9: {
            const auto v = i1();
            sprintf_s(gString2, "%d %s", v, GxtCharToAscii(TheText.Get("ST_SECS"), 0));
            break;
        }
        }
    }

    GxtCharStrcpy(gGxtString, TheText.Get(line));
    FilterOutTokensFromString(gGxtString);
    AsciiToGxtChar(gString2, gGxtString2);
}

// 0x559540
int32 CStats::ConvertToMins(int32 statValue) {
    if (statValue > 59)
        return (statValue - 60) / 60 + 1;
    return 0;
}

// 0x559560
int32 CStats::ConvertToSecs(int32 statValue) {
    int32 seconds = statValue;
    if (statValue > 59)
        seconds = -60 - 60 * ((statValue - 60) / 60) + statValue;
    if (seconds < 0)
        seconds = -seconds;
    return seconds;
}

// 0x559590
bool CStats::SafeToShowThisStat(eStats stat) {
    if (!CLocalisation::GermanGame()) {
        return true;
    }

    switch (stat) {
    case STAT_RAMPAGES_ATTEMPTED:
    case STAT_RAMPAGES_PASSED:
    case STAT_TOTAL_LEGITIMATE_KILLS:
    case STAT_HIGHEST_CIVILIAN_PEDS_KILLED_ON_RAMPAGE:
    case STAT_HIGHEST_POLICE_PEDS_KILLED_ON_RAMPAGE:
    case STAT_HIGHEST_CIVILIAN_VEHICLES_DESTROYED_ON_RAMPAGE:
    case STAT_HIGHEST_POLICE_VEHICLES_DESTROYED_ON_RAMPAGE:
    case STAT_HIGHEST_NUMBER_OF_TANKS_DESTROYED_ON_RAMPAGE:
        return false;
    default:
        return true;
    }
}

// 0x5595F0
bool CStats::CheckForThreshold(float* pValue, float range) {
    if (*pValue + 40.0f >= range && *pValue - 40.0f <= range) {
        return false;
    }
    *pValue = range;
    return true;
}

// 0x559630
bool CStats::IsStatCapped(eStats stat) {
    switch (stat) {
    case STAT_GIRLFRIEND_RESPECT:
    case STAT_CLOTHES_RESPECT:
    case STAT_FITNESS_RESPECT:
    case STAT_FAT:
    case STAT_STAMINA:
    case STAT_MUSCLE:
    case STAT_MAX_HEALTH:
    case STAT_SEX_APPEAL:
    case STAT_PISTOL_SKILL:
    case STAT_SILENCED_PISTOL_SKILL:
    case STAT_DESERT_EAGLE_SKILL:
    case STAT_SHOTGUN_SKILL:
    case STAT_SAWN_OFF_SHOTGUN_SKILL:
    case STAT_COMBAT_SHOTGUN_SKILL:
    case STAT_MACHINE_PISTOL_SKILL:
    case STAT_SMG_SKILL:
    case STAT_AK_47_SKILL:
    case STAT_M4_SKILL:
    case STAT_RIFLE_SKILL:
    case STAT_APPEARANCE:
    case STAT_ARMOR:
    case STAT_ENERGY:
    case STAT_DRIVING_SKILL:
    case STAT_FLYING_SKILL:
    case STAT_LUNG_CAPACITY:
    case STAT_BIKE_SKILL:
    case STAT_LUCK:
    case STAT_HORSESHOES_COLLECTED:
    case STAT_TOTAL_HORSESHOES:
    case STAT_OYSTERS_COLLECTED:
    case STAT_TOTAL_OYSTERS:
    case STAT_CYCLING_SKILL:
        return true;
    }
    return false;
}

// 0x559760
void CStats::CheckForStatsMessage() {
    if (CPad::GetPad(0)->JustOutOfFrontEnd
        || !bShowUpdateStats
        || TheCamera.m_bWideScreenOn
        || CCutsceneMgr::ms_cutsceneProcessing
        || CHud::HelpMessageDisplayed()
        || CMenuSystem::num_menus_in_use)
    {
        return;
    }

    for (uint32 i = 0; i < TotalNumStatMessages && i < StatMessage.size(); i++) {
        auto& msg = StatMessage[i];
        if (msg.displayed) {
            continue;
        }
        const auto value = GetStatValue((eStats)msg.stat_num);
        if (msg.condition == STATMESSAGE_LESSTHAN ? value <= msg.value : value >= msg.value) {
            msg.displayed = true;
            CHud::SetHelpMessage(TheText.Get(msg.text_id), false, false, false);
            bStatUpdateMessageDisplayed = true;
        }
    }
}

// 0x559860
void CStats::LoadStatUpdateConditions() {
    CFileMgr::SetDir("");
    auto* file = CFileMgr::OpenFile("DATA\\STATDISP.DAT", "rb");

    TotalNumStatMessages = 0;

    uint32 numMessages = 0;
    int32  statNum{};
    float  value{};
    for (char* line = CFileLoader::LoadLine(file); line != nullptr; line = CFileLoader::LoadLine(file)) {
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        char statName[64]{}, condition[64]{}, textId[64]{};
        (void)sscanf_s(line, "%d %s %s %f %s", &statNum, SCANF_S_STR(statName), SCANF_S_STR(condition), &value, SCANF_S_STR(textId));

        assert(numMessages < StatMessage.size());
        auto& msg = StatMessage[numMessages++];
        msg.stat_num  = (int16)statNum;
        msg.displayed = false;
        if (strcmp(condition, "lessthan") == 0) {
            msg.condition = STATMESSAGE_LESSTHAN;
        } else if (strcmp(condition, "morethan") == 0) {
            msg.condition = STATMESSAGE_MORETHAN;
        }
        msg.value = value;
        strcpy_s(msg.text_id, textId);
    }

    TotalNumStatMessages = numMessages;
    CFileMgr::CloseFile(file);
}

// 0x5599B0
void CStats::LoadActionReactionStats() {
    CFileMgr::SetDir("");

    auto* file = CFileMgr::OpenFile("DATA\\AR_STATS.DAT", "rb");

    for (char* line = CFileLoader::LoadLine(file); line != nullptr; line = CFileLoader::LoadLine(file)) {
        int32 reactId;
        float reactValue;

        if (line[0] != '#' && line[0] != NULL) {
            VERIFY(sscanf_s(line, "%d %*s %f", &reactId, &reactValue) == 2);

            StatReactionValue[reactId] = reactValue;
        }
    }

    CFileMgr::CloseFile(file);
}

// 0x559A50
int32 CStats::FindMaxNumberOfGroupMembers() {
    float respect = StatTypesFloat[STAT_TOTAL_RESPECT];

    if (respect < 10.0f)
        return 0;
    if (respect < 60.0f)
        return 2;
    if (respect < 160.0f)
        return 3;
    if (respect < 330.0f)
        return 4;
    if (respect < 540.0f)
        return 5;
    if (respect < 800.0f)
        return 6;

    return 7;
}

// 0x559AF0
float CStats::GetFatAndMuscleModifier(eStatModAbilities statMod) {
    // Tuning values, table at 0x8CDE58 - 0x8CDEC0 (Indexed by address here, so it's easy to cross-check with the original code)
    static auto& s_Tuning = StaticRef<std::array<float, 27>>(0x8CDE58);
    const auto T = [](uint32 addr) { return s_Tuning[(addr - 0x8CDE58) / sizeof(float)]; };

    const auto fat          = StatTypesFloat[STAT_FAT];
    const auto stamina      = StatTypesFloat[STAT_STAMINA];
    const auto muscle       = StatTypesFloat[STAT_MUSCLE];
    const auto maxHealth    = StatTypesFloat[STAT_MAX_HEALTH];
    const auto intStat      = [](eStats stat) { return (float)StatTypesInt[stat - FIRST_INT_STAT]; };

    const auto fatThreshold    = T(0x8CDEC0);
    const auto muscleThreshold = T(0x8CDEB8);

    // `(stat - threshold) * mult / (1000 - threshold)`
    const auto FatTerm    = [&](uint32 multAddr) { return (fat - fatThreshold) * T(multAddr) / (1000.0f - fatThreshold); };
    const auto MuscleTerm = [&](uint32 multAddr) { return (muscle - muscleThreshold) * T(multAddr) / (1000.0f - muscleThreshold); };
    // Same as `FatTerm`, but the fraction is clamped to be positive first
    const auto FatTermPositive = [&](uint32 multAddr) { return std::max(0.0f, (fat - fatThreshold) / (1000.0f - fatThreshold)) * T(multAddr); };

    switch (statMod) {
    case STAT_MOD_0: { // Android: STAT_MODIFIER_CLIMB_HEIGHT
        if (fat > 800.0f) {
            return 2.0f;
        }
        return fat > 400.0f ? 1.0f : 0.0f;
    }
    case STAT_MOD_1: // Android: STAT_MODIFIER_CLIMB_SPEED
        return std::max(FatTermPositive(0x8CDEBC) + 1.0f + MuscleTerm(0x8CDEB4), 0.7f);
    case STAT_MOD_2: // Android: STAT_MODIFIER_JUMP_SPEED
        return std::max(FatTermPositive(0x8CDEB0) + 1.0f + MuscleTerm(0x8CDEAC), 0.8f);
    case STAT_MOD_3: // Android: STAT_MODIFIER_FIGHT_SPEED
        return std::max(FatTerm(0x8CDEA8) + 1.0f + MuscleTerm(0x8CDEA4), 0.8f);
    case STAT_MOD_4: // Android: STAT_MODIFIER_FIGHT_DAMAGE
        return std::min(FatTerm(0x8CDEA0) + 1.0f + MuscleTerm(0x8CDE9C), 2.0f);
    case STAT_MOD_5: {
        const auto v = FatTerm(0x8CDE98) + 1.0f + MuscleTerm(0x8CDE94)
            + stamina * 0.001f * T(0x8CDE90)
            + intStat(STAT_CYCLING_SKILL) * 0.001f * T(0x8CDE8C);
        if (v > 2.0f) {
            return 2.0f;
        }
        return v < 0.25f ? 0.25f : v;
    }
    case STAT_MOD_6: {
        const auto v = FatTerm(0x8CDE88) + 1.0f + intStat(STAT_CYCLING_SKILL) * 0.001f * T(0x8CDE84);
        if (v > 2.0f) {
            return 2.0f;
        }
        return v < 0.5f ? 0.5f : v;
    }
    case STAT_MOD_TIME_CAN_RUN:
        return stamina * 0.001f * T(0x8CDE80) + 150.0f;
    case STAT_MOD_AIR_IN_LUNG:
        return (intStat(STAT_LUNG_CAPACITY) + stamina) * 0.0005f * T(0x8CDE7C) + 1000.0f;
    case STAT_MOD_MAX_HEALTH:
        return maxHealth * 0.001f * T(0x8CDE78);
    case STAT_MOD_10: // Android: STAT_MODIFIER_MAX_HEALTH_LIMIT
        return T(0x8CDE78);
    case STAT_MOD_11:
        return std::min(1.0f, intStat(STAT_BIKE_SKILL) * 0.001f) * T(0x8CDE74) + 1.0f;
    case STAT_MOD_12:
        return std::min(1.0f, intStat(STAT_BIKE_SKILL) * 0.001f) * T(0x8CDE70) + 1.0f;
    case STAT_MOD_13:
        return std::min(1.0f, intStat(STAT_BIKE_SKILL) * 0.001f) * T(0x8CDE6C) + 1.0f;
    case STAT_MOD_DRIVING_SKILL:
        return std::min(FatTerm(0x8CDE68) + intStat(STAT_DRIVING_SKILL) * 0.001f * T(0x8CDE64) + 1.0f, 1.0f);
    case STAT_MOD_15: // Android: STAT_MODIFIER_TAKE_DAMAGE_MULT
        return std::max(FatTerm(0x8CDE60) + 1.0f + MuscleTerm(0x8CDE5C), T(0x8CDE58));
    default:
        return 1.0f;
    }
}

// 0x559730
void CStats::ProcessReactionStatsOnDecrement(eStats stat) {
    if (stat == STAT_ENERGY && GetStatValue(STAT_ENERGY) < 0.0f)
        DecrementStat(STAT_FAT, 23.0f);
}

// 0x559FA0
void CStats::DecrementStat(eStats stat, float value) {
    if (value <= 0.0f)
        return;

    float oldValue = GetStatValue(stat);

    SetStatValue(stat, std::max(oldValue - value, 0.0f));

    ProcessReactionStatsOnDecrement(stat);
    CheckForStatsMessage();
}

// 0x55C410
void CStats::SetNewRecordStat(eStats stat, float value) {
    float currentValue = GetStatValue(stat);

    if (currentValue < value)
        SetStatValue(stat, value);

    CheckForStatsMessage();
}

// 0x55A0B0
void CStats::RegisterFastestTime(eStats stat, int32 fastestTime) {
    SetNewRecordStat(stat, (float)fastestTime);
}

// 0x55A160
void CStats::RegisterBestPosition(eStats stat, int32 position) {
    SetNewRecordStat(stat, (float)position);
}

// 0x55A210
GxtChar* CStats::FindCriminalRatingString() {
    const auto rating = FindCriminalRatingNumber();
    const auto Get    = [](const char* key) { return const_cast<GxtChar*>(TheText.Get(key)); };

    if (rating < 0) {
        if (rating > -500) {
            return Get("RATNG53");
        }
        if (rating > -2000) {
            return Get("RATNG54");
        }
        if (rating > -4000) {
            return Get("RATNG55");
        }
        if (rating > -6000) {
            return Get("RATNG56");
        }
        return Get("RATNG57");
    }

    // Ratings 1 - 24
    constexpr struct { int32 limit; const char* key; } LOW_RATINGS[]{
        { 20,   "RATNG1"  }, { 50,   "RATNG2"  }, { 75,   "RATNG3"  }, { 100,  "RATNG4"  },
        { 120,  "RATNG5"  }, { 150,  "RATNG6"  }, { 200,  "RATNG7"  }, { 240,  "RATNG8"  },
        { 270,  "RATNG9"  }, { 300,  "RATNG10" }, { 335,  "RATNG11" }, { 370,  "RATNG12" },
        { 400,  "RATNG13" }, { 450,  "RATNG14" }, { 500,  "RATNG15" }, { 550,  "RATNG16" },
        { 600,  "RATNG17" }, { 610,  "RATNG18" }, { 650,  "RATNG19" }, { 700,  "RATNG20" },
        { 850,  "RATNG21" }, { 1000, "RATNG22" }, { 1005, "RATNG23" }, { 1150, "RATNG24" },
    };
    for (const auto& [limit, key] : LOW_RATINGS) {
        if (rating < limit) {
            return Get(key);
        }
    }

    if (rating < 1300) {
        return Get((float)StatTypesInt[STAT_TIMES_BUSTED - FIRST_INT_STAT] > 0.0f ? "RATNG25" : "RATNG24");
    }

    // Ratings 26 - 49
    constexpr struct { int32 limit; const char* key; } HIGH_RATINGS[]{
        { 1500,   "RATNG26" }, { 1700,   "RATNG27" }, { 2000,   "RATNG28" }, { 2100,   "RATNG29" },
        { 2300,   "RATNG30" }, { 2500,   "RATNG31" }, { 2750,   "RATNG32" }, { 3000,   "RATNG33" },
        { 3500,   "RATNG34" }, { 4000,   "RATNG35" }, { 5000,   "RATNG36" }, { 7500,   "RATNG37" },
        { 10000,  "RATNG38" }, { 20000,  "RATNG39" }, { 30000,  "RATNG40" }, { 40000,  "RATNG41" },
        { 50000,  "RATNG42" }, { 65000,  "RATNG43" }, { 80000,  "RATNG44" }, { 100000, "RATNG45" },
        { 150000, "RATNG46" }, { 200000, "RATNG47" }, { 300000, "RATNG48" }, { 375000, "RATNG49" },
    };
    for (const auto& [limit, key] : HIGH_RATINGS) {
        if (rating < limit) {
            return Get(key);
        }
    }

    if (rating < 500'000) {
        // Flight time in hours
        const auto flightHours = (int32)((float)StatTypesInt[STAT_FLIGHT_TIME - FIRST_INT_STAT] * (1.0f / 60'000.0f) * (1.0f / 60.0f));
        return Get(flightHours <= 10 ? "RATNG49" : "RATNG50");
    }

    if (rating >= 1'000'000 && CWorld::Players[CWorld::PlayerInFocus].m_nDisplayMoney > 10'000'000) {
        return Get("RATNG52");
    }
    return Get("RATNG51");
}

// 0x55A780
// Builds the stat line with index `lineId` of the stats page `category` into `gGxtString` (name) and `gGxtString2` (value).
// Returns 0 if the line was built, otherwise the total number of lines of the page (pass a huge `lineId` to count them).
int32 CStats::ConstructStatLine(int32 lineId, uint8 category) {
    struct tStatListEntry {
        int16 stat;       // -99 terminates the list
        uint8 type;       // How the value is to be displayed
        bool  alwaysShow; // If not set the stat is only shown if its value is > 0
        bool  special;    // Stat needs special handling (it's not a plain value)
        uint8 pad;
    };
    static_assert(sizeof(tStatListEntry) == 6);

    static constexpr uintptr_t s_StatLists[]{ 0x8CD8A0, 0x8CD938, 0x8CD9B0, 0x8CD9D8, 0x8CDA70, 0x8CDAC8, 0x8CDBF8 };
    const auto* const list = reinterpret_cast<const tStatListEntry*>(category < std::size(s_StatLists) ? s_StatLists[category] : uintptr_t{ 0x8CDE08 }); // Last one is `StatsMiscList`

    // Value of the stat without any checks (the lists may contain anything)
    const auto GetRawValue = [](int32 stat) {
        return stat >= (int32)FIRST_UNUSED_STAT
            ? (float)StatTypesInt.data()[stat - (int32)FIRST_INT_STAT]
            : StatTypesFloat[stat];
    };

    int32 line = 0;

    // Is the line about to be built the requested one? If not, it's counted and skipped.
    const auto IsRequestedLine = [&] {
        if (line == lineId) {
            return true;
        }
        line++;
        return false;
    };

    // 0x55A8E9 - `gString` holds the key of the stat's name, `gString2` the value
    const auto FinishStatLine = [] {
        GxtCharStrcpy(gGxtString, TheText.Get(gString));
        FilterOutTokensFromString(gGxtString);
        AsciiToGxtChar(gString2, gGxtString2);
    };

    // Line without a name, just a text
    const auto FinishTextLine = [](const char* key) {
        gGxtString[0] = 0;
        GxtCharStrcpy(gGxtString2, TheText.Get(key));
    };

    // `value` out of `total`
    const auto FinishOutOfLine = [&](int32 value, int32 total) {
        gString2[0] = '\0';
        sprintf_s(gString2, " %d %s %d", value, GxtCharToAscii(TheText.Get("FEST_OO"), 0), total);
        FinishStatLine();
    };

    const auto FinishIntLine = [&](int32 value) {
        gString2[0] = '\0';
        sprintf_s(gString2, "%d", value);
        FinishStatLine();
    };

    // NOTE: The original keeps the values on the x87 stack, so they are promoted to double without rounding to float first
    const auto FinishFloatLine = [&](const char* fmt, double value) {
        gString2[0] = '\0';
        sprintf_s(gString2, fmt, value);
        FinishStatLine();
    };

    const auto FinishBlankLine = [&] {
        gString2[0] = '\0';
        FinishStatLine();
    };

    // NOTE: Many of the comparisons can never be true, that's how the original code is
    const auto GetPilotRanking = [](int32 hours, int32 mins) -> const char* {
        if (hours <= 0) {
            if (mins < 5) {
                return nullptr;
            }
            if (mins < 10) {
                return "ST_PR01";
            }
            if (mins < 20) {
                return "ST_PR02";
            }
            if (mins < 30) {
                return "ST_PR03";
            }
        }
        if (hours <= 1) {
            if (mins < 0) {
                return "ST_PR04";
            }
            if (mins < 30) {
                return "ST_PR05";
            }
        }
        if (hours <= 2) {
            if (mins < 0) {
                return "ST_PR06";
            }
            if (mins < 30) {
                return "ST_PR07";
            }
        }
        if (hours <= 3) {
            if (mins < 0) {
                return "ST_PR08";
            }
            if (mins < 30) {
                return "ST_PR09";
            }
        }
        if (hours <= 4 && mins < 0) {
            return "ST_PR10";
        }
        if (hours <= 5 && mins < 0) {
            return "ST_PR11";
        }
        if (hours <= 10 && mins < 0) {
            return "ST_PR12";
        }
        if (hours <= 20 && mins < 0) {
            return "ST_PR13";
        }
        if (hours <= 25 && mins < 0) {
            return "ST_PR14";
        }
        if (hours <= 30 && mins < 0) {
            return "ST_PR15";
        }
        if (hours <= 49 && mins < 2) {
            return "ST_PR16";
        }
        if (hours <= 50 && mins < 0) {
            return "ST_PR17";
        }
        if (hours > 100 || mins >= 0) {
            return "ST_PR19";
        }
        return "ST_PR18";
    };

    for (uint16 i = 0; list[i].stat != -99; i++) {
        const auto& entry = list[i];

        if (!entry.alwaysShow && !(GetRawValue((uint16)entry.stat) > 0.0f)) {
            continue;
        }

        const int32 stat = entry.stat;
        sprintf_s(gString, stat < 10 ? "STAT00%d" : stat < 100 ? "STAT0%d" : "STAT%d", stat);

        if (CLocalisation::GermanGame()) {
            // NOTE: Only the low byte of the stat is checked
            switch ((uint8)stat) {
            case STAT_RAMPAGES_ATTEMPTED:                              // 167
            case STAT_RAMPAGES_PASSED:                                 // 168
            case STAT_TOTAL_LEGITIMATE_KILLS:                          // 177
            case STAT_HIGHEST_CIVILIAN_PEDS_KILLED_ON_RAMPAGE:         // 205
            case STAT_HIGHEST_POLICE_PEDS_KILLED_ON_RAMPAGE:           // 206
            case STAT_HIGHEST_CIVILIAN_VEHICLES_DESTROYED_ON_RAMPAGE:  // 207
            case STAT_HIGHEST_POLICE_VEHICLES_DESTROYED_ON_RAMPAGE:    // 208
            case STAT_HIGHEST_NUMBER_OF_TANKS_DESTROYED_ON_RAMPAGE:    // 209
                continue;
            default:
                break;
            }
        }

        m_ThisStatIsABarChart = 0;

        if (!entry.special) {
            switch (entry.type) {
            case 1: // Float
                if (IsRequestedLine()) {
                    FinishFloatLine("%.2f", GetStatValue((eStats)stat));
                    return 0;
                }
                break;
            case 3: // Money
                if (IsRequestedLine()) {
                    FinishFloatLine("$%.2f", GetStatValue((eStats)stat));
                    return 0;
                }
                break;
            case 4:
                if (IsRequestedLine()) {
                    gString2[0] = '\0';
                    sprintf_s(gString2, "%d|", (int32)GetStatValue((eStats)stat));
                    FinishStatLine();
                    return 0;
                }
                break;
            case 5: // Weight
                if (IsRequestedLine()) {
                    const auto v = (int32)GetStatValue((eStats)stat);
                    gString2[0] = '\0';
                    if (CLocalisation::Metric()) {
                        // BUG (OG): A double is passed to `%d` (kept as is)
                        sprintf_s(gString2, "%dkgs", (int32)((double)v * (double)0.4536f)); // NOTE: Original passes a double to `%d` here
                    } else {
                        sprintf_s(gString2, "%dlbs", v);
                    }
                    FinishStatLine();
                    return 0;
                }
                break;
            case 6: // Distance in miles
                if (IsRequestedLine()) {
                    const auto v = GetStatValue((eStats)stat);
                    gString2[0] = '\0';
                    sprintf_s(gString2, "%.2f %s", v, GxtCharToAscii(TheText.Get("ST_MILE"), 0));
                    FinishStatLine();
                    return 0;
                }
                break;
            case 7: // Height
                if (CLocalisation::Metric()) {
                    if (IsRequestedLine()) {
                        FinishFloatLine("%.2fm", GetStatValue((eStats)stat));
                        return 0;
                    }
                } else {
                    if (IsRequestedLine()) {
                        FinishFloatLine("%.2fft", (double)GetStatValue((eStats)stat) * (double)3.3333333f);
                        return 0;
                    }
                }
                break;
            case 9: // Time (minutes:seconds)
                if (IsRequestedLine()) {
                    int32 mins = ConvertToMins((int32)GetStatValue((eStats)stat));
                    int32 secs = ConvertToSecs((int32)GetStatValue((eStats)stat));
                    BuildStatLine(gString, &mins, 0, &secs, 1);
                    return 0;
                }
                break;
            case 10: // Bar chart
                m_ThisStatIsABarChart = (int16)stat;
                if (IsRequestedLine()) {
                    FinishIntLine((int32)GetStatValue((eStats)stat));
                    return 0;
                }
                break;
            case 2:
            case 8:
            default: // Integer
                if (IsRequestedLine()) {
                    FinishIntLine((int32)GetRawValue(stat));
                    return 0;
                }
                break;
            }
            continue;
        }

        if (stat >= STAT_PROGRESS_WITH_DENISE && stat <= STAT_PROGRESS_WITH_MILLIE) { // 252 .. 257 - Girlfriend progress
            if (GetRawValue(stat) > 0.0f) {
                if (IsRequestedLine()) {
                    FinishFloatLine("%0.2f%%", GetStatValue((eStats)stat));
                    return 0;
                }
            }
            continue;
        }

        switch (stat) {
        case STAT_PROGRESS_MADE: // 0
            if (IsRequestedLine()) {
                FinishFloatLine("%0.2f%%", GetPercentageProgress());
                return 0;
            }
            break;
        case STAT_BEST_INSANE_STUNT_AWARDED: { // 143
            if (IsRequestedLine()) {
                FinishBlankLine();
                return 0;
            }
            static constexpr const char* s_StuntNames[]{ "INSTUN", "PRINST", "DBINST", "DBPINS", "TRINST", "PRTRST", "QUINST", "PQUINS" };
            const auto stunt = (uint32)(StatTypesInt[STAT_BEST_INSANE_STUNT_AWARDED - FIRST_INT_STAT] - 1);
            if (IsRequestedLine()) {
                FinishTextLine(stunt < std::size(s_StuntNames) ? s_StuntNames[stunt] : "NOSTUC");
                return 0;
            }
            break;
        }
        case STAT_UNIQUE_JUMPS_FOUND: // 144
        case STAT_UNIQUE_JUMPS_DONE:  // 145
            if (IsRequestedLine()) {
                FinishOutOfLine(StatTypesInt[stat - FIRST_INT_STAT], CStuntJumpManager::m_iNumJumps);
                return 0;
            }
            break;
        case STAT_ARMOR: // 164
            if (IsRequestedLine()) {
                FinishIntLine((int32)FindPlayerPed(0)->m_fArmour);
                return 0;
            }
            break;
        case STAT_FLIGHT_TIME: { // 169
            const auto totalMins = (int32)((double)StatTypesInt[STAT_FLIGHT_TIME - FIRST_INT_STAT] * (double)(1.0f / 60'000.0f));
            if (IsRequestedLine()) {
                int32 hours = totalMins / 60;
                int32 mins  = totalMins % 60;
                BuildStatLine(gString, &hours, 0, &mins, 1);
                return 0;
            }
            break;
        }
        case STAT_TIME_ON_JETPACK: { // 173
            const auto totalMins = (int32)((double)StatTypesInt[STAT_TIME_ON_JETPACK - FIRST_INT_STAT] * (double)(1.0f / 60'000.0f));
            int32      hours     = totalMins / 60;
            int32      mins      = totalMins % 60;
            if (hours > 0 || mins > 0) {
                if (IsRequestedLine()) {
                    BuildStatLine(gString, &hours, 0, &mins, 1);
                    return 0;
                }
            }
            break;
        }
        case STAT_SHOOTING_RANGE_LEVELS_PASSED: // 174
            if (IsRequestedLine()) {
                FinishOutOfLine(StatTypesInt[stat - FIRST_INT_STAT], 12);
                return 0;
            }
            break;
        case STAT_MOST_CARS_PARKED_ON_VALET_PARKING: // 175
            if (IsRequestedLine()) {
                FinishOutOfLine(StatTypesInt[stat - FIRST_INT_STAT], 25);
                return 0;
            }
            break;
        case STAT_NUMBER_OF_VEHICLES_EXPORTED: // 213
            if (IsRequestedLine()) {
                FinishOutOfLine(StatTypesInt[stat - FIRST_INT_STAT], 30);
                return 0;
            }
            break;
        case STAT_SNAPSHOTS_TAKEN:      // 231
        case STAT_HORSESHOES_COLLECTED: // 241
        case STAT_OYSTERS_COLLECTED:    // 243
            if (IsRequestedLine()) { // The total is stored in the next stat
                FinishOutOfLine(StatTypesInt[stat - FIRST_INT_STAT], StatTypesInt[stat + 1 - FIRST_INT_STAT]);
                return 0;
            }
            break;
        case STAT_PLAYING_TIME: { // 320
            const auto totalMins = (int32)(CTimer::GetTimeInMS() / 60'000u);
            if (IsRequestedLine()) {
                int32 hours = totalMins / 60;
                int32 mins  = totalMins % 60;
                BuildStatLine(gString, &hours, 0, &mins, 1);
                return 0;
            }
            break;
        }
        case STAT_TAGS_SPRAYED: // 322
            if (IsRequestedLine()) {
                FinishOutOfLine(CTagManager::ms_numTagged, CTagManager::ms_numTags);
                return 0;
            }
            break;
        case STAT_LEAST_FAVORITE_GANG: { // 323
            int32 gang = 0, mostKills = 0;
            for (int32 pedType = PED_TYPE_GANG1; pedType <= PED_TYPE_GANG8; pedType++) {
                if (PedsKilledOfThisType[pedType] > mostKills) {
                    mostKills = PedsKilledOfThisType[pedType];
                    gang      = pedType;
                }
            }
            if (!gang) {
                break;
            }
            if (IsRequestedLine()) {
                FinishBlankLine();
                return 0;
            }
            if (IsRequestedLine()) {
                char key[16];
                sprintf_s(key, "ST_GNG%d", gang - PED_TYPE_GANG1);
                FinishTextLine(key);
                return 0;
            }
            break;
        }
        case STAT_GANG_MEMBERS_WASTED: // 324
            if (IsRequestedLine()) {
                int32 total = 0;
                for (int32 pedType = PED_TYPE_GANG1; pedType <= PED_TYPE_GANG9; pedType++) {
                    total += PedsKilledOfThisType[pedType];
                }
                FinishIntLine(total);
                return 0;
            }
            break;
        case STAT_CRIMINALS_WASTED: // 325
            if (IsRequestedLine()) {
                FinishIntLine(PedsKilledOfThisType[PED_TYPE_CRIMINAL]);
                return 0;
            }
            break;
        case STAT_MOST_FAVORITE_RADIO_STATION:    // 326
        case STAT_LEAST_FAVORITE_RADIO_STATION: { // 327
            if (PopulateFavoriteRadioStationList()) { // Nothing listened to yet
                break;
            }
            if (IsRequestedLine()) {
                FinishBlankLine();
                return 0;
            }
            char key[8];
            AudioEngine.GetRadioStationNameKey(
                stat == STAT_MOST_FAVORITE_RADIO_STATION ? FindMostFavoriteRadioStation() : (eRadioID)FindLeastFavoriteRadioStation(),
                key
            );
            if (IsRequestedLine()) {
                FinishTextLine(key);
                return 0;
            }
            break;
        }
        case STAT_CURRENT_WEAPON_SKILL: { // 328
            auto weaponType = (int32)FindPlayerPed()->GetActiveWeapon().m_Type;
            if (weaponType == WEAPON_TEC9) {
                weaponType = WEAPON_MICRO_UZI; // They share the skill stat
            } else if (weaponType < WEAPON_PISTOL || weaponType > WEAPON_TEC9) {
                break;
            }
            const auto skillIdx = (uint8)(weaponType - (WEAPON_PISTOL - 1)); // 1 based
            if (IsRequestedLine()) {
                FinishIntLine((int32)GetStatValue((eStats)(STAT_PISTOL_SKILL - 1 + skillIdx)));
                return 0;
            }
            if (FindPlayerPed()->GetActiveWeapon().m_Type == WEAPON_TEC9) {
                sprintf_s(gString, "STWE0%d", 11);
            } else {
                sprintf_s(gString, skillIdx < 10 ? "STWE00%d" : skillIdx < 100 ? "STWE0%d" : "STWE%d", (uint32)skillIdx);
            }
            if (IsRequestedLine()) {
                FinishTextLine(gString);
                return 0;
            }
            break;
        }
        case STAT_WEAPON_SKILL_LEVELS: { // 329
            if (IsRequestedLine()) {
                FinishBlankLine();
                return 0;
            }
            for (int32 skillIdx = 1; skillIdx < 11; skillIdx++) {
                sprintf_s(gString, skillIdx < 10 ? "STWE00%d" : skillIdx < 100 ? "STWE0%d" : "STWE%d", skillIdx);
                switch (FindPlayerPed()->GetWeaponSkill((eWeaponType)(WEAPON_PISTOL - 1 + skillIdx))) {
                case eWeaponSkill::STD:
                    sprintf_s(gString2, "WS_STD");
                    break;
                case eWeaponSkill::PRO:
                    sprintf_s(gString2, "WS_PRO");
                    break;
                default:
                    sprintf_s(gString2, "WS_POOR");
                    break;
                }
                if (IsRequestedLine()) {
                    GxtCharStrcpy(gGxtString, TheText.Get(gString));
                    GxtCharStrcpy(gGxtString2, TheText.Get(gString2));
                    return 0;
                }
            }
            break;
        }
        case STAT_PILOT_RANKING: { // 330
            const auto totalMins = (int32)((double)StatTypesInt[STAT_FLIGHT_TIME - FIRST_INT_STAT] * (double)(1.0f / 60'000.0f));
            const auto hours     = totalMins / 60;
            const auto mins      = totalMins % 60;
            if (hours > 0 || mins >= 5) {
                if (IsRequestedLine()) {
                    FinishBlankLine();
                    return 0;
                }
            }
            if (const auto ranking = GetPilotRanking(hours, mins)) {
                if (IsRequestedLine()) {
                    FinishTextLine(ranking);
                    return 0;
                }
            }
            break;
        }
        case STAT_STRONGEST_GANG:       // 331
        case STAT_2ND_STRONGEST_GANG:   // 332
        case STAT_3RD_STRONGEST_GANG: { // 333
            const auto place    = stat - STAT_STRONGEST_GANG;
            const auto gang     = CGangWars::GangRatings[place];
            const auto strength = CGangWars::GangRatingStrength[place];
            if (gang < 0) {
                break;
            }
            sprintf_s(gString2, "ST_GNG%d", gang);
            sprintf_s(gString, "ST_LAB%d", place);
            const auto* const label = TheText.Get(gString);
            if (IsRequestedLine()) {
                GxtCharStrcpy(gGxtString, label);
                GxtCharStrcpy(gGxtString2, TheText.Get(gString2));
                return 0;
            }
            sprintf_s(gString, "%d", strength);
            AsciiToGxtChar(gString, gGxtString2);
            if (IsRequestedLine()) {
                gGxtString[0] = 0; // (The original also copies `gGxtString2` onto itself here)
                return 0;
            }
            break;
        }
        case STAT_MONEY_LOST_GAMBLING: // 334
            if (IsRequestedLine()) {
                FinishFloatLine("$%.2f", std::max((double)StatTypesFloat[STAT_MONEY_SPENT_GAMBLING] - (double)StatTypesFloat[STAT_MONEY_WON_GAMBLING], 0.0));
                return 0;
            }
            break;
        case STAT_TERRITORY_UNDER_CONTROL: // 337
            if (IsRequestedLine()) {
                FinishFloatLine("%0.2f%%", (double)CGangWars::TerritoryUnderControlPercentage * 100.0);
                return 0;
            }
            break;
        default:
            break;
        }
    }

    return line;
}

// 0x55B900
void CStats::ProcessReactionStatsOnIncrement(eStats stat) {
    if (stat != STAT_STAMINA && stat != STAT_ENERGY && stat != STAT_LUNG_CAPACITY)
        return;

    float energy = GetStatValue(STAT_ENERGY);

    if (stat == STAT_STAMINA || stat == STAT_LUNG_CAPACITY) {
        if (energy < 0.0f) {
            StatTypesFloat[STAT_FAT] = std::max(StatTypesFloat[STAT_FAT] - 23.0f, 0.0f);
            CheckForStatsMessage();
        }
        return;
    }

    if (energy > 1000.0f)
        IncrementStat(STAT_FAT, energy - 1000.0f);
}

// 0x55B980
void CStats::DisplayScriptStatUpdateMessage(eStatUpdateState state, eStats stat, float value) {
    if (CPad::GetPad(0)->JustOutOfFrontEnd
        || !bShowUpdateStats
        || TheCamera.m_bWideScreenOn
        || CHud::HelpMessageDisplayed()
        || bStatUpdateMessageDisplayed
        || CMenuSystem::num_menus_in_use)
    {
        bStatUpdateMessageDisplayed = false;

        return;
    }

    if (IsStatCapped(stat) && GetStatValue(stat) >= 1000.0f)
        return;

    switch (stat) {
    case STAT_FAT:
    case STAT_STAMINA:
    case STAT_MUSCLE:
    case STAT_MAX_HEALTH:
    case STAT_SEX_APPEAL:
    case STAT_PISTOL_SKILL:
    case STAT_SILENCED_PISTOL_SKILL:
    case STAT_DESERT_EAGLE_SKILL:
    case STAT_SHOTGUN_SKILL:
    case STAT_SAWN_OFF_SHOTGUN_SKILL:
    case STAT_COMBAT_SHOTGUN_SKILL:
    case STAT_MACHINE_PISTOL_SKILL:
    case STAT_SMG_SKILL:
    case STAT_AK_47_SKILL:
    case STAT_M4_SKILL:
    case STAT_RIFLE_SKILL:
    case STAT_GAMBLING:
    case STAT_DRIVING_SKILL:
    case STAT_ARMOR:
    case STAT_ENERGY:
        if (value > 1.0f)
            CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_TOTAL_RESPECT:
        CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_FLYING_SKILL:
    case STAT_LUNG_CAPACITY:
    case STAT_BIKE_SKILL:
    case STAT_CYCLING_SKILL:
    case STAT_LUCK:
        if (value > 1.0f)
            CHud::SetHelpMessageStatUpdate(state, stat, value, 1000.0f);
        break;

    case STAT_PROGRESS_WITH_DENISE:
    case STAT_PROGRESS_WITH_MICHELLE:
    case STAT_PROGRESS_WITH_HELENA:
    case STAT_PROGRESS_WITH_BARBARA:
    case STAT_PROGRESS_WITH_KATIE:
    case STAT_PROGRESS_WITH_MILLIE:
        CHud::SetHelpMessageStatUpdate(state, stat, value, 100.0f);
        break;

    case STAT_PIMPING_LEVEL:
        CHud::SetHelpMessageStatUpdate(state, STAT_PIMPING_LEVEL, value, 10.0f);
        break;

    case STAT_GANG_STRENGTH: {
        if (auto player = FindPlayerPed())
        {
            auto maxGroup = std::min<uint8>(FindMaxNumberOfGroupMembers(), player->GetPlayerData()->m_nScriptLimitToGangSize);
            CHud::SetHelpMessageStatUpdate(state, stat, value, maxGroup);
        }
        break;
    }
    default:
        return;
    }
}

// 0x55BC50
void CStats::UpdateRespectStat(uint8 arg0) {
    static auto& s_LastThresholdValue = StaticRef<float>(0x8CDEC4); // = -99.f
    static auto& s_LastValue          = StaticRef<float>(0x8CDEC8); // = -99.f

    if (arg0) { // Reset
        s_LastValue          = -99.0f;
        s_LastThresholdValue = -99.0f;
        return;
    }

    const auto missionTotal = std::max((float)StatTypesInt[STAT_RESPECT_MISSION_TOTAL - FIRST_INT_STAT], 1.0f);
    const auto territory    = std::max(CGangWars::TerritoryUnderControlPercentage - 0.2f, 0.0f);
    const auto money        = std::min((float)CWorld::Players[CWorld::PlayerInFocus].m_nMoney * 0.0000001f, 1.0f);
    const auto muscle       = StatTypesFloat[STAT_MUSCLE];
    const auto clothes      = StatTypesFloat[STAT_CLOTHES_RESPECT];

    auto respect = territory * 1250.0f * 0.05f
        + (StatTypesFloat[STAT_RESPECT] * 0.4f
            + (float)StatTypesInt[STAT_RESPECT_MISSION - FIRST_INT_STAT] * 1000.0f / missionTotal * 0.36f
            + StatTypesFloat[STAT_GIRLFRIEND_RESPECT] * 0.03f);

    respect = (float)(CTagManager::GetPercentageTagged() * 10) * 0.05f
        + (money * 1000.0f * 0.05f + respect + muscle * 0.03f + clothes * 0.03f);

    if (respect < 0.0f) {
        respect = 0.0f;
    }
    if (CCheat::IsActive(CHEAT_MAX_RESPECT)) {
        respect = 1000.0f;
    }

    if ((int32)s_LastValue == (int32)respect) {
        return;
    }

    StatTypesFloat[STAT_TOTAL_RESPECT] = respect;
    CheckForStatsMessage();

    if (respect > s_LastThresholdValue || s_LastValue < 2.0f) { // Increased
        if (s_LastValue != -99.0f && s_LastThresholdValue != -99.0f) {
            if (CheckForThreshold(&s_LastThresholdValue, respect) || s_LastValue < 2.0f) {
                DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_TOTAL_RESPECT, respect);
            }
        }
        if (respect < s_LastValue) {
            s_LastThresholdValue = respect;
        }
    } else { // Decreased
        if (s_LastValue != -99.0f && s_LastThresholdValue != -99.0f) {
            if (CheckForThreshold(&s_LastThresholdValue, respect) || respect < 2.0f) {
                DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_TOTAL_RESPECT, respect);
            }
        }
        if (respect > s_LastValue) {
            s_LastThresholdValue = respect;
        }
    }
    s_LastValue = respect;
}

// 0x55BF20
void CStats::UpdateSexAppealStat() {
    static auto& s_LastPlayerVehicle = StaticRef<CVehicle*>(0xB79530);

    const auto appearance = StatTypesFloat[STAT_APPEARANCE] * 0.5f;

    if (FindPlayerVehicle()) {
        s_LastPlayerVehicle = FindPlayerVehicle();
        s_LastPlayerVehicle->RegisterReference(reinterpret_cast<CEntity**>(&s_LastPlayerVehicle));
    }

    auto vehicleAppeal = 0.0f;
    if (const auto veh = s_LastPlayerVehicle; veh && DistanceBetweenPoints2D(veh->GetPosition(), FindPlayerCoors()) < 35.0f) {
        vehicleAppeal = veh->m_fHealth + veh->m_fHealth - 1000.0f;
        if (vehicleAppeal > 1000.0f) {
            vehicleAppeal = 1000.0f;
        } else if (vehicleAppeal < 0.0f) {
            vehicleAppeal = 0.0f;
        }

        switch (veh->GetVehicleModelInfo()->m_nVehicleClass) {
        case VEHICLE_CLASS_POORFAMILY:
        case VEHICLE_CLASS_BIG:
        case VEHICLE_CLASS_MOPED:
        case VEHICLE_CLASS_BICYCLE:
            vehicleAppeal *= 0.1f;
            break;
        case VEHICLE_CLASS_RICHFAMILY:
        case VEHICLE_CLASS_EXECUTIVE:
        case VEHICLE_CLASS_LEISUREBOAT:
            break;
        case VEHICLE_CLASS_WORKER:
        case VEHICLE_CLASS_TAXI:
        case VEHICLE_CLASS_WORKERBOAT:
            vehicleAppeal *= 0.3f;
            break;
        default:
            vehicleAppeal *= 0.5f;
            break;
        }
    }

    auto sexAppeal = vehicleAppeal * 0.5f + appearance;
    if (sexAppeal < 0.0f) {
        sexAppeal = 0.0f;
    } else if (sexAppeal > 1000.0f) {
        sexAppeal = 1000.0f;
    }
    if (CCheat::IsActive(CHEAT_MAX_SEX_APPEAL)) {
        sexAppeal = 1000.0f;
    }

    StatTypesFloat[STAT_SEX_APPEAL] = sexAppeal;
    CheckForStatsMessage();
}

// 0x55C180
void CStats::IncrementStat(eStats stat, float value)
{
    if (value <= 0.0f)
        return;

    if (IsStatFloat(stat)) { // float
        StatTypesFloat[stat] += value;

        if (IsStatCapped(stat))
            StatTypesFloat[stat] = std::min(StatTypesFloat[stat], 1000.0f);

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    CPlayerPed* player = FindPlayerPed();
    CPlayerInfo* playerInfo = player->GetPlayerInfoForThisPlayerPed();

    if (stat == STAT_CALORIES) {
        float healthDiff = playerInfo->m_nMaxHealth - player->m_fHealth;

        IncrementStat(STAT_RIOT_MISSION_ACCOMPLISHED, value);

        if (value > healthDiff) {
            float avg = (value - healthDiff) / 2.0f;

            IncrementStat(STAT_FAT, avg);
        }

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    if (stat != STAT_RIOT_MISSION_ACCOMPLISHED) {
        assert(stat >= FIRST_INT_STAT);

        StatTypesInt[stat - FIRST_INT_STAT] += (int32)value;

        if (IsStatCapped(stat))
            StatTypesInt[stat - FIRST_INT_STAT] = std::min(StatTypesInt[stat - FIRST_INT_STAT], 1000);

        ProcessReactionStatsOnIncrement(stat);
        CheckForStatsMessage();

        return;
    }

    // STAT_RIOT_MISSION_ACCOMPLISHED increment, enum name incorrect?

    float kcals = playerInfo->m_nNumHoursDidntEat - value / 2.0f;
    kcals = std::clamp(kcals, 0.0f, 36.0f);

    float healthDiff = playerInfo->m_nMaxHealth - player->m_fHealth;

    if (value >= healthDiff) {
        playerInfo->m_nNumHoursDidntEat = 0;
    }

    player->m_fHealth += value;
    UpdateStatsAddToHealth((uint32)value);
    ProcessReactionStatsOnIncrement(stat);
    CheckForStatsMessage();
}

// 0x55C470
void CStats::UpdateFatAndMuscleStats(uint32 value) {
    static auto& s_LastMessageShown = StaticRef<int32>(0xB79534);

    if (StatReactionValue[STAT_TIMELIMIT_FAT_ADJUST] * 1000.0f >= static_cast<float>(m_FatCounter)) {
        m_FatCounter += static_cast<uint32>(CTimer::GetTimeStepInMS()) * value / 10;
    } else {
        m_FatCounter = 0;
        if (StatTypesFloat[STAT_FAT] <= 0.0f) { // No fat left to burn, so burn muscle
            if (const auto dec = StatReactionValue[STAT_DEC_BODY_MUSCLE]; dec > 0.0f) {
                StatTypesFloat[STAT_MUSCLE] = std::max(StatTypesFloat[STAT_MUSCLE] - dec, 0.0f);
                CheckForStatsMessage();
            }
            s_LastMessageShown = 3;
            DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MUSCLE, StatReactionValue[STAT_DEC_BODY_MUSCLE]);
        } else {
            if (const auto dec = StatReactionValue[STAT_DEC_FAT]; dec > 0.0f) {
                StatTypesFloat[STAT_FAT] = std::max(StatTypesFloat[STAT_FAT] - dec, 0.0f);
                CheckForStatsMessage();
            }
            IncrementStat(STAT_MUSCLE, StatReactionValue[STAT_INC_BODY_MUSCLE]);
            if (s_LastMessageShown == 1) { // Alternate between the 2 messages
                s_LastMessageShown = 2;
                DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MUSCLE, StatReactionValue[STAT_INC_BODY_MUSCLE]);
            } else {
                s_LastMessageShown = 1;
                DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_FAT, StatReactionValue[STAT_DEC_FAT]);
            }
        }
    }

    if (StatReactionValue[STAT_TIMELIMIT_MAX_HEALTH] * 1000.0f >= static_cast<float>(m_MaxHealthCounter)) {
        m_MaxHealthCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        m_MaxHealthCounter = 0;
    }
}

// 0x55C660
void CStats::UpdateStatsWhenSprinting() {
    UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[STAT_EXERCISE_RATE_SPRINT]));
    if (StatReactionValue[STAT_TIMELIMIT_SPRINT_STAMINA] * 1000.0f >= static_cast<float>(m_SprintStaminaCounter)) {
        m_SprintStaminaCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        m_SprintStaminaCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_SPRINT_STAMINA]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_SPRINT_STAMINA]);
    }
}

// 0x55C6F0
void CStats::UpdateStatsWhenRunning() {
    UpdateFatAndMuscleStats((uint32)StatReactionValue[STAT_EXERCISE_RATE_RUN]);
    if (StatReactionValue[STAT_TIMELIMIT_RUNNING] * 1000.0f >= static_cast<float>(m_RunningCounter)) {
        m_RunningCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        m_RunningCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_RUNNING]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_RUNNING]);
    }
}

// 0x55C780
void CStats::UpdateStatsWhenCycling(bool arg0, CBmx* bmx) {
    const auto isSprinting = arg0;

    if (std::fabs(bmx->m_GasPedal) > 0.0f || isSprinting) {
        UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[isSprinting ? STAT_EXERCISE_RATE_CYCLE_SPRINT : STAT_EXERCISE_RATE_CYCLE]));
        if (StatReactionValue[STAT_TIMELIMIT_CYCLE_STAMINA] * 1000.0f >= static_cast<float>(m_CycleStaminaCounter)) {
            m_CycleStaminaCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
        } else {
            m_CycleStaminaCounter = 0;
            IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_CYCLE_STAMINA]);
            DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_CYCLE_STAMINA]);
        }
    }

    if (StatReactionValue[STAT_TIMELIMIT_CYCLE_SKILL] * 1000.0f < static_cast<float>(m_CycleSkillCounter)) {
        m_CycleSkillCounter = 0;
        IncrementStat(STAT_CYCLING_SKILL, StatReactionValue[STAT_INC_CYCLE_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_CYCLING_SKILL, StatReactionValue[STAT_INC_CYCLE_SKILL]);
        return;
    }

    const auto speedSq = bmx->m_vecMoveSpeed.SquaredMagnitude();
    float mult;
    if (bmx->m_nNoOfContactWheels < 2 && speedSq > sq(0.05f)) { // Wheelie/Stoppie/In air
        mult = 3.0f;
    } else if (speedSq > sq(0.2f)) {
        mult = isSprinting ? 1.5f : 1.0f;
    } else {
        return;
    }
    m_CycleSkillCounter += static_cast<uint32>(std::ceil(mult * static_cast<float>(static_cast<uint32>(CTimer::GetTimeStepInMS()))));
}

// 0x55C990
void CStats::UpdateStatsWhenSwimming(bool arg0, bool arg1) {
    const auto isUnderWater = arg0, isSprinting = arg1;

    UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[isUnderWater || isSprinting ? STAT_EXERCISE_RATE_SWIM_SPRINT : STAT_EXERCISE_RATE_SWIM]));

    if (StatReactionValue[STAT_TIMELIMIT_SWIM_STAMINA] * 1000.0f >= static_cast<float>(m_SwimStaminaCounter)) {
        m_SwimStaminaCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
    } else {
        m_SwimStaminaCounter = 0;
        IncrementStat(STAT_STAMINA, StatReactionValue[STAT_INC_SWIM_STAMINA]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_STAMINA, StatReactionValue[STAT_INC_SWIM_STAMINA]);
    }

    if (isUnderWater) {
        if (StatReactionValue[STAT_TIMELIMIT_BREATH_UNDERWATER] * 1000.0f >= static_cast<float>(m_SwimUnderWaterCounter)) {
            m_SwimUnderWaterCounter += static_cast<uint32>(CTimer::GetTimeStepInMS());
        } else {
            m_SwimUnderWaterCounter = 0;
            IncrementStat(STAT_LUNG_CAPACITY, StatReactionValue[STAT_INC_BREATH_UNDERWATER]);
            DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_LUNG_CAPACITY, StatReactionValue[STAT_INC_BREATH_UNDERWATER]);
        }
    }
}

// 0x55CAC0
void CStats::UpdateStatsWhenDriving(CVehicle* vehicle) {
    const auto automobile = vehicle->IsAutomobile() ? static_cast<CAutomobile*>(vehicle) : nullptr;
    const auto counter    = static_cast<float>(m_DrivingCounter);

    if (StatReactionValue[STAT_TIMELIMIT_DRIVING_SKILL] * 1000.0f < counter) {
        m_DrivingCounter = 0;
        IncrementStat(STAT_DRIVING_SKILL, StatReactionValue[STAT_INC_DRIVING_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_DRIVING_SKILL, StatReactionValue[STAT_INC_DRIVING_SKILL]);
        return;
    }

    const auto speed    = vehicle->m_vecMoveSpeed.Magnitude();
    const auto timeStep = static_cast<float>(static_cast<uint32>(CTimer::GetTimeStepInMS()));
    if (speed > 0.8f || (automobile && automobile->m_nNumContactWheels == 0)) {
        m_DrivingCounter = static_cast<uint32>(timeStep * 1.5f + counter);
    } else if (speed > 0.2f) {
        m_DrivingCounter = static_cast<uint32>(timeStep * 0.5f + counter);
    }
}

// 0x55CC00
void CStats::UpdateStatsWhenFlying(CVehicle* vehicle) {
    if (!vehicle->IsAutomobile()) {
        return;
    }

    const auto counter = static_cast<float>(m_FlyingCounter);
    if (StatReactionValue[STAT_TIMELIMIT_FLYING_SKILL] * 1000.0f < counter) {
        m_FlyingCounter = 0;
        IncrementStat(STAT_FLYING_SKILL, StatReactionValue[STAT_INC_FLYING_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_FLYING_SKILL, StatReactionValue[STAT_INC_FLYING_SKILL]);
        return;
    }

    if (static_cast<CAutomobile*>(vehicle)->m_nNumContactWheels != 0) { // On the ground
        return;
    }

    const auto speed    = vehicle->m_vecMoveSpeed.Magnitude();
    const auto timeStep = static_cast<float>(static_cast<uint32>(CTimer::GetTimeStepInMS()));
    if (speed > 1.3f || vehicle->GetMatrix().GetUp().z < 0.0f) { // Fast or upside down
        m_FlyingCounter = static_cast<uint32>(timeStep * 1.5f + counter);
    } else if (speed > 0.5f) {
        m_FlyingCounter = static_cast<uint32>(timeStep * 0.5f + counter);
    }
}

// 0x55CD60
void CStats::UpdateStatsWhenOnMotorBike(CBike* bike) {
    auto bikeCounter = static_cast<float>(m_BikeCounter);
    if (StatReactionValue[STAT_TIMELIMIT_MOTORBIKE_SKILL] * 1000.0f >= bikeCounter) {
        const float bikeMoveSpeed = bike->m_vecMoveSpeed.Magnitude();
        const auto  fTimeStep = CTimer::GetTimeStepInMS();

        if (bikeMoveSpeed > 0.6f || bike->m_nNoOfContactWheels < 3u && bikeMoveSpeed > 0.1f)
            m_BikeCounter = static_cast<uint32>(fTimeStep * 1.5f + bikeCounter);
        else if (bikeMoveSpeed > 0.2f)
            m_BikeCounter = static_cast<uint32>(fTimeStep * 0.5f + bikeCounter);
    } else {
        m_BikeCounter = 0;
        IncrementStat(STAT_BIKE_SKILL, StatReactionValue[STAT_INC_MOTORBIKE_SKILL]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_BIKE_SKILL, StatReactionValue[STAT_INC_MOTORBIKE_SKILL]);
    }
}

// 0x55CEB0
void CStats::UpdateStatsWhenWeaponHit(eWeaponType weaponType) {
    const auto skillStat  = CWeaponInfo::GetSkillStatIndex(weaponType);
    const auto skillValue = GetStatValue(skillStat);

    if (CGameLogic::IsCoopGameGoingOn() || skillValue >= 1000.0f) {
        return;
    }

    const auto weaponIdx = static_cast<uint32>(skillStat - STAT_PISTOL_SKILL);
    const auto incValue  = StatReactionValue[STAT_INC_PISTOL_SKILL + weaponIdx];

    IncrementStat(skillStat, incValue);

    if (m_LastWeaponTypeFired != weaponIdx) {
        m_LastWeaponTypeFired = weaponIdx;
        m_WeaponCounter       = 0;
        return;
    }

    if (static_cast<float>(m_WeaponCounter) > StatReactionValue[STAT_TIMELIMIT_PISTOL_SKILL + weaponIdx]) {
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, skillStat, static_cast<float>(m_WeaponCounter) * incValue);
        m_WeaponCounter = 0;
    } else {
        m_WeaponCounter++;
    }
}

// 0x55CFA0
void CStats::UpdateStatsWhenFighting() {
    UpdateFatAndMuscleStats(static_cast<uint32>(StatReactionValue[STAT_EXERCISE_RATE_FIGHT]));
}

// 0x55CFC0
void CStats::UpdateStatsOnRespawn() {
    if (static_cast<float>(m_DeathCounter) <= StatReactionValue[STAT_TIMELIMIT_DEATH_HEALTH]) {
        m_DeathCounter++;
        return;
    }

    if (StatTypesFloat[STAT_MAX_HEALTH] > 400.0f) {
        // NOTE: Yes, the game calls `IncrementStat` here with the "decrease" value
        IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_DEC_MAX_HEALTH]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_DECREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_DEC_MAX_HEALTH]);
    }
    m_DeathCounter = 0;
}

// 0x55D030
void CStats::UpdateStatsAddToHealth(uint32 addToHealth) {
    m_AddToHealthCounter += addToHealth;
    if (static_cast<float>(m_AddToHealthCounter) > StatReactionValue[STAT_TIMELIMIT_ADD_TO_HEALTH]) {
        IncrementStat(STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        DisplayScriptStatUpdateMessage(STAT_UPDATE_INCREASE, STAT_MAX_HEALTH, StatReactionValue[STAT_INC_MAX_HEALTH]);
        m_AddToHealthCounter = 0;
    }
}

// 0x55D090
void CStats::ModifyStat(eStats stat, float value) {
    if (value < 0.0f) {
        CStats::DecrementStat(stat, -value);
    } else {
        CStats::IncrementStat(stat, value);
    }
}

// 0x5D3B40
bool CStats::Save() {
    IncrementStat(STAT_TOTAL_LEGITIMATE_KILLS, GetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT));
    SetStatValue(STAT_KILLS_SINCE_LAST_CHECKPOINT, 0.0f);

    CGenericGameStorage::SaveDataToWorkBuffer(StatTypesFloat);
    CGenericGameStorage::SaveDataToWorkBuffer(StatTypesInt);
    CGenericGameStorage::SaveDataToWorkBuffer(PedsKilledOfThisType);
    CGenericGameStorage::SaveDataToWorkBuffer(LastMissionPassedName);
    CGenericGameStorage::SaveDataToWorkBuffer(FavoriteRadioStationList);
    CGenericGameStorage::SaveDataToWorkBuffer(TimesMissionAttempted);
    // TODO: NOTSA: CGenericGameStorage::SaveDataToWorkBuffer(StatMessage);
    for (auto& statMessage : StatMessage) {
        CGenericGameStorage::SaveDataToWorkBuffer(statMessage.displayed);
    }
    return true;
}

// 0x5D3BF0
bool CStats::Load() {
    CGenericGameStorage::LoadDataFromWorkBuffer(StatTypesFloat);
    CGenericGameStorage::LoadDataFromWorkBuffer(StatTypesInt);
    CGenericGameStorage::LoadDataFromWorkBuffer(PedsKilledOfThisType);
    CGenericGameStorage::LoadDataFromWorkBuffer(LastMissionPassedName);
    CGenericGameStorage::LoadDataFromWorkBuffer(FavoriteRadioStationList);
    CGenericGameStorage::LoadDataFromWorkBuffer(TimesMissionAttempted);
    // TODO: NOTSA: CGenericGameStorage::LoadDataFromWorkBuffer(StatMessage);
    for (auto& statMessage : StatMessage) {
        CGenericGameStorage::LoadDataFromWorkBuffer(statMessage.displayed);
    }
    return true;
}

// Unused
// 0x558DE0
char* CStats::GetStatID(eStats stat) {
    if (!IsStatFloat(stat)) // int32
        sprintf_s(gString, "stat_i_%d", stat);
    else
        sprintf_s(gString, "stat_f_%d", stat);

    return gString;
}

// Unused
// 0x558E70
int8 CStats::GetTimesMissionAttempted(uint8 missionId) {
    return TimesMissionAttempted[missionId];
}

// Unused
// 0x558E80
void CStats::RegisterMissionAttempted(uint8 missionId) {
    if (TimesMissionAttempted[missionId] != -1) {
        TimesMissionAttempted[missionId]++;
    }
}

// Unused
// 0x558EA0
void CStats::RegisterMissionPassed(uint8 missionId) {
    TimesMissionAttempted[missionId] = -1;
}
