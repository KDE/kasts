// SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

package org.kde.kasts;

import org.qtproject.qt5.android.bindings.QtActivity;

import android.app.PendingIntent;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.CalendarContract;
import android.os.Bundle;
import android.util.Log;

import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.app.Service;
import android.content.Context;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.support.v4.media.MediaMetadataCompat;
import androidx.core.app.NotificationCompat;
import androidx.media.app.NotificationCompat.MediaStyle;

import androidx.core.content.ContextCompat;

import java.io.*;
import java.util.*;

public class KastsActivity extends QtActivity
{
    private static final String TAG = "org.kde.kasts.mediasession";

    private static KastsActivity activity;

    private static MediaData mediaData;
    private static MediaSessionCompat mediaSession;
    private static PlaybackStateCompat.Builder mediaPlayback;

    private static NotificatiooCompat.Builder notification;

    private static NotificationCompat.Action.Builder play;
    private static NotificationCompat.Action.Builder pause;
    private static NotificationCompat.Action.Builder next;
    private static MediaStyle mediaStyle;
    private static NotificationManager notificationManager;

    void initNotification() {
        Intent iPlay = new Intent(this, Receiver.class);
        iPlay.setAction("ACTION_PLAY");
        PendingIntent piPlay = PendingIntent.getBroadcast(this, 0, iPlay, PendingIntent.FLAG_UPDATE_CURRENT);
        play = new NotificationCompat.Action.Builder(R.drawable.ic_play_white, "Play", piPlay);

        Intent iPause = new Intent(this, Receiver.class);
        iPause.setAction("ACTION_PAUSE");
        PendingIntent piPause = PendingIntent.getBroadcast(this, 0, iPause, PendingIntent.FLAG_UPDATE_CURRENT);
        pause = new NotificationCompat.Action.Builder(R.drawable.ic_pause_white, "Pause", piPause);

        Intent iNext = new Intent(this, Receiver.class);
        iNext.setAction("ACTION_NEXT");
        PendingIntent piNext = PendingIntent.getBroadcast(this, 0, iNext, PendingIntent.FLAG_UPDATE_CURRENT);
        next = new NotificationCompat.Action.Builder(R.drawable.ic_next_white, "Next", piNext);

        Intent iOpenActivity = new Intent(activity, KastsActivity.class);
        notification = new NotificationCompat.Builder(activity, "media_control");
        notification.setAutoCancel(false)
            .setShowWhen(false)
            .setVisibility(androidx.core.app.NotificationCompat.VISIBILITY_PUBLIC)
            .setSmallIcon(this.getApplicationInfo().icon)
            .setChannelId("org.kde.kasts.channel")
            .setContentText("Unknown")
            .setContentIntent(PendingIntent.getActivity(this, 0, iOpenActivity, 0));

        mediaStyle = new MediaStyle();
        mediaStyle.setMediaSession(mediaSession.getSessionToken());
        mediaStyle.setShowActionsInCompactView(0, 1, 2);
        notification.setStyle(mediaStyle);
        mediaSession.setActive(true);
        notification.setGroup("MprisMediaSession");
        NotificationManager nm = ContextCompat.getSystemService(this, NotificationManager.class);
        NotificationChannel channel = new NotificationChannel("org.kde.kasts.channel", "KastsChannel", NotificationManager.IMPORTANCE_HIGH);
        channel.setDescription("No Media Loaded");
        channel.enableLights(false);
        channel.enableVibration(false);
        notificationManager.createNotificationChannel(channel);
    }

    void updateNotification() {
        nm.notify(0x487671, notification.build());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mediaSession = new MediaSessionCompat(this, TAG);
        mediaSession.setFlags(
                MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS |
                MediaSessionCompat.FLAG_HANDLES_QUEUE_COMMANDS |
                MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS);
        mediaSession.setCallback(new MediaSessionCallback(this));
        mediaPlayback = new PlaybackStateCompat.Builder();
        activity = this;
        mediaData = new MediaData();

        initNotification();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        mediaSession.release();
    }

    private final class MediaSessionCallback extends MediaSessionCompat.Callback {
        private Context mContext;

        public MediaSessionCallback(Context context) {
            super();

            mContext = context;
        }

        @Override
        public void onPlay() {
            super.onPlay();

            if (!mediaSession.isActive()) {
                mediaSession.setActive(true);
            }
        }

        @Override
        public void onPause() {
            super.onPause();

            //JNI to audiomanager pause
            //setPlaybackState for mediaSession
        }

        @Override
        public void onStop() {
            super.onStop();

            //JNI call to audiomanager stop
            mediaSession.setActive(false);
        }

        @Override
        public void onSkipToNext() {
            super.onPause();

            //JNI to audiomanager next
        }
    }

    public static void setSessionState(int state, float speed, long position)
    {
        Log.e(TAG, "Updating session state");
        notification.clearActions();
        switch(state) {
            case 0:
                mediaPlayback.setState(MediaPlaybackCompat.PLAYING, position, playbackSpeed);
                notification.addAction(pause.build());
                notification.addAction(next.build());
                break;
                case 1:
                mediaPlayback.setState(MediaPlaybackCompat.PAUSED, position, playbackSpeed);
                notification.addAction(play.build());
                notification.addAction(next.build());
                break;
            case 2:
                mediaPlayback.setState(MediaPlaybackCompat.STOPPED, position, playbackSpeed);
                notification.addAction(play.build());
                notification.addAction(next.build());
                break;
        }

        mediaSession.setPlaybackState(mediaPlayback.build());
        activity.updateNotification();
    }

    public static void setMetadata(String title, String author, String album, long position, long duration, float rate)
    {
        Log.e(TAG, "Handling metadata");

        var metadata = new MediaMetadataCompat.Builder();
        metadata.putString(MediaMetadataCompat.METADATA_KEY_TITLE, title);
        metadata.putString(MediaMetadataCompat.METADATA_KEY_AUTHOR, author);
        metadata.putString(MediaMetadataCompat.METADATA_KEY_ARTIST, author);
        metadata.putString(MediaMetadataCompat.METADATA_KEY_ALBUM, album);
        metadata.putLong(MediaMetadataCompat.METADATA_KEY_DURATION, duration);

        notification.setSubText(author).setContentTitle(title);
        mediaSession.setMetadata(metadata.build());

        activity.updateNotification();
    }
}
