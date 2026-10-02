package me.shadow.eclipselauncher.prefs.screens;

import android.os.Bundle;

import me.shadow.eclipselauncher.R;

public class LauncherPreferenceExperimentalFragment extends LauncherPreferenceFragment {

    @Override
    public void onCreatePreferences(Bundle b, String str) {
        addPreferencesFromResource(R.xml.pref_experimental);
    }
}
