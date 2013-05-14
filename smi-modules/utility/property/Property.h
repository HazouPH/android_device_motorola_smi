/* Property.h
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
#pragma once

#include "PropertyBase.h"

using namespace std;

template <typename type>
class TProperty : public CPropertyBase
{
public:
    TProperty(const string& strProperty, const type& typeDefaultValue = type());

    // get the property
    type getValue() const;

    // set the property
    bool setValue(const type& typeVal);

    // Cast accessor
    operator type() const;

private:
    // property default value
    type _typeDefaultValue;
};
