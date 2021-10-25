// SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>
// SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
// SPDX-FileCopyrightText: 2021 Tobias Fella <fella@posteo.de>
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
        public String title = "Unknown Media";
        public String author = "Unknown Artist";
        public String album = "Unknown Album";
        public long position = 0;
        public long duration = 0;
        public float playbackSpeed = 1;
        public int state = 2;
    }

    private static MediaData mediaData;
    static MediaSessionCompat mSession;
    private static PlaybackStateCompat.Builder mPBuilder;
    private static KastsActivity activity;
    private static MediaMetadataCompat.Builder metadata = new MediaMetadataCompat.Builder();
    private NotificationCompat.Builder notification = new NotificationCompat.Builder(this, "mediaControl");

    void updateNotification() {
        switch(mediaData.state) {
            case 0:
                mPBuilder.setState(PlaybackStateCompat.STATE_PLAYING, mediaData.position, mediaData.playbackSpeed);
                break;
            case 1:
                mPBuilder.setState(PlaybackStateCompat.STATE_PAUSED, mediaData.position, mediaData.playbackSpeed);
                break;
            case 2:
                mPBuilder.setState(PlaybackStateCompat.STATE_STOPPED, mediaData.position, mediaData.playbackSpeed);
                break;
            default:
                Log.e("Notification", "Invalid state: " + mediaData.state);
        }

        metadata.putString(MediaMetadataCompat.METADATA_KEY_TITLE, mediaData.title);
        metadata.putString(MediaMetadataCompat.METADATA_KEY_AUTHOR, mediaData.author);
        //metadata.putString(MediaMetadataCompat.METADATA_KEY_ARTIST, mediaData.author);
        metadata.putString(MediaMetadataCompat.METADATA_KEY_ALBUM, mediaData.album);
        metadata.putLong(MediaMetadataCompat.METADATA_KEY_DURATION, mediaData.duration);
        //TODO Image
        mSession.setMetadata(metadata.build());

        Intent iPlay = new Intent(this, Receiver.class);
        iPlay.setAction("ACTION_PLAY");
        PendingIntent piPlay = PendingIntent.getBroadcast(this, 0, iPlay, PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Action.Builder aPlay = new NotificationCompat.Action.Builder(
                R.drawable.ic_play_white, "Play", piPlay);

        Intent iPause = new Intent(this, Receiver.class);
        iPause.setAction("ACTION_PAUSE");
        PendingIntent piPause = PendingIntent.getBroadcast(this, 0, iPause, PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Action.Builder aPause = new NotificationCompat.Action.Builder(
                R.drawable.ic_pause_white, "Pause", piPause);

        Intent iNext = new Intent(this, Receiver.class);
        iNext.setAction("ACTION_NEXT");
        PendingIntent piNext = PendingIntent.getBroadcast(this, 0, iNext, PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Action.Builder aNext = new NotificationCompat.Action.Builder(
                R.drawable.ic_next_white, "Next", piNext);

        Intent iOpenActivity = new Intent(this, KastsActivity.class);

        notification
            .setAutoCancel(false)
            .setShowWhen(false)
            .setVisibility(androidx.core.app.NotificationCompat.VISIBILITY_PUBLIC)
            .setSubText(mediaData.author)
            .setContentTitle(mediaData.title)
            .setSmallIcon(this.getApplicationInfo().icon)
            .setChannelId("org.kde.kasts.mediaNotification")
            .setContentText("Unknown")
            .setContentIntent(PendingIntent.getActivity(this, 0, iOpenActivity, 0));

        if(mediaData.state == 0)
            notification.addAction(aPause.build());
        else
            notification.addAction(aPlay.build());
        notification.addAction(aNext.build());
        mSession.setPlaybackState(mPBuilder.build());
        MediaStyle mediaStyle = new MediaStyle();
        mediaStyle.setMediaSession(mSession.getSessionToken());
        mediaStyle.setShowActionsInCompactView(0, 1);
        notification.setStyle(mediaStyle);
        notification.setGroup("MediaNotification");
        mSession.setActive(true);
        NotificationManager nm = ContextCompat.getSystemService(this, NotificationManager.class);
        NotificationChannel channel = new NotificationChannel("org.kde.kasts.mediaNotification", "Media Notification", NotificationManager.IMPORTANCE_HIGH);
        channel.setDescription("No Media Loaded");
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

    /*
    * JNI METHODS
    */
    public static void setSessionState(int state)
    {
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

    public static void setDuration(long duration)
    {
        mediaData.duration = duration;
        activity.updateNotification();
    }

    public static void setPosition(long position)
    {
        mediaData.position = position;
        activity.updateNotification();
    }
}
