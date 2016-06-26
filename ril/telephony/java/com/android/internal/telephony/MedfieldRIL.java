package com.android.internal.telephony;

import static com.android.internal.telephony.RILConstants.*;

import android.content.Context;
import android.media.AudioManager;
import android.os.AsyncResult;
import android.os.Message;
import android.os.Parcel;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.telephony.SignalStrength;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;

import com.android.internal.telephony.uicc.IccCardApplicationStatus;
import com.android.internal.telephony.uicc.IccCardStatus;

/**
 * Provides SignalStrength correction for old MedfieldRIL extends RIL
 */
public class MedfieldRIL extends RIL {

    public MedfieldRIL(Context context, int NetworkType, int cdmaSubscription) {
        super(context, NetworkType, cdmaSubscription);
       mQANElements = 5;
    }

    public MedfieldRIL(Context context, int NetworkType, int cdmaSubscription, Integer instanceId) {
        super(context, NetworkType, cdmaSubscription, instanceId);
       mQANElements = 5;
    }
}
