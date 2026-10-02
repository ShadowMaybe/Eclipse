package me.shadow.eclipselauncher.customcontrols.gamepad;

import me.shadow.eclipselauncher.GrabListener;

public interface GamepadDataProvider {
    GamepadMap getMenuMap();
    GamepadMap getGameMap();
    boolean isGrabbing();
    void attachGrabListener(GrabListener grabListener);
    void detachGrabListener(GrabListener grabListener);
}
