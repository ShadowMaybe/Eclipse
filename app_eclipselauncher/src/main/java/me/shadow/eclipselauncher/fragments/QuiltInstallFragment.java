package me.shadow.eclipselauncher.fragments;

import me.shadow.eclipselauncher.modloaders.FabriclikeUtils;
import me.shadow.eclipselauncher.modloaders.ModloaderListenerProxy;

public class QuiltInstallFragment extends FabriclikeInstallFragment {

    public static final String TAG = "QuiltInstallFragment";
    private static ModloaderListenerProxy sTaskProxy;

    public QuiltInstallFragment() {
        super(FabriclikeUtils.QUILT_UTILS, TAG);
    }
}
