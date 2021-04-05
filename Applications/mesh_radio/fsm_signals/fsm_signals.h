#ifndef APPLICATIONS_MESH_RADIO_RADIO_FSM_SIGNALS_H_
#define APPLICATIONS_MESH_RADIO_RADIO_FSM_SIGNALS_H_

////////////////////////////////////////////////////////////////////

#include "types.h"
#include "fsm_signals_output.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

ENUM(TX_FLAGS) WITH_MASK
    PARAM(confirm_required)
    PARAM(no_ack)

ENUM(RESULT)
    PARAM(success)
    PARAM(tx_failure)
    PARAM(invalid_state)
    PARAM(invalid_signal)
    PARAM(invalid_param)
ENUMS_DONE

////////////////////////////////////////////////////////////////////

SIGNAL(Received_Frame_indication) WITH_DATA
    PARAM(uint16_t channel)
    PARAM(uint16_t rssi)
    PARAM(uint16_t snr)

SIGNAL(Transmit_Frame_request) WITH_DATA
    PARAM(uint8_t channel)
    PARAM(uint16_t power)
    PARAM(uint16_t flags)
    PARAM(uint32_t stay_on_channel)

SIGNAL(Transmit_Done_indication)
    PARAM(RESULT_enum_t result_code)
    PARAM(uint8_t channel)

SIGNAL(Set_Channel_request)
    PARAM(uint8_t channel)
    PARAM(uint32_t listening_time)

SIGNAL(Set_Channel_confirm)
    PARAM(RESULT_enum_t result_code)
    PARAM(uint8_t channel)

SIGNAL(Unset_Channel_request)
    PARAM(uint8_t channel)

SIGNAL(Listening_indication)
    PARAM(RESULT_enum_t result_code)
    PARAM(uint8_t channel)

SIGNAL(Listen_Done_indication)
    PARAM(RESULT_enum_t result_code)
    PARAM(uint8_t channel)

SIGNAL(Listen_Timeout_indication)

////////////////////////////////////////////////////////////////////

SIGNAL(Debug_Print_indication) WITH_DATA
    PARAM(uint32_t timestamp)

SIGNAL(Scan_Start_request)
    PARAM(uint8_t scan_type)
    PARAM(uint32_t channel_mask)

SIGNAL(Scan_Start_confirm)
    PARAM(RESULT_enum_t result_code)
    PARAM(uint16_t scan_id)

SIGNAL(Scan_Result_indication) WITH_DATA
    PARAM(uint16_t scan_id)
    PARAM(uint16_t rssi)
    PARAM(uint16_t channel)

SIGNAL(Scan_Finish_indication) WITH_DATA
    PARAM(uint16_t scan_id)

SIGNALS_DONE

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_MESH_RADIO_RADIO_FSM_SIGNALS_H_ */
