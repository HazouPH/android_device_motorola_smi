#ifndef _BHD_H
#define _BHD_H

#include <stdbool.h>
#include <errno.h>
#include <cutils/log.h>

#ifdef LOG_TAG
	#undef LOG_TAG
#endif
#define LOG_TAG "BHD"

/* Uncomment this line to allow for invalid batteries.  To used in test builds only */
/* #define BHD_ALLOW_INVALID_BATTERY */

#ifndef LOG_BHD_DISABLE
	#undef LOG_NDEBUG
	#define LOG_NDEBUG 0
	#undef LOG_NDDEBUG
	#define LOG_NDDEBUG 0

	/* Uncomment this line to enable logging to file system.  To be used in test
	   builds only, should never have this defined in baseline */
	/* #define BHD_LOG_FS */

	/* Uncomment this line to enable verbose logging.  To be used in test
	   builds only, should never have this defined in baseline */
	/* #define BHD_LOG_VERBOSE */
#endif

#ifdef LOG_BHD_DISABLE
	#define LOG_TRACE(x...) do{} while(0)
	#define LOG_ERROR(x...) do{ LOGE(x); } while(0)
#elif defined BHD_LOG_FS
	extern FILE *bhd_debug_log_fp;

	#define LOG_TRACE(format, args...)  \
		do { \
			if (bhd_debug_log_fp == NULL) \
				LOGD(format, ##args); \
			else { \
				struct timeval tv; \
				gettimeofday(&tv, NULL); \
				fprintf(bhd_debug_log_fp, "[%d.%06d]: "format"\n", \
					(int)tv.tv_sec, (int)tv.tv_usec, ##args); \
				fflush(bhd_debug_log_fp); \
			} \
		} while(0)

	#define LOG_ERROR(format, args...) \
		do { \
			if (bhd_debug_log_fp == NULL) \
				LOGE(format, ##args); \
			else { \
				struct timeval tv; \
				gettimeofday(&tv, NULL);\
				fprintf(bhd_debug_log_fp, "[%d.%06d]: ERROR - "format"\n", \
					(int)tv.tv_sec, (int)tv.tv_usec, ##args); \
				fflush(bhd_debug_log_fp); \
			} \
		} while(0)
#else
	#define LOG_TRACE(x...) do{ LOGD(x); } while(0)
	#define LOG_ERROR(x...) do{ LOGE(x); } while(0)
#endif

#ifdef BHD_LOG_VERBOSE
	#define LOG_TRACE_V(x...) LOG_TRACE(x)
	#define LOG_TRACE_V_DUMP(buff, len) do { BHD_DBG_data_dump(buff, len); } while (0)
#else
	#define LOG_TRACE_V(x...) do{} while(0)
	#define LOG_TRACE_V_DUMP(buff, len) do{} while(0)
#endif


#define BHD_INVALID_VALUE -EINVAL
#define BHD_BATT_UID_SIZE 8

typedef struct {
	char uid_str[(BHD_BATT_UID_SIZE*2) + 1];
	unsigned short capacity;
} bhd_batt_eeprom_t;

typedef struct {
	int real_fcc_batt_temp;
	int real_fcc_mah;
	int soc;
	int ocv_uv;
	int rbatt;
	int charge_increase;
	int chargecycles;
} bhd_bms_params_t;

typedef struct {
	int boc_percent;
	int boc_ocv_uv;
	int boc_cc_uah;
	int eoc_percent;
	int eoc_ocv_uv;
	int eoc_cc_uah;
	int bms_offset;
	time_t aged_timestamp;
} bhd_eoc_params_t;

typedef struct {
	bhd_batt_eeprom_t eeprom;
	bhd_bms_params_t bms;
	bhd_eoc_params_t eoc;
	bool is_factory_mode;
	bool is_charging;
	unsigned long long int file_write_count;
	int file_next_write;
	int last_chg_inc_write;
	int aged_capacity;
} bhd_state_t;

__BEGIN_DECLS
bool BHD_NVM_file_load(bhd_state_t *state);
bool BHD_NVM_file_save(bhd_state_t *state);
bool BHD_SYS_bms_state_write(bhd_state_t *state);
void BHD_SYS_battery_state_change_handle(bhd_state_t *state);
bool BHD_SYS_battery_eeprom_read(bhd_state_t *state);
void BHD_DBG_data_dump(void* databuff, int len);
void BHD_DBG_log_state(bhd_state_t *state);
int BHD_SYS_sys_param_int_get(const char *path, int missing_value);
int BHD_SYS_sys_param_string_get(const char *path, char *s, int size);
void BHD_LOGGER_log_power_supply_details();
void BHD_LOGGER_update_powerup_logger_info();
__END_DECLS

#endif
