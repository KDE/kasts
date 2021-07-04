// SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later


package org.kde.kasts;

import org.qtproject.qt5.android.bindings.QtActivity;

import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.CalendarContract;
import android.os.Bundle;
import android.util.Log;

import java.io.*;
import java.util.*;

public class Activity extends QtActivity
{
    private static final String TAG = "org.kde.kasts";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}
