#include "bhd.h"

#include <string.h>
#include <stdio.h>

/* Debug function to dump a binary buffer */
void BHD_DBG_data_dump(void* databuff, int len)
{
	const int max_col = 16;
	int row = 0;
	int col = 0;
	int num_col;
	int buffer_index = 0;
	unsigned char value;

	/* Each column takes up 3 characters, plus NULL */
	char string_buffer[(max_col * 3) + 1];

        for (row = 0; row <= (len / max_col); row++) {
		/* Reset string buffer for each new row */
		memset(string_buffer, 0, sizeof(string_buffer));

		/* For all rows, the number of columns is the max number, except for the last row */
		num_col = (row == (len / max_col)) ? (len % max_col) : max_col;
		if (num_col != 0) {
			for (col = 0; col < num_col; col++) {
				value = ((unsigned char*)databuff)[buffer_index++];
				sprintf(string_buffer, "%s%02x ", string_buffer, value);
			}
			ALOGD_V("%s", string_buffer);
		}
        }
}

/* Debug function to print BHD state */
void BHD_DBG_log_state(bhd_state_t *state)
{
	ALOGD("State - EEPROM id = %s; capacity = %d",
		state->eeprom.uid_str, state->eeprom.capacity);
	ALOGD("State - BMS real fcc batt temp = %d, real fcc = %d",
		state->bms.real_fcc_batt_temp, state->bms.real_fcc_mah);
	ALOGD("State - BMS SOC = %d%%, OCV = %d, rbatt = %d",
		state->bms.soc, state->bms.ocv_uv, state->bms.rbatt);
	ALOGD("State - BMS charge inc = %d, charge cycles = %d",
		state->bms.charge_increase, state->bms.chargecycles);
	ALOGD("State - is factory mode = %d",
		state->is_factory_mode);
	ALOGD("State - is charging = %d",
		state->is_charging);
	ALOGD("State - aged capacity %% = %d", state->aged_capacity);
	if (state->aged_capacity != 0) {
		ALOGD("State - aged capacity calculated @ %s",
			ctime(&state->eoc.aged_timestamp));
		ALOGD("State - Begin charge %% = %d, cc = %d, ocv = %d",
			state->eoc.boc_percent, state->eoc.boc_cc_uah, state->eoc.boc_ocv_uv);
		ALOGD("State - End charge %% = %d, cc = %d, ocv = %d",
			state->eoc.eoc_percent, state->eoc.eoc_cc_uah, state->eoc.eoc_ocv_uv);
	}
}
