package org.kde.kasts;

import android.content.Intent;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.util.Log;

public class Receiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_MEDIA_BUTTON.equals(intent.getAction())) {
            KastsActivity.mSession.getController().dispatchMediaButtonEvent(intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT));
        } else {
            switch(intent.getAction()) {
                case "ACTION_PLAY":
                    Log.d("kasts", "play");
                break;
            }
        }
    }
}