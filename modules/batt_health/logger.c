#include "bhd.h"
#include "logger.h"
#include <stdlib.h>
#include <sys/sysinfo.h>

#define AC_PATH "/sys/class/power_supply/ac"
#define BATTERY_PATH "/sys/class/power_supply/battery"
#define USB_PATH "/sys/class/power_supply/usb"
#define ACC_PATH "/sys/class/switch"
#define SYS_BATTERY_PATH "/sys/bus/platform/devices/mmi-battery.0"
#define BATT_PWR_CHARGER_PATH "/sys/devices/platform/msm_ssbi.0/pm8921-core/pm8921-charger"
#define PARAM_PATH "/sys/module/pm8921_bms/parameters"
#define ENABLE_LOG_FILE "/data/power_supply_logger/enable_logging"
#define LOG_FILE "/data/power_supply_logger/power_supply_info.bin"
#define OLD_LOG_FILE "/data/power_supply_logger/old_power_supply_info.bin"
/* The max size of the logger file power_supply_info.bin  */
#define MAX_LOG_FILE_SIZE 3000000
#define ENABLE_LOG_FILE "/data/power_supply_logger/enable_logging"


int rename_file(char *from, char *to)
{
	int err;
	err = rename(from, to);
	if (err < 0)
		return -errno;
	return 0;
}


void logger_file_write(void *generic_info,int length)
{
	FILE *logger_write_fp;
	int status = 0;
	char version = 0x01;
	int log_file_size = 0;

	logger_write_fp = fopen(LOG_FILE, "a");
	if (logger_write_fp == NULL) {
		ALOGE("fopen failure for power_supply_info.bin,errno = %d (%s)",errno,strerror(errno));
	}
	else {
		/* update the log_file_size on every  log write  */
		fseek(logger_write_fp, 0L, SEEK_END);
		log_file_size = ftell(logger_write_fp);
		fseek(logger_write_fp, 0L, SEEK_SET);
		if (log_file_size <= 0 ) {
			/* update file version number ,0x01,0x02,0x03 etc */
			fwrite(&version,sizeof(version),1,logger_write_fp);
			log_file_size += 1;
		}

		fwrite(generic_info,length,1,logger_write_fp);
		log_file_size += length;
		/* check whether file size has exceeded 3MB,if so rename file */
		if (log_file_size > MAX_LOG_FILE_SIZE) {
			fclose(logger_write_fp);
			ALOGD("file rename called");
			status = rename_file(LOG_FILE,OLD_LOG_FILE);
			if (status < 0)
				ALOGE("renaming failed,errno = %d (%s)",errno,strerror(errno));
			log_file_size = 0;
			return;
		}
		fclose(logger_write_fp);
	}
}

void populate_header_info(logger_record_hdr_t *hdr,RECORD_TYPE_T type)
{
	struct sysinfo s_info;
	hdr->type = type;
	gettimeofday(&hdr->timestamp,NULL);

	/* get the uptime */
	if (sysinfo(&s_info) < 0) {
		ALOGE("sysinfo call failure\n");
		hdr->uptime = -1;
	}
	else {
		hdr->uptime = s_info.uptime;
	}
}

void BHD_LOGGER_update_powerup_logger_info()
{
	power_up_t powerup_info;
	struct sysinfo s_info;

#ifdef LOG_PS_ACTIVATE_LOG_ENABLE_CHECK
	/* If file check activated, only log if enable file present */
	if (access(ENABLE_LOG_FILE,F_OK) == -1)
		return;
#endif
	memset(&powerup_info,0,sizeof(powerup_info));
	/* logger_record_hdr updation */
	populate_header_info(&(powerup_info.hdr),POWER_UP_RECORD);
	property_get("ro.build.description",powerup_info.phone_flash_version,"unknown software");
	property_get("ro.bootmode",powerup_info.phone_bootmode,"unknown bootmode");
	powerup_info.battery_valid =
	BHD_SYS_sys_param_int_get(SYS_BATTERY_PATH "/is_valid",-1);
	powerup_info.voltage_max_design =
	BHD_SYS_sys_param_int_get(BATTERY_PATH "/voltage_max_design",-1);
	powerup_info.voltage_min_design =
	BHD_SYS_sys_param_int_get(BATTERY_PATH "/voltage_min_design",-1);
	logger_file_write(&powerup_info,sizeof(powerup_info));
}

/*!
 * @brief Main function of Power Supply Logger.
 *
 * This function is called whenever a uevent is triggered for  /sys/class/power_supply.
 * When we receive uevents related to ac,battery,usb, we update the  power_supply_t structure defined in bhd.h
 * along with the timestamp and then write to the logger file created in /data/power_supply_logger directory.
 */
void BHD_LOGGER_log_power_supply_details()
{
	power_supply_t power_supply_info;
	char temp_buffer[128];
	struct sysinfo s_info;
	FILE *power_supply_info_fp;

#ifdef LOG_PS_ACTIVATE_LOG_ENABLE_CHECK
	/* If file check activated, only log if enable file present */
	if (access(ENABLE_LOG_FILE,F_OK) == -1)
		return;
#endif
	/* logger_record_hdr updation */
	populate_header_info(&(power_supply_info.hdr),POWER_SUPPLY_RECORD);

	/* /power_supply/ac  related events updation */
	power_supply_info.ac_info.ac_online = BHD_SYS_sys_param_int_get(AC_PATH "/online",-1);
	power_supply_info.ac_info.ac_present = BHD_SYS_sys_param_int_get(AC_PATH "/present",-1);

	/* /power_supply/battery  related events updation */
	power_supply_info.battery_info.capacity = BHD_SYS_sys_param_int_get(BATTERY_PATH "/capacity",-1);
	power_supply_info.battery_info.power_supply_present = BHD_SYS_sys_param_int_get(BATTERY_PATH "/present",-1);
	power_supply_info.battery_info.charge_counter = BHD_SYS_sys_param_int_get(BATTERY_PATH "/charge_counter",-1);
	power_supply_info.battery_info.energy_full = BHD_SYS_sys_param_int_get(BATTERY_PATH "/energy_full",-1);
	power_supply_info.battery_info.cycle_count = BHD_SYS_sys_param_int_get(BATTERY_PATH "/cycle_count",-1);
	power_supply_info.battery_info.temp = BHD_SYS_sys_param_int_get(BATTERY_PATH "/temp",-1);
	power_supply_info.battery_info.voltage_now = BHD_SYS_sys_param_int_get(BATTERY_PATH "/voltage_now",-1);
	power_supply_info.battery_info.current_now = BHD_SYS_sys_param_int_get(BATTERY_PATH "/current_now",-1);

	if (BHD_SYS_sys_param_string_get(BATTERY_PATH "/status", temp_buffer, sizeof(temp_buffer)) > 0) {
		if (strncmp(temp_buffer,"Not charging",12) == 0)
			power_supply_info.battery_info.power_supply_status = 1;
		else if (strncmp(temp_buffer,"Charging",8) == 0)
			power_supply_info.battery_info.power_supply_status = 2;
		else if (strncmp(temp_buffer,"Full",4) == 0)
			power_supply_info.battery_info.power_supply_status = 3;
		else if (strncmp(temp_buffer,"Discharging",11) == 0)
			power_supply_info.battery_info.power_supply_status = 4;
		else
			power_supply_info.battery_info.power_supply_status = 0;
	}
	else
		power_supply_info.battery_info.power_supply_status = -1;

	if (BHD_SYS_sys_param_string_get(BATTERY_PATH "/health", temp_buffer, sizeof(temp_buffer)) > 0) {
		if (strncmp(temp_buffer,"Good",4) == 0)
			power_supply_info.battery_info.power_supply_health = 1;
		else
			power_supply_info.battery_info.power_supply_health = 0;
	}
	else
		power_supply_info.battery_info.power_supply_health = -1;

	if (BHD_SYS_sys_param_string_get(BATTERY_PATH "/charge_type", temp_buffer, sizeof(temp_buffer)) > 0) {
		if (strncmp(temp_buffer,"Fast",4) == 0)
			power_supply_info.battery_info.power_supply_charge_type = 1;
		else
			power_supply_info.battery_info.power_supply_charge_type = 0;
	}
	else
		power_supply_info.battery_info.power_supply_charge_type = -1;

	/* /power_supply/usb  related events updation */
	power_supply_info.usb_info.usb_online = BHD_SYS_sys_param_int_get(USB_PATH "/online",-1);
	power_supply_info.usb_info.usb_present = BHD_SYS_sys_param_int_get(USB_PATH "/present",-1);

	/* /sys/class/switch/ accessory related events updation */
	power_supply_info.acc_info.extdock_state = BHD_SYS_sys_param_int_get(ACC_PATH "/extdock/state",-1);
	power_supply_info.acc_info.semu_audio_state = BHD_SYS_sys_param_int_get(ACC_PATH "/semu_audio/state",-1);
	power_supply_info.acc_info.smartdock_state = BHD_SYS_sys_param_int_get(ACC_PATH "/smartdock/state",-1);

	power_supply_info.fsm_state = BHD_SYS_sys_param_int_get(BATT_PWR_CHARGER_PATH "/fsm_state",-1);
	power_supply_info.max_charge_rate = BHD_SYS_sys_param_int_get(BATT_PWR_CHARGER_PATH "/charge_rate",-1);
	power_supply_info.last_ocv_uv = BHD_SYS_sys_param_int_get(PARAM_PATH "/last_ocv_uv",-1);

	/* update logger file with the power_supply_info structure */
	logger_file_write(&power_supply_info,sizeof(power_supply_info));
}

