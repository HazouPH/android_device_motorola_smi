/* PropertyBase.h
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

#include <string>

using namespace std;

class CPropertyBase
{
public:

    virtual ~CPropertyBase();

protected:

    CPropertyBase(const string& strProperty);

    // set default value
    void setDefaultValue(const string& strDefaultValue);

    // get the property
    string get() const;

    // set the property
    bool set(const string& strVal);

private:

    // property key
    string _strProperty;

    // property default value
    string _strDefaultValue;
};
