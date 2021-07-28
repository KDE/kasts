/**
 * SPDX-FileCopyrightText: 2021 Swapnil Tripathi <swapnil06.st@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */


package org.kde.kasts;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.Bundle;
import android.support.v4.media.session.MediaSessionCompat;

public class MediaService extends Service {
    public static final String TAG = "MediaService";

    private MediaSessionCompat mSession;
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
    }
}
