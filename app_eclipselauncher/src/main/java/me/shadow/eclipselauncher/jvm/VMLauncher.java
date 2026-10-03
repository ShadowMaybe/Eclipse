package me.shadow.eclipselauncher.jvm;

public final class VMLauncher {
	private VMLauncher() {
	}
	public static native int launchJVM(String[] args);
}
