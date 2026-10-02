package me.shadow.eclipselauncher.fragments;

import android.content.Context;
import android.view.LayoutInflater;
import android.widget.ExpandableListAdapter;
import android.widget.Toast;

import me.shadow.eclipselauncher.R;
import me.shadow.eclipselauncher.modloaders.ModloaderListenerProxy;
import me.shadow.eclipselauncher.modloaders.OptiFineDownloadTask;
import me.shadow.eclipselauncher.modloaders.OptiFineUtils;
import me.shadow.eclipselauncher.modloaders.OptiFineVersionListAdapter;

import java.io.File;
import java.io.IOException;

public class OptiFineInstallFragment extends ModVersionListFragment<OptiFineUtils.OptiFineVersions> {
    public static final String TAG = "OptiFineInstallFragment";
    public OptiFineInstallFragment() {
        super(TAG);
    }
    @Override
    public int getTitleText() {
        return R.string.of_dl_select_version;
    }

    @Override
    public int getNoDataMsg() {
        return R.string.of_dl_failed_to_scrape;
    }
    @Override
    public OptiFineUtils.OptiFineVersions loadVersionList() throws IOException {
        return OptiFineUtils.downloadOptiFineVersions();
    }

    @Override
    public ExpandableListAdapter createAdapter(OptiFineUtils.OptiFineVersions versionList, LayoutInflater layoutInflater) {
        return new OptiFineVersionListAdapter(versionList, layoutInflater);
    }

    @Override
    public Runnable createDownloadTask(Object selectedVersion, ModloaderListenerProxy listenerProxy) {
        return new OptiFineDownloadTask((OptiFineUtils.OptiFineVersion) selectedVersion, listenerProxy, requireActivity());
    }

    @Override
    public void onDownloadFinished(Context context, File downloadedFile) {
        // The AWT-based GUI installer that used to run this jar has been removed.
        // Tell the user that this path is not available (yet).
        Toast.makeText(context, R.string.gui_installation_unavailable, Toast.LENGTH_LONG).show();
    }
}
