package me.shadow.eclipselauncher.fragments;

import static me.shadow.eclipselauncher.Tools.openPath;
import static me.shadow.eclipselauncher.Tools.shareLog;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import me.shadow.eclipselauncher.widget.mcgui.mcVersionSpinner;

import me.shadow.eclipselauncher.CustomControlsActivity;
import me.shadow.eclipselauncher.R;
import me.shadow.eclipselauncher.Tools;
import me.shadow.eclipselauncher.extra.ExtraConstants;
import me.shadow.eclipselauncher.extra.ExtraCore;
import me.shadow.eclipselauncher.prefs.LauncherPreferences;
import me.shadow.eclipselauncher.progresskeeper.ProgressKeeper;
import me.shadow.eclipselauncher.value.MinecraftAccount;
import me.shadow.eclipselauncher.value.launcherprofiles.LauncherProfiles;
import me.shadow.eclipselauncher.value.launcherprofiles.MinecraftProfile;

import java.io.File;

public class MainMenuFragment extends Fragment {
    public static final String TAG = "MainMenuFragment";

    private mcVersionSpinner mVersionSpinner;

    public MainMenuFragment(){
        super(R.layout.fragment_launcher);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        Button mNewsButton = view.findViewById(R.id.news_button);
        Button mCustomControlButton = view.findViewById(R.id.custom_control_button);
        Button mInstallJarButton = view.findViewById(R.id.install_jar_button);
        Button mShareLogsButton = view.findViewById(R.id.share_logs_button);
        Button mOpenDirectoryButton = view.findViewById(R.id.open_files_button);

        ImageButton mEditProfileButton = view.findViewById(R.id.edit_profile_button);
        Button mPlayButton = view.findViewById(R.id.play_button);
        mVersionSpinner = view.findViewById(R.id.mc_version_spinner);

        mNewsButton.setOnClickListener(v -> {
            if (!requireAccount()) return;
            Tools.openURL(requireActivity(), Tools.URL_HOME);
        });
        // Custom controls is exempt from the account gate. Settings > Control opens this
        // very same CustomControlsActivity through an <intent> preference row, and that
        // route never passes through Tools.swapFragment, so it was already free while
        // this button demanded a login - one feature, two different answers depending on
        // which door you came in by.
        mCustomControlButton.setOnClickListener(v ->
                startActivity(new Intent(requireContext(), CustomControlsActivity.class)));
        // The .jar button gates itself inside runInstallerWithConfirmation.
        mInstallJarButton.setOnClickListener(v -> runInstallerWithConfirmation());
        mEditProfileButton.setOnClickListener(v -> {
            if (!requireAccount()) return;
            mVersionSpinner.openProfileEditor(requireActivity());
        });

        mPlayButton.setOnClickListener(v -> ExtraCore.setValue(ExtraConstants.LAUNCH_GAME, true));

        mShareLogsButton.setOnClickListener((v) -> {
            if (!requireAccount()) return;
            shareLog(requireContext());
        });

        mOpenDirectoryButton.setOnClickListener((v)-> {
            if (!requireAccount()) return;
            openPath(v.getContext(), getCurrentProfileDirectory(), false);
        });

        // Long-press on the Wiki button used to jump straight to the controller mapper.
        // Dropped as a hidden gesture: Settings > Control > "Remap controller" reaches the
        // same GamepadMapperFragment, so the shortcut was redundant.
    }

    /**
     * Everything on the main menu except settings needs an account. The settings gear is
     * wired in the Activity and never comes through here, so there is nothing else to
     * exempt - and Tools.swapFragment applies the same rule to every other screen.
     *
     * @return true when the caller may proceed, false when a login was requested instead.
     */
    private boolean requireAccount() {
        if (MinecraftAccount.anyAccountExists()) return true;
        Toast.makeText(requireContext(), R.string.not_available_without_account, Toast.LENGTH_LONG).show();
        Tools.swapFragment(requireActivity(), SelectAuthFragment.class, SelectAuthFragment.TAG, null);
        return false;
    }

    private File getCurrentProfileDirectory() {
        String currentProfile = LauncherPreferences.DEFAULT_PREF.getString(LauncherPreferences.PREF_KEY_CURRENT_PROFILE, null);
        if(!Tools.isValidString(currentProfile)) return new File(Tools.DIR_GAME_NEW);
        LauncherProfiles.load();
        MinecraftProfile profileObject = LauncherProfiles.mainProfileJson.profiles.get(currentProfile);
        if(profileObject == null) return new File(Tools.DIR_GAME_NEW);
        return Tools.getGameDirPath(profileObject);
    }

    @Override
    public void onResume() {
        super.onResume();
        mVersionSpinner.reloadProfiles();
    }

    private void runInstallerWithConfirmation() {
        // Installing a mod loader writes into a game directory that belongs to a profile, so it is gated
        // exactly like launching: one account of any kind is enough, none is not.
        if (!MinecraftAccount.anyAccountExists()) {
            Toast.makeText(requireContext(), R.string.not_available_without_account, Toast.LENGTH_LONG).show();
            Tools.swapFragment(requireActivity(), SelectAuthFragment.class, SelectAuthFragment.TAG, null);
            return;
        }
        if (ProgressKeeper.getTaskCount() == 0)
            // false = run the installer with default Java arguments. The long-press that
            // used to ask for custom arguments is gone with the Wiki one; the AWT
            // installer gets its own pre-launch argument editor instead, which is why
            // Tools.installMod keeps its boolean parameter.
            Tools.installMod(requireActivity(), false);
        else
            Toast.makeText(requireContext(), R.string.tasks_ongoing, Toast.LENGTH_LONG).show();
    }
}
