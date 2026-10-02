package me.shadow.eclipselauncher.profiles;

public interface VersionSelectorListener {
    void onVersionSelected(String versionId, boolean isSnapshot);
}
