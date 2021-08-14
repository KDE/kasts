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

    class MediaData {
        public String title;
        public String author;
        public String album;
        public long position;
        public long duration;
        public float playbackSpeed;
        public int state = 2;
        // add more variables here
    }

    static MediaData mediaData;

    private static MediaSessionCompat mSession;
    private static PlaybackStateCompat.Builder mPBuilder;
    private static KastsActivity activity;

    void updateNotification() {

        // TODO: Change all of these variables to the values in mediaData
        // add other required values

        MediaMetadataCompat.Builder metadata = new MediaMetadataCompat.Builder();

        switch(mediaData.state)
        {
            case 0:
                mPBuilder.setState(PlaybackStateCompat.STATE_PLAYING, mediaData.position, mediaData.playbackSpeed);
            case 1:
                mPBuilder.setState(PlaybackStateCompat.STATE_PAUSED, mediaData.position, mediaData.playbackSpeed);
            case 2:
                mPBuilder.setState(PlaybackStateCompat.STATE_STOPPED, mediaData.position, mediaData.playbackSpeed);
        }

        metadata.putString(MediaMetadataCompat.METADATA_KEY_TITLE, "The title");
        metadata.putString(MediaMetadataCompat.METADATA_KEY_AUTHOR, "Author");
        metadata.putString(MediaMetadataCompat.METADATA_KEY_ARTIST, "Author");
        metadata.putString(MediaMetadataCompat.METADATA_KEY_ALBUM, "The album");
        metadata.putLong(MediaMetadataCompat.METADATA_KEY_DURATION, 1000000);
        //TODO Image
        mSession.setMetadata(metadata.build());

        mPBuilder.setState(PlaybackStateCompat.STATE_PLAYING, 100000, 1.0f); //TODO:Logically we should remove this statement??

        Intent iPlay = new Intent(this, MediaSessionCallback.class);
        iPlay.setAction("ACTION_PLAY");
        PendingIntent piPlay = PendingIntent.getBroadcast(this, 0, iPlay, PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Action.Builder aPlay = new NotificationCompat.Action.Builder(
                R.drawable.ic_play_white, "Play", piPlay);

        Intent iPause = new Intent(this, MediaSessionCallback.class);
        iPause.setAction("ACTION_PAUSE");
        PendingIntent piPause = PendingIntent.getBroadcast(this, 0, iPause, PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Action.Builder aPause = new NotificationCompat.Action.Builder(
                R.drawable.ic_pause_white, "Pause", piPause);

        Intent iPrevious = new Intent(this, MediaSessionCallback.class);
        iPrevious.setAction("ACTION_PREVIOUS");
        PendingIntent piPrevious = PendingIntent.getBroadcast(this, 0, iPrevious, PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Action.Builder aPrevious = new NotificationCompat.Action.Builder(
                R.drawable.ic_previous_white, "Previous", piPrevious);

        Intent iNext = new Intent(this, MediaSessionCallback.class);
        iNext.setAction("ACTION_NEXT");
        PendingIntent piNext = PendingIntent.getBroadcast(this, 0, iNext, PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Action.Builder aNext = new NotificationCompat.Action.Builder(
                R.drawable.ic_next_white, "Next", piNext);

        Intent iOpenActivity = new Intent(this, KastsActivity.class);
        iOpenActivity.putExtra("deviceId", "device");

        NotificationCompat.Builder notification = new NotificationCompat.Builder(this, "media_control");
        notification
            .setAutoCancel(false)
            .setShowWhen(false)
            .setVisibility(androidx.core.app.NotificationCompat.VISIBILITY_PUBLIC)
            .setSubText("foobar")
            .setContentTitle("Foo's title")
            .setSmallIcon(this.getApplicationInfo().icon)
            .setChannelId("org.kde.neochat.channel")
            .setContentText("some random text");

        notification.addAction(aPrevious.build());
        notification.addAction(aPause.build());
        notification.addAction(aNext.build());
        mSession.setPlaybackState(mPBuilder.build());
        MediaStyle mediaStyle = new MediaStyle();
        mediaStyle.setMediaSession(mSession.getSessionToken());
        mediaStyle.setShowActionsInCompactView(0, 1, 2);
        notification.setStyle(mediaStyle);
        notification.setGroup("MprisMediaSession");
        mSession.setActive(true);
        NotificationManager nm = ContextCompat.getSystemService(this, NotificationManager.class);
        NotificationChannel channel = new NotificationChannel("org.kde.neochat.channel", "KastsChannel", NotificationManager.IMPORTANCE_HIGH);
        channel.setDescription("The notification channel");
        channel.enableLights(false);
        channel.enableVibration(false);
        nm.createNotificationChannel(channel);
        nm.notify(0x487671, notification.build());
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mSession = new MediaSessionCompat(this, TAG);
        mSession.setFlags(
                MediaSessionCompat.FLAG_HANDLES_MEDIA_BUTTONS |
                MediaSessionCompat.FLAG_HANDLES_QUEUE_COMMANDS |
                MediaSessionCompat.FLAG_HANDLES_TRANSPORT_CONTROLS);
        mSession.setCallback(new MediaSessionCallback(this));
        mPBuilder = new PlaybackStateCompat.Builder();
        activity = this;
        mediaData = new MediaData();

        updateNotification();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        mSession.release();
    }

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

            //Update variables of mediaData;
            activity.updateNotification();

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

    /*
    * JNI METHODS
    */

    public static void setSessionState(int state)
    {
        //TODO: set state in mediadata

        mediaData.state = state;

        activity.updateNotification();
    }

    public static void setMetadata(String title, String author, String album, long position, long duration, float rate)
    {
        mediaData.title = title;
        mediaData.author = author;
        mediaData.album = album;
        mediaData.position = position;
        mediaData.duration = duration;
        mediaData.playbackSpeed = rate;

        activity.updateNotification();
    }

    public static void setPlaybackSpeed(int rate)
    {
        mediaData.playbackSpeed = rate;

        activity.updateNotification();
    }

    public static void setDuration(int duration)
    {
        mediaData.duration = duration;

        activity.updateNotification();
    }

    public static void setPosition(int position)
    {
        mediaData.position = position;

        activity.updateNotification();
    }
}
