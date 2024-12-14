package org.kde.kasts;

import android.util.Log;
import androidx.media3.common.SimpleBasePlayer;

class Receiver extends BroadcastReceiver {

}

class KastsPlayer extends SimpleBasePlayer {
}

public class MediaNotification {
    static MediaNotification instance;
    void init() {
        Log.d("Hello", "Play");

    }

    public static void play() {
        instance = new MediaNotification();
        instance.init();
    }
}
