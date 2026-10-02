package me.shadow.eclipselauncher.fragments;

import me.shadow.eclipselauncher.modloaders.FabriclikeUtils;
import me.shadow.eclipselauncher.modloaders.ModloaderListenerProxy;

public class FabricInstallFragment extends FabriclikeInstallFragment {

    public static final String TAG = "FabricInstallFragment";

    public FabricInstallFragment() {
        super(FabriclikeUtils.FABRIC_UTILS, TAG);
    }
}
