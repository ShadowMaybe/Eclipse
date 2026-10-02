package me.shadow.eclipselauncher.fragments;

import android.content.Context;
import android.view.LayoutInflater;
import android.widget.ExpandableListAdapter;
import android.widget.Toast;

import androidx.annotation.NonNull;

import me.shadow.eclipselauncher.R;
import me.shadow.eclipselauncher.Tools;
import me.shadow.eclipselauncher.modloaders.ForgeDownloadTask;
import me.shadow.eclipselauncher.modloaders.ForgeUtils;
import me.shadow.eclipselauncher.modloaders.ForgeVersionListAdapter;
import me.shadow.eclipselauncher.modloaders.ModloaderListenerProxy;

import java.io.File;
import java.io.IOException;
import java.util.List;

public class ForgeInstallFragment extends ModVersionListFragment<List<String>> {
    public static final String TAG = "ForgeInstallFragment";
    public ForgeInstallFragment() {
        super(TAG);
    }

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);
    }

    @Override
    public int getTitleText() {
        return R.string.forge_dl_select_version;
    }

    @Override
    public int getNoDataMsg() {
        return R.string.forge_dl_no_installer;
    }

    @Override
    public List<String> loadVersionList() throws IOException {
        return ForgeUtils.downloadForgeVersions();
    }

    @Override
    public ExpandableListAdapter createAdapter(List<String> versionList, LayoutInflater layoutInflater) {
        return new ForgeVersionListAdapter(versionList, layoutInflater);
    }

    @Override
    public Runnable createDownloadTask(Object selectedVersion, ModloaderListenerProxy listenerProxy) {
        return new ForgeDownloadTask(listenerProxy, (String) selectedVersion);
    }

    @Override
    public void onDownloadFinished(Context context, File downloadedFile) {
        // The AWT-based GUI installer that used to run this jar has been removed.
        // Tell the user that this path is not available (yet).
        Toast.makeText(context, R.string.gui_installation_unavailable, Toast.LENGTH_LONG).show();
    }
}
