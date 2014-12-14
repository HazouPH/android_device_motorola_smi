#include "bhd.h"

#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NVM_NUM_FILES 2 /* Number of NVM files per battery id to store batt data */
#define NVM_HDR_TEXT "BHD"
#define NVM_PATH_ROOT "/pds/batt_health/batt-" /* Path to store NVM files to */

#define NVM_FILE_HEADER_VERSION_0 0
#define NVM_FILE_HEADER_VERSION_1 1

/* NVM file header, constant for all file versions */
typedef struct __attribute__ ((__packed__)) {
	char text[4];
	int version;
	unsigned long long int file_write_count;
	time_t timestamp;
} nvm_file_hdr_t;

/* NVM file data, version 0 */
typedef struct __attribute__ ((__packed__)) {
	int real_fcc_batt_temp;
	int real_fcc_mah;
	int soc;
	int ocv_uv;
	int rbatt;
	int charge_increase;
	int chargecycles;
	int aged_capacity;
	int boc_percent;
	int boc_ocv_uv;
	int boc_cc_uah;
	int eoc_percent;
	int eoc_ocv_uv;
	int eoc_cc_uah;
	time_t aged_timestamp;
} nvm_file_data_ver_0_t;

typedef struct {
	size_t offset;
	int low_limit;
	int high_limit;
	int def_val;
	const char name[20];
} nvm_file_limit_t;

/* Read files using format version 0 */
static bool nvm_file_ver_0_read(int fd, bhd_state_t *state)
{
	bool is_success = false;
	nvm_file_data_ver_0_t data;
	ssize_t r;

	r = read(fd, &data, sizeof(data));
	if (r != sizeof(data))
		ALOGE("NVM Read - Failed to read data, r = %lu, errno = %d (%s)",
			r, errno, strerror(errno));
	else {
		state->bms.real_fcc_batt_temp = data.real_fcc_batt_temp;
		state->bms.real_fcc_mah = data.real_fcc_mah;
		state->bms.soc = data.soc;
		state->bms.ocv_uv = data.ocv_uv;
		state->bms.rbatt = data.rbatt;
		state->bms.charge_increase = data.charge_increase;
		state->bms.chargecycles = data.chargecycles;
		state->aged_capacity = data.aged_capacity;

		/* begin-of-charge and end-of-charge variables just stored for debugging */
		state->eoc.aged_timestamp = data.aged_timestamp;
		state->eoc.boc_percent = data.boc_percent;
		state->eoc.boc_ocv_uv = data.boc_ocv_uv;
		state->eoc.boc_cc_uah = data.boc_cc_uah;
		state->eoc.eoc_percent = data.eoc_percent;
		state->eoc.eoc_ocv_uv = data.eoc_ocv_uv;
		state->eoc.eoc_cc_uah = data.eoc_cc_uah;
		is_success = true;
	}

	return (is_success);
}

/* Reads battery health file from NVM */
static bool nvm_file_read(int fd, bhd_state_t *state)
{
	bool is_success = false;
	ssize_t r;
	nvm_file_hdr_t hdr;

	/* Read the standard header first */
	r = read(fd, &hdr, sizeof(hdr));
	if (r != sizeof(hdr))
		ALOGE("NVM Read - Failed to read file header, r = %lu, errno = %d (%s)",
			r, errno, strerror(errno));
	else if (strncmp(hdr.text, NVM_HDR_TEXT, sizeof(hdr.text)))
		ALOGE("NVM Read - Invalid header");
	else {
		ALOGD("NVM Read - Read header, version = %d, file write count = %llu, "
			"time = %s", hdr.version, hdr.file_write_count,
			ctime(&hdr.timestamp));

		/* Read rest of file depending on format version */
		switch (hdr.version) {
		case NVM_FILE_HEADER_VERSION_1:
			is_success = nvm_file_ver_0_read(fd, state);
			break;

		default:
			ALOGE("NVM Read - Unknown version %d", hdr.version);
			break;
		}

		if (is_success)
			state->file_write_count = hdr.file_write_count;
	}

	return is_success;
}

/* Writes battery health file to NVM */
static bool nvm_file_write(int fd, bhd_state_t *state)
{
	bool is_success = false;
	ssize_t r;
	nvm_file_hdr_t hdr;
	nvm_file_data_ver_0_t data;

	/* Populate header, always use latest version */
	hdr.version = NVM_FILE_HEADER_VERSION_1;
	hdr.timestamp = time(NULL);
	hdr.file_write_count = state->file_write_count;
	strcpy(hdr.text, NVM_HDR_TEXT);

	/* Populate data section */
	data.real_fcc_batt_temp = state->bms.real_fcc_batt_temp;
	data.real_fcc_mah = state->bms.real_fcc_mah;
	data.soc = state->bms.soc;
	data.ocv_uv = state->bms.ocv_uv;
	data.rbatt = state->bms.rbatt;
	data.charge_increase = state->bms.charge_increase;
	data.chargecycles = state->bms.chargecycles;
	data.aged_capacity = state->aged_capacity;
	data.aged_timestamp = state->eoc.aged_timestamp;
	data.boc_percent = state->eoc.boc_percent;
	data.boc_ocv_uv = state->eoc.boc_ocv_uv;
	data.boc_cc_uah = state->eoc.boc_cc_uah;
	data.eoc_percent = state->eoc.eoc_percent;
	data.eoc_ocv_uv = state->eoc.eoc_ocv_uv;
	data.eoc_cc_uah = state->eoc.eoc_cc_uah;

	r = write(fd, &hdr, sizeof(hdr));
	if (r != sizeof(hdr))
		ALOGE("NVM Write - Failed to file header, r = %lu, errno = %d (%s)",
			r, errno, strerror(errno));
	else {
		r = write(fd, &data, sizeof(data));
		if (r != sizeof(data))
			ALOGE("NVM Write - Failed to write file data, r = %lu, errno = %d (%s)",
				r, errno, strerror(errno));
		else {
			is_success = true;
			ALOGV("NVM Write - File write count = %llu, timestamp = %s",
				hdr.file_write_count, ctime(&hdr.timestamp));
			ALOGV("NVM Write - OCV = %d, rbatt = %d", data.ocv_uv, data.rbatt);
			ALOGV("NVM Write - Charge %% increase = %d, charge cycles = %d",
				data.charge_increase, data.chargecycles);
			ALOGV("NVM Write - Aged capacity = %d, Calculated @ %s",
				data.aged_capacity, ctime(&data.aged_timestamp));
			ALOGV("NVM Write - BOC percent = %d, ocv = %d, cc = %d",
				data.boc_percent, data.boc_ocv_uv, data.boc_cc_uah);
			ALOGV("NVM Write - EOC percent = %d, ocv = %d, cc = %d",
				data.eoc_percent, data.eoc_ocv_uv, data.eoc_cc_uah);
		}
	}
	return is_success;
}

/* Syncs the given file */
static void nvm_file_fsync(char *path)
{
	int fd;
	bool is_success = false;

	fd = open(path, O_RDONLY);
	if (fd <0)
		ALOGE("NVM Write - Can not open %s for sync, errno = %d (%s)",
			path, errno, strerror(errno));
	else {
		if (fsync(fd) != 0)
			ALOGE("NVM Write - Failed to sync, errno = %d (%s)",
				errno, strerror(errno));
		else
			is_success = true;
		close(fd);
	}

	/* If failed to do individual fsync, just sync the whole thing */
	if (!is_success)
		sync();
}

/* Does range checks on data read from NVM */
static void nvm_file_bms_data_purify(bhd_state_t *state)
{
	/* Limit table for BMS values.  Set limit value to BHD_INVALID_VALUE if there is no
	   limit */
	const nvm_file_limit_t limit_table[] = {
		/* TODO: Need to determine valid ranges for:
		   real_fcc_batt_temp, real_fcc, rbatt */
		{offsetof(bhd_state_t, bms.soc), 0, 100,
		 	 BHD_INVALID_VALUE, "SOC"},
		/* TODO: These values reasonable? */
		{offsetof(bhd_state_t, bms.ocv_uv), 1000000, 5000000,
			 BHD_INVALID_VALUE, "OCV UV"},
		{offsetof(bhd_state_t, bms.charge_increase), 0, 100,
		 	0, "charge increase"},
		{offsetof(bhd_state_t, bms.chargecycles), 0, BHD_INVALID_VALUE,
		 	0, "charge cycles"},
		{offsetof(bhd_state_t, aged_capacity), 0, 100,
		 	0, "aged capacity"}
	};
	int num_elem = (sizeof(limit_table) / sizeof(limit_table[0]));
	int i;
	int *val_ptr;

	for (i = 0; i < num_elem; i++) {
		val_ptr = (int*)( (char *) state + limit_table[i].offset);
		ALOGV("NVM purify check - %s value = %d (range: low = %d, high = %d)",
			limit_table[i].name, *val_ptr,
			limit_table[i].low_limit, limit_table[i].high_limit);
		if (*val_ptr != BHD_INVALID_VALUE) {
			if ( (limit_table[i].low_limit != BHD_INVALID_VALUE) &&
				(*val_ptr < limit_table[i].low_limit)) {
				ALOGE("NVM purify check - Parameter %s "
					"too low = %d, reset to %d",
					limit_table[i].name, *val_ptr, limit_table[i].def_val);
				*val_ptr = limit_table[i].def_val;
			} else if ( (limit_table[i].high_limit != BHD_INVALID_VALUE) &&
				(*val_ptr > limit_table[i].high_limit)) {
				ALOGE("NVM purify check - Parameter %s "
					"too high = %d, reset to %d",
					limit_table[i].name, *val_ptr, limit_table[i].def_val);
				*val_ptr = limit_table[i].def_val;
			}
		}
	}
}

/* Loads latest battery health file from NVM */
bool BHD_NVM_file_load(bhd_state_t *state)
{
	bool is_success = false;
	bool read_success;
	bhd_state_t tmp_state[NVM_NUM_FILES];
	int r;
	int fd;
	char path[100];
	int i;
	int index_to_use = -1;
	unsigned long long int max_counter = 0;
	nvm_file_hdr_t hdr;
	int status;

	/* Attempt to read all battery health files associated with battery ID */
	state->file_write_count = 0;
	for (i = 0; i < NVM_NUM_FILES; i++) {
		read_success = false;

		/* Copy current state into temporary state */
		memcpy(&tmp_state[i], state, sizeof(*state));

		r = snprintf(path, sizeof(path), "%s%s_%d",
			NVM_PATH_ROOT, state->eeprom.uid_str, i);
		if (r < 0)
			ALOGE("NVM Read - Failed to create nvm path name");
		else if (r > (int) (sizeof(path) - 1))
			ALOGE("NVM Read - Failed to create nvm path name, too long");
		else {
			fd = open(path, O_RDONLY);
			if (fd < 0)
				ALOGE("NVM Read - Can not open %s, errno = %d (%s)",
					path, errno, strerror(errno));
			else {
				ALOGD("NVM Read - Opened NVM file %s", path);
				r = read(fd, &hdr, sizeof(hdr));
				if (r != sizeof(hdr))
				     ALOGE("Failed to read header, r = %d, errno = %d (%s)",
				                r, errno, strerror(errno));
				else if (hdr.version == NVM_FILE_HEADER_VERSION_0) {
					close(fd);
					status = remove(path);
					if (status == 0 ) {
						ALOGD("File deletion with version0 success %s",
									path);
					}
					else
						ALOGE("Unable to delete file %s\n",path);
					continue;
				}
				else {
						if(lseek(fd,0L,SEEK_SET) < 0)
							ALOGE("lseek failed \n");
						else
							read_success = nvm_file_read(fd, &tmp_state[i]);
				}
				close(fd);
			}
		}

		/* If the file was read successfully, determine if it is the latest
		   data file based on file write counter */
		if (read_success && (tmp_state[i].file_write_count >= max_counter)) {
			index_to_use = i;
			max_counter = tmp_state[i].file_write_count;
		}
	}

	/* If we found a valid file, set the current state to the temp state
	   associated with the valid file */
	if (index_to_use != -1) {
		ALOGD("NVM Load - Found newest file, index to use = %d", index_to_use);
		is_success = true;
		memcpy(state, &tmp_state[index_to_use], sizeof(*state));

		/* Range check the values that were read */
		nvm_file_bms_data_purify(state);
		state->last_chg_inc_write = state->bms.charge_increase;
		state->file_next_write = (index_to_use + 1) % NVM_NUM_FILES;
		state->file_write_count++;
	}

	return is_success;
}

/* Save battery health state to a file */
bool BHD_NVM_file_save(bhd_state_t *state)
{
	bool is_success = false;
	int r;
	int fd;
	char path[100];
	const char tmp_path[] = NVM_PATH_ROOT"tmp";

	if (state->is_factory_mode) {
		ALOGE("NVM Write - In factory mode, forbid saving to NVM");
		return is_success;
	}

	r = snprintf(path, sizeof(path), "%s%s_%d",
		NVM_PATH_ROOT, state->eeprom.uid_str, state->file_next_write);
	if (r < 0)
		ALOGE("NVM Write - Failed to create nvm path name");
	else if (r > (int) (sizeof(path) - 1))
		ALOGE("NVM Write - Failed to create nvm path name, too long");
	else {
		/* Write data to temp file first */
		fd = open(tmp_path, O_WRONLY | O_CREAT,
			(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
		if (fd <0)
			ALOGE("NVM Write - Can not open %s, errno = %d (%s)",
				path, errno, strerror(errno));
		else {
			ALOGD("NVM Write - Opened NVM file %s", path);
			is_success = nvm_file_write(fd, state);
			close(fd);
			if (is_success) {
				/* If wrote to temp file successfully, move
				   to final location */
				r = rename(tmp_path, path);
				if (r) {
					is_success = false;
					ALOGE("Rename %s to %s failed, errno = %d (%s)",
						tmp_path, path, errno, strerror(errno));
				} else {
					nvm_file_fsync(path);
					state->file_next_write = (state->file_next_write + 1) %
						NVM_NUM_FILES;
					state->file_write_count++;
					state->last_chg_inc_write = state->bms.charge_increase;
					ALOGD("NVM Write - Successfully updated NVM file");
				}
			}
		}
	}

	return is_success;
}
