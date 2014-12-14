#include <cutils/properties.h>

#define BOOT_MODE_LENGTH      30  // Length in bytes.

typedef enum {
	POWER_UP_RECORD = 1,
	POWER_SUPPLY_RECORD
}RECORD_TYPE_T;

/* The logger structure  */
typedef struct {
    char type;
    struct timeval timestamp;
    long int uptime;
}__attribute__ ((packed)) logger_record_hdr_t;

typedef struct {
    char ac_online;
    char ac_present;
} __attribute__ ((packed)) ac_info_t;

typedef struct {
    char capacity;
    char power_supply_present;
    int charge_counter;
    int energy_full;
    int cycle_count;
    int temp;
    int voltage_now;
    int current_now;
    char power_supply_status; //char to represent strings Charging,Discharging etc
    char power_supply_health; //char to represent strings Good,Bad etc
    char power_supply_charge_type; //char to represent strings Fast,Unknown etc
}__attribute__ ((packed)) battery_info_t;

typedef struct {
    char usb_online;
    char usb_present;
}__attribute__ ((packed)) usb_info_t;

typedef struct {
    char extdock_state;
    char semu_audio_state;
    char smartdock_state;
}__attribute__ ((packed)) acc_status_t;

typedef struct {
    logger_record_hdr_t hdr;
    ac_info_t ac_info;
    battery_info_t battery_info;
    usb_info_t usb_info;
    acc_status_t acc_info;
    char fsm_state;
    char max_charge_rate;
    int last_ocv_uv;
}__attribute__ ((packed)) power_supply_t;

typedef struct
{
    logger_record_hdr_t hdr;
    char phone_flash_version[PROPERTY_VALUE_MAX];
    char phone_bootmode[PROPERTY_VALUE_MAX];
    char battery_valid;
    int voltage_max_design;
    int voltage_min_design;
} __attribute__ ((packed))  power_up_t;

