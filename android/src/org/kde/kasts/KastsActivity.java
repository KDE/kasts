// SPDX-FileCopyrightText: 2018 Matthieu Gallien <matthieu_gallien@yahoo.fr>
// SPDX-FileCopyrightText: 2026 Bart De Vries <bart@mogwai.be>
//
// SPDX-License-Identifier: LGPL-3.0-or-later

package org.kde.kasts;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import androidx.core.content.ContextCompat;

import org.qtproject.qt.android.bindings.QtActivity;

@SuppressLint("LongLogTag")
public class KastsActivity extends QtActivity
{
    private static final String TAG = "org.kde.kasts.KastsActivity";

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate: "+savedInstanceState);

        KastsService.createNotificationChannel(this);

        Intent serviceIntent = new Intent(this, KastsService.class);
        ContextCompat.startForegroundService(this, serviceIntent);
    }

    @Override
    public void onDestroy()
    {
        Intent serviceIntent = new Intent(this, KastsService.class);
        stopService(serviceIntent);

        super.onDestroy();
    }
}
