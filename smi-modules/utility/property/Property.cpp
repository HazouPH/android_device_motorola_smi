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
#include "Property.h"
#include <stdint.h>

#define base CPropertyBase

template <typename type>
TProperty<type>::TProperty(const string& strProperty, const type& typeDefaultValue) :
    CPropertyBase(strProperty), _typeDefaultValue(typeDefaultValue)
{
    // Convert default value to string
    ostringstream ostr;
    ostr << typeDefaultValue;

    // Set the default value
    setDefaultValue(ostr.str());
}

// get the property
template <typename type>
type TProperty<type>::getValue() const
{
    type typeValue = type();

    // Convert the string property to the correct type
    istringstream istr(get());
    istr >> boolalpha >> typeValue;
    if (istr.fail()) {

        typeValue = _typeDefaultValue;
    }

    return typeValue;
}

// set the property
template <typename type>
bool TProperty<type>::setValue(const type& typeVal)
{
    ostringstream ostr;
    ostr << boolalpha << typeVal;

    return set(ostr.str());
}

// Cast accessor
template <typename type>
TProperty<type>::operator type() const
{
    return getValue();
}

// Export supported types as library symbols
#include "PropertyTemplateInstanciations.h"
