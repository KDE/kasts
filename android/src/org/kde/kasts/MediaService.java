/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */


package org.kde.kasts;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.Bundle;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.util.Log;

public class MediaService extends Service {
    public static final String TAG = "MediaService";
    private MediaSessionCompat mSession;

    private class MediaSessionCallback extends MediaSessionCompat.Callback {
        private Context mContext;

        public MediaSessionCallback(Context context) {
            super();

            mContext = context;
        }

        @Override
        public void onPlay() {
            super.onPlay();

            mSession.setActive(true);

            //JNI to audiomanager play
            //setPlaybackState for mSession
        }

        @Override
        public void onPause() {
            super.onPause();

            //JNI to audiomanager pause
            //setPlaybackState for mSession
        }

        @Override
        public void onStop() {
            super.onStop();

            //JNI call to audiomanager stop
            mSession.setActive(false);
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();

        mSession = new MediaSessionCompat(this, TAG);
        mSession.setFlags(
                MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS |
                MediaSessionCompat.FLAG_HANDLES_QUEUE_COMMANDS |
                MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS);
        mSession.setCallback(new MediaSessionCallback(this));
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        mSession.release();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
