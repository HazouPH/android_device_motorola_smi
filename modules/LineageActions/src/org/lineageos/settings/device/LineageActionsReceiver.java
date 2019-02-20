/*
 * Copyright (c) 2016 The CyanogenMod Project
 * Copyright (c) 2017 The LineageOS Project
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

import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.provider.Settings;

import lineageos.preference.RemotePreferenceUpdater;

public class LineageActionsReceiver extends RemotePreferenceUpdater {

    @Override
    public void onReceive(final Context context, Intent intent) {
        super.onReceive(context, intent);
        if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
            if (areGesturesEnabled(context)) {
                context.startServiceAsUser(new Intent(context, LineageActionsService.class),
                        UserHandle.CURRENT);
            }
        }
    }

    private boolean areGesturesEnabled(Context context) {
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        return sharedPrefs.getBoolean(Constants.PREF_GESTURE_HAND_WAVE_KEY, false) ||
                sharedPrefs.getBoolean(Constants.PREF_GESTURE_POCKET_KEY, false);
    }

    @Override
    public String getSummary(Context context, String key) {
        if (Constants.DOZE_SETTINGS_TILE_KEY.equals(key)) {
            if (isDozeEnabled(context)) {
                return context.getString(R.string.ambient_display_summary_on);
            } else {
                return context.getString(R.string.ambient_display_summary_off);
            }
        }
        return null;
    }

    private boolean isDozeEnabled(Context context) {
        return Settings.Secure.getInt(context.getContentResolver(),
                Settings.Secure.DOZE_ENABLED, 1) != 0;
    }

    public static void notifyChanged(Context context) {
        notifyChanged(context, Constants.DOZE_SETTINGS_TILE_KEY);
    }
}
