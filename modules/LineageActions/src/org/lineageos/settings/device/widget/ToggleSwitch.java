/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.lineageos.settings.device.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.Switch;

public class ToggleSwitch extends Switch {

    private ToggleSwitch.OnBeforeCheckedChangeListener mOnBeforeListener;

    public static interface OnBeforeCheckedChangeListener {
        public boolean onBeforeCheckedChanged(ToggleSwitch toggleSwitch, boolean checked);
    }

    public ToggleSwitch(Context context) {
        super(context);
    }

    public ToggleSwitch(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ToggleSwitch(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public ToggleSwitch(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public void setOnBeforeCheckedChangeListener(OnBeforeCheckedChangeListener listener) {
        mOnBeforeListener = listener;
    }

    @Override
    public void setChecked(boolean checked) {
        if (mOnBeforeListener != null
                && mOnBeforeListener.onBeforeCheckedChanged(this, checked)) {
            return;
        }
        super.setChecked(checked);
    }

    public void setCheckedInternal(boolean checked) {
        super.setChecked(checked);
    }
}
