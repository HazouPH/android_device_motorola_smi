#include "bhd.h"

#include <private/android_filesystem_config.h>
#include <linux/prctl.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <cutils/properties.h>

#ifdef BHD_LOG_FS
FILE *bhd_debug_log_fp;
#endif

/* Init fd for uevents */
static int main_batt_uevent_init(void)
{
    struct sockaddr_nl addr;
    int sz = 64 * 1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(s < 0) {
	    ALOGE("Failed to open uevent socket, errno = %d (%s)",
		    errno, strerror(errno));
	    return -1;
    }

    if (setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0) {
	    ALOGE("Failed to set socket option, errno = %d (%s)",
		    errno, strerror(errno));
	    close(s);
	    return -1;
    }

    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	    ALOGE("Failed to bind socket, errno = %d (%s)",
		    errno, strerror(errno));
	    close(s);
	    return -1;
    }

    ALOGD("Created socket %d for uevents", s);
    return s;
}

/* Handle a uevent,log uevents to /data/power_supply_logger/power_supply_info.bin file */
static void main_uevent_handler(int uevent_fd, bhd_state_t *state)
{
	int status;
	char msg[1024];

	status = recv(uevent_fd, msg, sizeof(msg), MSG_DONTWAIT);
	if (status == -1) {
		ALOGE("uevent recv error, fd = %d, errno = %d (%s)",
			uevent_fd, errno, strerror(errno));
	} else {
		msg[(sizeof(msg) - 1)] = '\0';
		ALOGV("Uevent = %s", msg);
		if (strcasestr(msg, "change@") &&
			strcasestr(msg, "power_supply/battery")) {
			ALOGD("Got a battery uevent,log details");
			BHD_SYS_battery_state_change_handle(state);
			BHD_LOGGER_log_power_supply_details();
		} else if (strcasestr(msg, "change@") &&
			strcasestr(msg, "power_supply/usb")) {
			ALOGD("Got a USB uevent, log details");
			BHD_LOGGER_log_power_supply_details();
		} else if (strcasestr(msg, "change@") &&
			strcasestr(msg, "power_supply/ac")) {
			ALOGD("Got a AC uevent, log details");
			BHD_LOGGER_log_power_supply_details();
		}
	}
}

/* Init the state variable */
static void main_state_init(bhd_state_t *state)
{
	memset(state, 0, sizeof(*state));
	state->bms.real_fcc_batt_temp = BHD_INVALID_VALUE;
	state->bms.real_fcc_mah = BHD_INVALID_VALUE;
	state->bms.soc = BHD_INVALID_VALUE;
	state->bms.ocv_uv = BHD_INVALID_VALUE;
	state->bms.rbatt = BHD_INVALID_VALUE;
	state->bms.charge_increase = BHD_INVALID_VALUE;
	state->bms.chargecycles = BHD_INVALID_VALUE;
	state->aged_capacity = 0;
}

/* Determine if we are in factory mode or not */
static void main_factory_mode_check(bhd_state_t *state)
{
	char value[PROPERTY_VALUE_MAX] = {0};
	const char unknown[] = "unknown";
	const char factory[] = "factory";
	int r;

	state->is_factory_mode = false;
	r = property_get("ro.bootmode", value, unknown);
	if ( (r <= 0) || (strncmp(value, unknown, sizeof(unknown)) == 0) )
		ALOGE("Failed to read factory mode property");
	else if (strncmp(value, factory, sizeof(factory)) == 0) {
		ALOGD("Phone is in factory mode!");
		state->is_factory_mode = true;
	}
}

/* Wait for an event to occur */
static void main_event_listener(int uevent_fd, bhd_state_t *state)
{
	fd_set events_fds;
	fd_set working_events_fds;
	int select_val;

	FD_ZERO(&events_fds);
	FD_SET(uevent_fd, &events_fds);
	working_events_fds = events_fds;

	while (1) {
		working_events_fds = events_fds;
		ALOGV("Waiting for event...");
		select_val = select(uevent_fd + 1, &working_events_fds, NULL,
				NULL, NULL);

		if (select_val == -1)
			ALOGE("select failure, errno = %d (%s)", errno,
				strerror(errno));
		else if (select_val > 0)
			if (FD_ISSET(uevent_fd, &working_events_fds))
				main_uevent_handler(uevent_fd, state);
	}
}

/* Switch from root to mot_pwric user.  Need to start as root so we can keep
   CAP_NET_ADMIN capability. */
static bool main_switch_user(void)
{
	bool is_success = false;
	struct __user_cap_header_struct header;
	struct __user_cap_data_struct   cap;

	if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) < 0)
		ALOGE("Failed to keep caps, errno = %d (%s)",
			errno, strerror(errno));
	else {
		if (setuid(AID_MOT_PWRIC) < 0)
			ALOGE("Failed to set uid, errno = %d (%s)",
				errno, strerror(errno));
		else {
			header.version = _LINUX_CAPABILITY_VERSION;
			header.pid = 0;
			cap.effective = cap.permitted = (1 << CAP_NET_ADMIN);
			cap.inheritable = 0;
			if (capset(&header, &cap) < 0)
				ALOGE("Failed to set caps, errno = %d (%s)",
					errno, strerror(errno));
			else
				is_success = true;
		}
	}

	return is_success;
}

/* Stops the batt health daemon from running.  Should only be used in non-recoverable
   failures. Due to the fact that daemon will atuo restart if it exits, just loop forever */
static void main_stop_app(const char *msg)
{
	while (1) {
		ALOGE("Batt health app stopped due to: %s", msg);
		sleep(14400);
	}
}

int main(int argc, char* argv[])
{
	bhd_state_t state;
	int uevent_fd;

#ifdef BHD_LOG_FS
	/* Start file system logging if enabled */
	bhd_debug_log_fp = fopen("/data/batt_health.log", "a");

	/* If log_fp is not valid, log macros default to Android log
	   utils, so ok to use macros here in fail case */
	if (bhd_debug_log_fp == NULL)
		ALOGE("Failed to open logfile");
	else
		ALOGD("--------------------------------------------------");
#endif

	ALOGD("Battery Health Daemon start!");

	if (main_switch_user() == false) {
		main_stop_app("Failed to change user, stop daemon...");
	}

	BHD_LOGGER_update_powerup_logger_info();
	main_state_init(&state);

	/* Determine if we are in factory mode */
	main_factory_mode_check(&state);

	/* Load battery eeprom, exit if there is a failure */
	if (BHD_SYS_battery_eeprom_read(&state) == false) {
		/* TODO: Need to support removable batteries at some point.
		   Need to consider fact that phone is booted with no battery,
		   but has charger. */
		main_stop_app("Failed to validate battery, stop daemon...");
	}

	/* Load persistent data from NVM & update BMS with values */
	if (BHD_NVM_file_load(&state) == false)
		ALOGE("No valid NVM files found");
	else {
		if (BHD_SYS_bms_state_write(&state) == false) {
			/* Since we failed to update kernel with loaded NVM data,
			   we'd lose that data on future kernel state changes,
			   so exit the daemon to stop collecting data */
			main_stop_app("Failed to update kernel with loaded data, stop daemon...");
		}
	}

	BHD_DBG_log_state(&state);

	/* Act as if a state change has occurred.  Needed to be sure we update
	   NVM if a drastic change has happened since previous write */
	BHD_SYS_battery_state_change_handle(&state);

	/* Register to get uevents */
	if ( (uevent_fd = main_batt_uevent_init()) == -1 ) {
		main_stop_app("Failed to listen for uevents, stop daemon...");
	}

	/* Listen for events */
	main_event_listener(uevent_fd, &state);

	/* TODO: Currently never will reach here. Need to add graceful
	   shutdown */
	close (uevent_fd);
	ALOGD("Battery Health Daemon exit!");

	exit(EXIT_SUCCESS);
}
