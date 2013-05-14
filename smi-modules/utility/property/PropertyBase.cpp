/*
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 ** http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
#define LOG_TAG "PROPERTY"

#include <errno.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <sstream>
#include "PropertyBase.h"

CPropertyBase::CPropertyBase(const string& strProperty) :
    _strProperty(strProperty)
{
    if (strProperty.length() > PROPERTY_KEY_MAX) {

        ALOGE("Invalid property name, it shall be less than %d characters: %s", PROPERTY_KEY_MAX, strProperty.c_str());
    }
}

CPropertyBase::~CPropertyBase()
{
}

// set default value
void CPropertyBase::setDefaultValue(const string& strDefaultValue)
{
    _strDefaultValue = strDefaultValue;
}

// get the property
string CPropertyBase::get() const
{
    char acValue[PROPERTY_VALUE_MAX];

    // Retrieve property
    property_get(_strProperty.c_str(), acValue, _strDefaultValue.c_str());

    ALOGD("%s %s: %s", __FUNCTION__, _strProperty.c_str(), acValue);

    return acValue;
}

// set the property
bool CPropertyBase::set(const string& strVal)
{
    ALOGD("%s %s: %s", __FUNCTION__, _strProperty.c_str(), strVal.c_str());

    if (strVal.length() > PROPERTY_VALUE_MAX) {

        ALOGE("Invalid property value, it shall be less than %d characters: %s", PROPERTY_VALUE_MAX, strVal.c_str());
        return false;
    }

    if (property_set(_strProperty.c_str(), strVal.c_str()) < 0) {

        return false;
    }
    return true;
}
