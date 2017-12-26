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

package com.cyanogenmod.cmactions;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.ContentObserver;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.Log;

public class CMActionsService extends Service {
    private static final String TAG = "CMActionsService";
    private static final boolean DEBUG = false;

    private static final int POCKET_DELTA_NS = 1000 * 1000 * 1000;

    private static final int INCLINATION_THS = 20;

    private Context mContext;
    private MotoProximitySensor mSensor;
    private PowerManager mPowerManager;
    private PowerManager.WakeLock mWakeLock;

    private boolean mHandwaveGestureEnabled = false;
    private boolean mPocketGestureEnabled = false;
    private boolean mHandwaveGestureFlatEnabled = false;

    private boolean mScreenStateReceiverAdded = false;

    private final ContentObserver mDozeContentObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (isDozeEnabled()) {
                addScreenStateReceiver();
            } else {
                removeScreenStateReceiver();
            }
        }
    };

    class MotoProximitySensor implements SensorEventListener {
        private SensorManager mSensorManager;
        private Sensor mProxSensor;
        private Sensor mAccelSensor;
        private boolean mProxEnabled = false;
        private boolean mAccelEnabled = false;

        private boolean mSawNear = false;
        private long mInPocketTime = 0;
        private boolean mPulseIfFlat = false;

        public MotoProximitySensor(Context context) {
            mSensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
            mProxSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_PROXIMITY);
            mAccelSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            int sensorType = event.sensor.getType();
            switch (sensorType) {
                case Sensor.TYPE_PROXIMITY:
                    checkProxEvent(event);
                    break;
                case Sensor.TYPE_ACCELEROMETER:
                    checkAccelEvent(event);
                    break;
            }
        }

        private void checkProxEvent(SensorEvent event) {
            boolean isNear = event.values[0] < mProxSensor.getMaximumRange();
            if (mSawNear && !isNear) {
                if (isHandWaveGesture(event.timestamp)) {
                    if (!mHandwaveGestureFlatEnabled) {
                        launchDozePulse();
                    } else {
                        mPulseIfFlat = true;
                        setAccelEnabled(true);
                    }
                } else if (isPocketGesture(event.timestamp)) {
                    launchDozePulse();
                }
            } else {
                mInPocketTime = event.timestamp;
            }
            mSawNear = isNear;
        }

        private void checkAccelEvent(SensorEvent event) {
            if (mPulseIfFlat) {
                mPulseIfFlat = false;

                // Taken from http://stackoverflow.com/a/15149421
                float x = event.values[0];
                float y = event.values[1];
                float z = event.values[2];
                float norm = (float) Math.sqrt(x * x + y * y + z * z);
                z /= norm;
                int inclination = (int) Math.round(Math.toDegrees(Math.acos(z)));

                if (DEBUG) Log.d(TAG, "Inclination=" + inclination);

                if (inclination < INCLINATION_THS) {
                    launchDozePulse();
                }
            }
            setAccelEnabled(false);
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            /* Empty */
        }

        private boolean isHandWaveGesture(long timestamp) {
            long delta = timestamp - mInPocketTime;
            return mHandwaveGestureEnabled && delta < POCKET_DELTA_NS;
        }

        private boolean isPocketGesture(long timestamp) {
            long delta = timestamp - mInPocketTime;
            return mPocketGestureEnabled && delta >= POCKET_DELTA_NS;
        }

        private void setProxEnabled(boolean enable) {
            if (mProxEnabled == enable) {
                return;
            }
            mProxEnabled = enable;
            if (enable) {
                mSensorManager.registerListener(this, mProxSensor,
                        SensorManager.SENSOR_DELAY_NORMAL);
            } else {
                mSensorManager.unregisterListener(this, mProxSensor);
            }
        }

        public void setAccelEnabled(boolean enable) {
            if (mAccelEnabled == enable) {
                return;
            }
            mAccelEnabled = enable;
            if (enable) {
                mSensorManager.registerListener(this, mAccelSensor,
                        SensorManager.SENSOR_DELAY_FASTEST);
            } else {
                mSensorManager.unregisterListener(this, mAccelSensor);
            }
        }
    }

    @Override
    public void onCreate() {
        if (DEBUG) Log.d(TAG, "CMActionsService Started");
        mContext = this;
        mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mSensor = new MotoProximitySensor(mContext);
        mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "CMActionsWakeLock");

        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);
        mHandwaveGestureEnabled =
                sharedPrefs.getBoolean(Constants.PREF_GESTURE_HAND_WAVE_KEY, false);
        mHandwaveGestureFlatEnabled =
                sharedPrefs.getBoolean(Constants.PREF_GESTURE_HAND_WAVE_FLAT_KEY, false);
        mPocketGestureEnabled = sharedPrefs.getBoolean(Constants.PREF_GESTURE_POCKET_KEY, false);
        sharedPrefs.registerOnSharedPreferenceChangeListener(mPrefListener);

        getContentResolver().registerContentObserver(
                Settings.Secure.getUriFor(Settings.Secure.DOZE_ENABLED),
                false, mDozeContentObserver);

        if (!isInteractive() && areGesturesEnabled() && isDozeEnabled()) {
            mSensor.setProxEnabled(true);
        }

        if (areGesturesEnabled()) {
            addScreenStateReceiver();
        }
    }

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "CMActionsService Stopped");
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);
        sharedPrefs.unregisterOnSharedPreferenceChangeListener(mPrefListener);
        getContentResolver().unregisterContentObserver(mDozeContentObserver);
        removeScreenStateReceiver();
        mSensor.setProxEnabled(false);
        mSensor.setAccelEnabled(false);
        holdWakelock(false);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (DEBUG) Log.d(TAG, "Starting service");
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private boolean isDozeEnabled() {
        return Settings.Secure.getInt(getContentResolver(),
                Settings.Secure.DOZE_ENABLED, 1) != 0;
    }

    private void addScreenStateReceiver() {
        if (!mScreenStateReceiverAdded) {
            if (DEBUG) Log.d(TAG, "Adding screen state receiver");
            IntentFilter screenStateFilter = new IntentFilter(Intent.ACTION_SCREEN_ON);
            screenStateFilter.addAction(Intent.ACTION_SCREEN_OFF);
            mContext.registerReceiver(mScreenStateReceiver, screenStateFilter);
            mScreenStateReceiverAdded = true;
        }
    }

    private void removeScreenStateReceiver() {
        if (mScreenStateReceiverAdded) {
            if (DEBUG) Log.d(TAG, "Removing screen state receiver");
            mContext.unregisterReceiver(mScreenStateReceiver);
            mScreenStateReceiverAdded = false;
        }
    }

    private void launchDozePulse() {
        mContext.sendBroadcastAsUser(new Intent(Constants.DOZE_INTENT), UserHandle.CURRENT);
    }

    private boolean isInteractive() {
        return mPowerManager.isInteractive();
    }

    private boolean areGesturesEnabled() {
        return mHandwaveGestureEnabled || mPocketGestureEnabled;
    }

    private void holdWakelock(boolean hold) {
        if (DEBUG) Log.d(TAG, "hold=" + hold + ", held=" + mWakeLock.isHeld());
        if (hold == mWakeLock.isHeld()) {
            return;
        }
        if (hold) {
            mWakeLock.acquire();
        } else {
            mWakeLock.release();
        }
    }

    private void onDisplayOn() {
        if (DEBUG) Log.d(TAG, "Display on");
        mSensor.setProxEnabled(false);
        if (areGesturesEnabled() && isDozeEnabled()) {
            holdWakelock(true);
        }
    }

    private void onDisplayOff() {
        if (DEBUG) Log.d(TAG, "Display off");
        if (areGesturesEnabled() && isDozeEnabled()) {
            mSensor.setProxEnabled(true);
        }
        holdWakelock(false);
    }

    private BroadcastReceiver mScreenStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF)) {
                onDisplayOff();
            } else if (intent.getAction().equals(Intent.ACTION_SCREEN_ON)) {
                onDisplayOn();
            }
        }
    };

    private SharedPreferences.OnSharedPreferenceChangeListener mPrefListener =
            new SharedPreferences.OnSharedPreferenceChangeListener() {
                @Override
                public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
                        String key) {
                    if (Constants.PREF_GESTURE_HAND_WAVE_KEY.equals(key)) {
                        mHandwaveGestureEnabled = sharedPreferences.getBoolean(
                                Constants.PREF_GESTURE_HAND_WAVE_KEY, false);
                    } else if (Constants.PREF_GESTURE_POCKET_KEY.equals(key)) {
                        mPocketGestureEnabled = sharedPreferences
                                .getBoolean(Constants.PREF_GESTURE_POCKET_KEY, false);
                    } else if (Constants.PREF_GESTURE_HAND_WAVE_FLAT_KEY.equals(key)) {
                        mHandwaveGestureFlatEnabled = sharedPreferences
                                .getBoolean(Constants.PREF_GESTURE_HAND_WAVE_FLAT_KEY, false);
                    }

                    if (areGesturesEnabled()) {
                        addScreenStateReceiver();
                        holdWakelock(true);
                    } else {
                        removeScreenStateReceiver();
                        holdWakelock(false);
                    }
                }
            };
}
