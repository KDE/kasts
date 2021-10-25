// SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

package org.kde.kasts;

import android.content.Intent;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.util.Log;

public class Receiver extends BroadcastReceiver {

    private static native void playerPlay();
    private static native void playerPause();
    private static native void playerNext();
    private static native void playerSeek(long position);

    @Override
    public void onReceive(Context context, Intent intent) {
        if (Intent.ACTION_MEDIA_BUTTON.equals(intent.getAction())) {
            KastsActivity.mSession.getController().dispatchMediaButtonEvent(intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT));
        } else {
            switch(intent.getAction()) {
                case "ACTION_PLAY":
                    playerPlay();
                break;
                case "ACTION_PAUSE":
                    playerPause();
                break;
                case "ACTION_NEXT":
                    playerNext();
                break;
            }
        }
    }
}
