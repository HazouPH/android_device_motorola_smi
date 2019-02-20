/*
 * Copyright (C) 2016 The CyanogenMod Project
 * Copyright (C) 2017 The LineageOS Project
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

package org.lineageos.settings.device;

import android.content.Intent;
import android.os.Bundle;
import android.os.UserHandle;
import android.support.v14.preference.PreferenceFragment;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.Preference;
import android.provider.Settings;
import android.widget.Switch;

import org.lineageos.settings.device.widget.SwitchBar;

public class GesturePreferenceFragment extends PreferenceFragment implements
        Preference.OnPreferenceChangeListener, SwitchBar.OnSwitchChangeListener {

    private SwitchBar mSwitchBar;
    private SwitchPreference mPocketPreference;
    private SwitchPreference mHandwavePreference;
    private SwitchPreference mHandwaveFlatPreference;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        boolean isDozeEnabled = isDozeEnabled();
        mSwitchBar = ((GesturePreferenceActivity) getActivity()).getSwitchBar();
        mSwitchBar.setChecked(isDozeEnabled);
        mSwitchBar.show();
        mSwitchBar.addOnSwitchChangeListener(this);
        getPreferenceScreen().setEnabled(isDozeEnabled);
    }

    @Override
    public void onDestroyView() {
        mSwitchBar.removeOnSwitchChangeListener(this);
        super.onDestroyView();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.gesture_panel);
        mPocketPreference = (SwitchPreference) findPreference(Constants.PREF_GESTURE_POCKET_KEY);
        mPocketPreference.setOnPreferenceChangeListener(this);
        mHandwavePreference =
                (SwitchPreference) findPreference(Constants.PREF_GESTURE_HAND_WAVE_KEY);
        mHandwavePreference.setOnPreferenceChangeListener(this);
        mHandwaveFlatPreference =
                (SwitchPreference) findPreference(Constants.PREF_GESTURE_HAND_WAVE_FLAT_KEY);
        mHandwaveFlatPreference.setEnabled(mHandwavePreference.isChecked());
    }

    private boolean enableDoze(boolean enable) {
        boolean result = Settings.Secure.putInt(getActivity().getContentResolver(),
                Settings.Secure.DOZE_ENABLED, enable ? 1 : 0);
        LineageActionsReceiver.notifyChanged(getActivity());
        return result;
    }

    private boolean isDozeEnabled() {
        return Settings.Secure.getInt(getActivity().getContentResolver(),
                Settings.Secure.DOZE_ENABLED, 1) != 0;
    }

    private void serviceEnabled(boolean enable) {
        Intent intent = new Intent(getContext(), LineageActionsService.class);
        if (enable) {
            getContext().startServiceAsUser(intent, UserHandle.CURRENT);
        } else {
            getContext().stopServiceAsUser(intent, UserHandle.CURRENT);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        boolean enable = (boolean) newValue;
        if (preference.equals(mPocketPreference)) {
            serviceEnabled(enable || mHandwavePreference.isChecked());
            return true;
        } else if (preference.equals(mHandwavePreference)) {
            serviceEnabled(enable || mPocketPreference.isChecked());
            mHandwaveFlatPreference.setEnabled(enable);
            return true;
        }
        return false;
    }

    @Override
    public void onSwitchChanged(Switch switchView, boolean isChecked) {
        enableDoze(isChecked);
        getPreferenceScreen().setEnabled(isChecked);
    }
}
