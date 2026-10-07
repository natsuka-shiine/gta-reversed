#include <StdInc.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

/*!
* Various Sequence commands
*/

namespace {
/*
 * @opcode 0615
 * @command OPEN_SEQUENCE_TASK
 * @class Sequence
 * @method Open
 *
 * @brief Begins a sequence of up to 8 tasks
 *
 * @returns {Sequence} handle
 */
int32 OpenSequenceTask(CRunningScript& S) { // 0x49186F
    const auto slot = CTaskSequences::GetAvailableSlot(S.m_UsesMissionCleanup);
    if (slot < 0 || slot >= MAX_NUM_SCRIPT_SEQUENCE_TASKS) {
        return -1;
    }
    CTaskSequences::ms_bIsOpened[slot] = true;
    CTaskSequences::ms_taskSequence[slot].Flush();
    CTaskSequences::ms_iActiveSequence = slot;
    const auto handle = CTheScripts::GetNewUniqueScriptThingIndex(slot, SCRIPT_THING_SEQUENCE_TASK);
    if (S.m_UsesMissionCleanup) {
        CTheScripts::MissionCleanUp.AddEntityToList(handle, MISSION_CLEANUP_ENTITY_TYPE_TASK_SEQUENCE);
    }
    return handle;
}

/*
 * @opcode 0616
 * @command CLOSE_SEQUENCE_TASK
 * @class Sequence
 * @method Close
 *
 * @brief Ends the task sequence
 *
 * @param {Sequence} self
 */
void CloseSequenceTask(int32 handle) { // 0x491908
    const auto index = CTheScripts::GetActualScriptThingIndex(handle, SCRIPT_THING_SEQUENCE_TASK);
    if (index < 0 || index >= MAX_NUM_SCRIPT_SEQUENCE_TASKS) {
        return;
    }
    CTaskSequences::ms_bIsOpened[index] = false;
    CTaskSequences::ms_iActiveSequence  = -1;
}

/*
 * @opcode 061B
 * @command CLEAR_SEQUENCE_TASK
 * @class Sequence
 * @method Clear
 *
 * @brief Clears the task sequence
 *
 * @param {Sequence} self
 */
void ClearSequenceTask(CRunningScript& S, int32 handle) { // 0x491A4A
    const auto index = CTheScripts::GetActualScriptThingIndex(handle, SCRIPT_THING_SEQUENCE_TASK);
    if (index >= 0 && index < MAX_NUM_SCRIPT_SEQUENCE_TASKS) {
        auto& sequence = CTaskSequences::ms_taskSequence[index];
        CTaskSequences::ms_bIsOpened[index] = false;
        if (sequence.m_RefCnt == 0) {
            sequence.m_bFlushTasks = false;
            sequence.Flush();
        } else { // Still in use, it's flushed when the last user is gone
            sequence.m_bFlushTasks = true;
        }
        CTheScripts::ScriptSequenceTaskArray[index].m_bUsed = false;
    }
    if (S.m_UsesMissionCleanup) { // Done for invalid handles too
        CTheScripts::MissionCleanUp.RemoveEntityFromList(handle, MISSION_CLEANUP_ENTITY_TYPE_TASK_SEQUENCE);
    }
}

/*
 * @opcode 0618
 * @command PERFORM_SEQUENCE_TASK
 * @class Char
 * @method PerformSequence
 *
 * @brief Assigns the character to the specified action sequence
 *
 * @param {Char} self
 * @param {Sequence} sequence
 */
void PerformSequenceTask(eScriptCommands command, CRunningScript& S, int32 pedHandle, int32 sequenceHandle) { // 0x49194B
    const auto index = CTheScripts::GetActualScriptThingIndex(sequenceHandle, SCRIPT_THING_SEQUENCE_TASK);
    if (index < 0 || index >= MAX_NUM_SCRIPT_SEQUENCE_TASKS) {
        return;
    }
    S.GivePedScriptedTask(
        pedHandle,
        new CTaskComplexUseSequence{ index },
        command
    );
}

void SetSequenceToRepeat(int32 index, bool repeat) {
    const auto actualIndex = CTheScripts::GetActualScriptThingIndex(index, SCRIPT_THING_SEQUENCE_TASK);
    if (actualIndex < 0 || actualIndex >= MAX_NUM_SCRIPT_SEQUENCE_TASKS) {
        NOTSA_LOG_DEBUG("COMMAND_SET_SEQUENCE_TO_REPEAT: Index should be in [0, 64], it is {}", actualIndex);
        return;
    }
    CTaskSequences::GetActiveSequence().SetRepeatMode(repeat);
}
};

void notsa::script::commands::sequence::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Sequence");

    REGISTER_COMMAND_HANDLER(COMMAND_OPEN_SEQUENCE_TASK, OpenSequenceTask);
    REGISTER_COMMAND_HANDLER(COMMAND_CLOSE_SEQUENCE_TASK, CloseSequenceTask);
    REGISTER_COMMAND_HANDLER(COMMAND_PERFORM_SEQUENCE_TASK, PerformSequenceTask);
    REGISTER_COMMAND_HANDLER(COMMAND_CLEAR_SEQUENCE_TASK, ClearSequenceTask);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_SEQUENCE_TO_REPEAT, SetSequenceToRepeat);
}
