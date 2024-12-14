package org.kde.kasts;

import android.util.Log;
import androidx.media3.common.SimpleBasePlayer;
import androidx.media3.common.Player;

class Receiver extends BroadcastReceiver {

}

class KastsPlayer extends SimpleBasePlayer {
    @Override
    public SimpleBasePlayer.State getState() {
        SimpleBasePlayer.State.Builder builder = new SimpleBasePlayer.State.Builder();

        Player.Commands.Builder commandsBuilder = new Player.Commands.Builder();
        commandsBuilder.add(Player.COMMAND_PLAY_PAUSE);
        builder.setAvailableCommands(commandsBuilder.build());
        builder.setContentPositionMs(1234);
        builder.setPlaybackState(Player.State.STATE_READY);
        return builder.build();
    }
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
