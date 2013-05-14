/*
 *
 * Copyright 2011 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "properties.h"

extern "C" {
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

const char* gpcBaseFolder = "/tmp/properties";

static void property_get_file_path(const char *key, char* filepath)
{
    // Build file path
    strcpy(filepath, gpcBaseFolder);
    strcat(filepath, "/");
    strcat(filepath, key);
}

/* property_get: returns the length of the value which will never be
** greater than PROPERTY_VALUE_MAX - 1 and will always be zero terminated.
** (the length does not include the terminating zero).
**
** If the property read fails or returns an empty value, the default
** value is used (if nonnull).
*/
int property_get(const char *key, char *value, const char *default_value)
{
    // Check key length (32 chars max)
    if (strlen(key) > PROPERTY_KEY_MAX) {

        return -1;
    }

    /// Read file
    char acFilePath[256];

    // Get path
    property_get_file_path(key, acFilePath);

    // return default
    strncpy(value, default_value, PROPERTY_VALUE_MAX);

    int fd = open(acFilePath, O_RDONLY);

    if (fd < 0) {

        // return default
        if (default_value) {

            strncpy(value, default_value, PROPERTY_VALUE_MAX);
        } else {

            strcpy(value, "");
        }

        return 0;
    }

    // Read
    int iRead = read(fd, value, PROPERTY_VALUE_MAX - 1);

    assert(iRead >= 0);

    // Close string
    value[iRead] = '\0';

    close(fd);

    return 0;
}

/* property_set: returns 0 on success, < 0 on failure
*/
int property_set(const char *key, const char *value)
{
    // Check key length (32 chars max)
    if (strlen(key) > PROPERTY_KEY_MAX) {

        return -1;
    }

    // Check value length
    if (strlen(value) > PROPERTY_VALUE_MAX) {

        return -1;
    }

    // Check folder
    struct stat sfolder;

    if (stat(gpcBaseFolder, &sfolder) == -1) {

        mkdir(gpcBaseFolder, S_IRWXU | S_IRWXG | S_IRWXO);
    }

    /// Write file
    char acFilePath[256];

    // Get path
    property_get_file_path(key, acFilePath);

    int fd = open(acFilePath, O_CREAT|O_WRONLY|O_TRUNC, 0664);

    if (fd < 0) {

        return -1;
    }
    // Write
    write(fd, value, strlen(value));

    close(fd);

    return 0;
}


}
