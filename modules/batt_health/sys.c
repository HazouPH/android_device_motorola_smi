#include "bhd.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

/* Battery EEPROM paths to read from */
#define SYS_EEPROM_PATH_ROOT "/sys/bus/platform/devices/mmi-battery.0/"
#define SYS_EEPROM_PATH_IS_VALID  SYS_EEPROM_PATH_ROOT"is_valid"
#define SYS_EEPROM_PATH_UID       SYS_EEPROM_PATH_ROOT"uid"
#define SYS_EEPROM_PATH_CAPACITY  SYS_EEPROM_PATH_ROOT"capacity"

/* BMS Parameters to read/write */
#define SYS_BMS_PARAM_ROOT "/sys/module/pm8921_bms/parameters/"
#define SYS_BMS_PARAM_REAL_FCC_BATT_TEMP SYS_BMS_PARAM_ROOT"last_real_fcc_batt_temp"
#define SYS_BMS_PARAM_REAL_FCC           SYS_BMS_PARAM_ROOT"last_real_fcc_mah"
#define SYS_BMS_PARAM_SOC                SYS_BMS_PARAM_ROOT"last_soc"
#define SYS_BMS_PARAM_OCV_UV             SYS_BMS_PARAM_ROOT"last_ocv_uv"
#define SYS_BMS_PARAM_RBATT              SYS_BMS_PARAM_ROOT"last_rbatt"
#define SYS_BMS_PARAM_CHG_INC            SYS_BMS_PARAM_ROOT"last_charge_increase"
#define SYS_BMS_PARAM_CHG_CYC            SYS_BMS_PARAM_ROOT"last_chargecycles"

#define SYS_BMS_PARAM_BOC_PERCENT        SYS_BMS_PARAM_ROOT"bms_start_percent"
#define SYS_BMS_PARAM_BMS_OFFSET         SYS_BMS_PARAM_ROOT"bms_meter_offset"
#define SYS_BMS_PARAM_BOC_OCV_UV         SYS_BMS_PARAM_ROOT"bms_start_ocv_uv"
#define SYS_BMS_PARAM_BOC_CC_UAH         SYS_BMS_PARAM_ROOT"bms_start_cc_uah"
#define SYS_BMS_PARAM_EOC_PERCENT        SYS_BMS_PARAM_ROOT"bms_end_percent"
#define SYS_BMS_PARAM_EOC_OCV_UV         SYS_BMS_PARAM_ROOT"bms_end_ocv_uv"
#define SYS_BMS_PARAM_EOC_CC_UAH         SYS_BMS_PARAM_ROOT"bms_end_cc_uah"

#define SYS_BMS_PARAM_AGED_CAPACITY      SYS_BMS_PARAM_ROOT"bms_aged_capacity"
#define SYS_BMS_PARAM_AGED_TIMESTAMP      SYS_BMS_PARAM_ROOT"timestamp"

struct bhd_sys_param_desc {
	size_t offset;
	const char path[70];
};

/* Associate BMS sysfs entries with state variable members.  Assumes all state
   variable members are of type int */
const struct bhd_sys_param_desc bms_param_table[] = {
	{offsetof(bhd_state_t, bms.real_fcc_batt_temp), SYS_BMS_PARAM_REAL_FCC_BATT_TEMP},
	{offsetof(bhd_state_t, bms.real_fcc_mah), SYS_BMS_PARAM_REAL_FCC},
	{offsetof(bhd_state_t, bms.soc), SYS_BMS_PARAM_SOC},
	{offsetof(bhd_state_t, bms.ocv_uv),  SYS_BMS_PARAM_OCV_UV},
	{offsetof(bhd_state_t, bms.rbatt), SYS_BMS_PARAM_RBATT},
	{offsetof(bhd_state_t, bms.charge_increase), SYS_BMS_PARAM_CHG_INC},
	{offsetof(bhd_state_t, bms.chargecycles), SYS_BMS_PARAM_CHG_CYC}
};

const struct bhd_sys_param_desc eoc_param_table[] = {
	{offsetof(bhd_eoc_params_t, boc_percent), SYS_BMS_PARAM_BOC_PERCENT},
	{offsetof(bhd_eoc_params_t, boc_ocv_uv),  SYS_BMS_PARAM_BOC_OCV_UV},
	{offsetof(bhd_eoc_params_t, boc_cc_uah),  SYS_BMS_PARAM_BOC_CC_UAH},
	{offsetof(bhd_eoc_params_t, eoc_percent), SYS_BMS_PARAM_EOC_PERCENT},
	{offsetof(bhd_eoc_params_t, eoc_ocv_uv),  SYS_BMS_PARAM_EOC_OCV_UV},
	{offsetof(bhd_eoc_params_t, eoc_cc_uah),  SYS_BMS_PARAM_EOC_CC_UAH},
	{offsetof(bhd_eoc_params_t, bms_offset),  SYS_BMS_PARAM_BMS_OFFSET},
};

/* Reads an integer value from a sysfs file */
int BHD_SYS_sys_param_int_get(const char *path, int missing_value)
{
	int value;
        char s[1024];
        ssize_t r;
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
		ALOGE("Failed to open %s for reading, errno = %d (%s)",
			path, errno, strerror(errno));
                return missing_value;
	}
        r = read(fd, s, sizeof(s) - 1);
        close(fd);
        if (r < 0) {
		ALOGE("Failed to read file %s, r = %lu, errno = %d (%s)",
			path, r, errno, strerror(errno));
                return missing_value;
	}
        s[(int) r] = '\0';
	value = atoi(s);

	ALOGV("Get %s = %d", path, value);
        return value;
}

/* Writes an integer value to a sysfs file */
static bool sys_param_int_set(const char *path, int value)
{
	bool is_success = false;
	ssize_t r;
	char buffer[32];
	int fd = open(path, O_WRONLY);
	if (fd < 0)
		ALOGE("Failed to open %s for writing %d, errno = %d (%s)",
			path, value, errno, strerror(errno));
	else {
		snprintf(buffer, sizeof(buffer) - 1, "%d", value);
		buffer[sizeof(buffer) - 1] = '\0';

		r = write(fd, buffer, strlen(buffer));
		if (r != (ssize_t) strlen(buffer))
			ALOGE("Failed to write %s to file %s, r = %lu, errno = %d (%s)",
				buffer, path, r, errno, strerror(errno));
		else {
			is_success = true;
			ALOGV("Set %s = %s", path, buffer);
		}
		close(fd);
	}

	return(is_success);
}

/* Writes an integer value to a sysfs file with retries on fail */
static bool sys_param_int_set_retry(const char *path, int value, int max_retry,
				unsigned long usec)
{
	bool is_success = false;
	int i;

	for (i = 0; ((i < max_retry) && (is_success == false)); i++) {
		is_success = sys_param_int_set(path, value);
		if (is_success == false)
			usleep(usec);
	}

	return is_success;
}

/* Gets a string value to a sysfs file */
int BHD_SYS_sys_param_string_get(const char *path, char *s, int size)
{
        int r;
        int fd = open(path, O_RDONLY);
        s[0] = 0;
        if (fd < 0) {
		ALOGE("Failed to open %s for reading, errno = %d (%s)",
			path, errno, strerror(errno));
                return -1;
	}
        r = read(fd, s, size - 1);
        close(fd);
        if (r >= 0) {
                s[r] = '\0';
		ALOGV("Get %s = %s", path, s);
	} else {
		ALOGE("Failed to read file %s, errno = %d (%s)",
			path, errno, strerror(errno));
	}

	return r;
}

/* Determine if battery is currently full, -1 = error */
static int sys_is_battery_full()
{
        char status[128];
	if (BHD_SYS_sys_param_string_get("/sys/class/power_supply/battery/status",
					status, sizeof(status)) < 0)
                return -1;

        return (strncmp(status, "Full", 4) == 0)? 1 : 0;
}

/* Determine if battery is currently charging, -1 = error */
static int sys_is_battery_charging()
{
        char status[128];
        if (BHD_SYS_sys_param_string_get("/sys/class/power_supply/battery/status",
					status, sizeof(status)) < 0)
                return -1;

        return (strncmp(status, "Charging", 8) == 0)? 1 : 0;
}

/* Get the battery charge level */
static int sys_battery_charge_level_get()
{
        return BHD_SYS_sys_param_int_get("/sys/class/power_supply/battery/capacity", -1);
}

/* Read all of the BMS sysfs entries */
static int sys_bms_param_read(bhd_state_t *battery_state)
{
	int i;
	int value;
	char *bhd_state = (char *) battery_state;
	int array_size = sizeof(bms_param_table)/sizeof(bms_param_table[0]);

	for (i = 0; i < array_size; i++)
	{
		/* TODO: There anything better to do if getting value failed? */
		value = BHD_SYS_sys_param_int_get(bms_param_table[i].path, BHD_INVALID_VALUE);
		*((int*)(bhd_state + bms_param_table[i].offset)) = value;
	}

	return 0;
}

/* Read all of the end of charge sysfs entries */
static bool sys_eoc_param_read(bhd_eoc_params_t *eoc)
{
	int i;
	int value;
	char *eoc_val = (char *) eoc;
	int array_size = sizeof(eoc_param_table)/sizeof(eoc_param_table[0]);
	bool is_success = true;

	for (i = 0; i < array_size; i++)
	{
		value = BHD_SYS_sys_param_int_get(eoc_param_table[i].path, BHD_INVALID_VALUE);
		*((int*)(eoc_val + eoc_param_table[i].offset)) = value;
		if (value == BHD_INVALID_VALUE)
			is_success = false;
	}

	return is_success;
}

/* Determines if sysfs values have changed enough to warrant and NVM write. Should
   make best effort to limit the number of NVM writes */
static bool sys_is_battery_nvm_write_needed(bhd_state_t *old_state, bhd_state_t *cur_state)
{
	bool is_write_needed = false;

	if (cur_state->bms.chargecycles > old_state->bms.chargecycles) {
		is_write_needed = true;
		ALOGD("Write needed - Charge cycle increase, current = %d, old = %d",
			cur_state->bms.chargecycles, old_state->bms.chargecycles);
	}

	/* Compare current charge increase against the last previously written value.
	   Need to do this to account for multiple smaller increases */
	if (cur_state->bms.charge_increase >= (old_state->last_chg_inc_write + 50)) {
		is_write_needed = true;
		ALOGD("Write needed - Charge %% increase, current = %d, old = %d",
			cur_state->bms.charge_increase, old_state->last_chg_inc_write);
	}

	if (cur_state->aged_capacity != old_state->aged_capacity) {
		is_write_needed = true;
		ALOGD("Write needed - Aged capacity chage, current = %d, old = %d",
			cur_state->aged_capacity, old_state->aged_capacity);
	}

	return is_write_needed;
}

static bool sys_bms_aged_timestamp_write(bhd_state_t *state, int max_retry,
                                        unsigned long usec)
{
        return sys_param_int_set_retry(SYS_BMS_PARAM_AGED_TIMESTAMP, state->eoc.aged_timestamp,
                                max_retry, usec);
}

static bool sys_bms_aged_capacity_write(bhd_state_t *state, int max_retry,
					unsigned long usec)
{
	return sys_param_int_set_retry(SYS_BMS_PARAM_AGED_CAPACITY, state->aged_capacity,
				max_retry, usec);
}

/* Read sysfs to determine battery state */
static void sys_battery_state_read(bhd_state_t *state)
{
	int is_charging;
	int chrg_lvl;
	int percent_delta;
	float cc_delta;
	float capacity_mah;
	float capacity_percent_float;
	int capacity_percent_int;

	sys_bms_param_read(state);

	is_charging = sys_is_battery_charging();
	if (is_charging == 1) {
		if (!state->is_charging) {
			chrg_lvl = sys_battery_charge_level_get();
			ALOGD("Battery start charging @ %d%%!", chrg_lvl);
			state->is_charging = true;
		}
	} else if (is_charging == 0) {
		if (state->is_charging) {
			bool eoc_ok;
			bhd_eoc_params_t eoc_tmp;

			chrg_lvl = sys_battery_charge_level_get();
			ALOGD("Battery stop charging @ %d%%", chrg_lvl);
			/* Read the end of charge parameters */
			eoc_ok = sys_eoc_param_read(&eoc_tmp);
			if ( (eoc_ok == true) &&
				(eoc_tmp.boc_percent >= 0) &&
				(eoc_tmp.boc_percent <= 5) &&
				(sys_is_battery_full() == 1) &&
				(eoc_tmp.eoc_percent == 100) ) {
				ALOGD("Criteria met to calculate new aged capacity!");
				memcpy(&state->eoc, &eoc_tmp, sizeof(eoc_tmp));

				percent_delta = 100 - (100 - state->eoc.eoc_percent) -
					(state->eoc.boc_percent + state->eoc.bms_offset);
				if (percent_delta < 1)
					percent_delta = 1;
				cc_delta = (float) (abs(state->eoc.eoc_cc_uah - state->eoc.boc_cc_uah)
						/ 1000);
				capacity_mah = (cc_delta / percent_delta) * 100;
				capacity_percent_float = (capacity_mah / state->eeprom.capacity)
					* 100;
				capacity_percent_float += 2.5;
				capacity_percent_int = (int) capacity_percent_float;
				if (capacity_percent_int > 120)
					capacity_percent_int = 120;
				else if (capacity_percent_int < 1)
					capacity_percent_int = 1;
				state->aged_capacity = capacity_percent_int;
				state->eoc.aged_timestamp = time(NULL);

				ALOGD("Start %% = %d, start cc = %d, end %% = %d, end cc = %d, offset = %d",
					state->eoc.boc_percent, state->eoc.boc_cc_uah,
					state->eoc.eoc_percent, state->eoc.eoc_cc_uah,
					state->eoc.bms_offset);
				ALOGD("Percent delta = %d, cc delta = %f, capacity mah = %f",
					percent_delta, cc_delta, capacity_mah);
				ALOGD("New aged capacity = %d%%, calculated @ %s",
					state->aged_capacity, ctime(&state->eoc.aged_timestamp));
				sys_bms_aged_capacity_write(state, 1, 0);
				sys_bms_aged_timestamp_write(state, 1, 0);
			}

			state->is_charging = false;
		}
	}

}

/* Update BMS sysfs entries with info from battery health daemon */
bool BHD_SYS_bms_state_write(bhd_state_t *state)
{
	bool is_success = true;
	int i;
	int value;
	char *bhd_state = (char *) state;
	int array_size = sizeof(bms_param_table)/sizeof(bms_param_table[0]);

	for (i = 0; i < array_size; i++)
	{
		value = *((int*)(bhd_state + bms_param_table[i].offset));
		if (value == BHD_INVALID_VALUE)
			ALOGE("Skipping %s", bms_param_table[i].path);
		else {
			is_success = sys_param_int_set_retry(bms_param_table[i].path,
							value, 5, 250000);
			if (is_success == false)
				return is_success;
		}
	}


	is_success = sys_bms_aged_capacity_write(state, 5, 250000);
	if (is_success)
		is_success = sys_bms_aged_timestamp_write(state, 5, 250000);

	return is_success;
}

/* Handle a battery state change */
void BHD_SYS_battery_state_change_handle(bhd_state_t *state)
{
	bhd_state_t old_state;

	/* Save the old state information for the battery */
	memcpy(&old_state, state, sizeof(*state));

	/* Get the new state */
	sys_battery_state_read(state);

	/* Determine if NVM file write is needed */
	if (sys_is_battery_nvm_write_needed(&old_state, state)) {
		BHD_DBG_log_state(state);
		BHD_NVM_file_save(state);
	}
}

/* Read battery eeprom info */
bool BHD_SYS_battery_eeprom_read(bhd_state_t *state)
{
	bool is_success = false;
#ifdef BHD_ALLOW_INVALID_BATTERY
	const int max_retries = 1;
#else
	const int max_retries = 20;
#endif
	int retries;
	int cap;

	for (retries = 0; retries < max_retries; retries++) {

		if (BHD_SYS_sys_param_int_get(SYS_EEPROM_PATH_IS_VALID, BHD_INVALID_VALUE) != 1)
			ALOGE("Battery not valid");
		else if ( (cap = BHD_SYS_sys_param_int_get(SYS_EEPROM_PATH_CAPACITY, BHD_INVALID_VALUE))
			== BHD_INVALID_VALUE)
			ALOGE("Battery capacity not valid");
		else if (BHD_SYS_sys_param_string_get(SYS_EEPROM_PATH_UID, state->eeprom.uid_str,
						sizeof(state->eeprom.uid_str)) !=
			(sizeof(state->eeprom.uid_str) - 1))
			ALOGE("Battery UID not valid");
		else {
			is_success = true;
			state->eeprom.capacity = cap;
			ALOGD("EEPROM - Battery ID = %s", state->eeprom.uid_str);
			break;
		}

		if ((retries + 1) < max_retries)
			usleep(750000);
		else
			ALOGE("Failed to validate battery after %d attempts",
				retries + 1);
	}
#ifdef BHD_ALLOW_INVALID_BATTERY
	if (!is_success) {
		ALOGD("Allowing invalid battery");
		strlcpy(state->eeprom.uid_str, "0123456789abcdef", sizeof(state->eeprom.uid_str));
		state->eeprom.capacity = 1750;
		is_success = true;
	}
#endif
	return is_success;
}
