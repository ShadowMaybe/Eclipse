#!/usr/bin/env python3
"""Eclipse Launcher identity codemod.

Applies ordered, most-specific-first text substitutions to every text file in
the repo, then renames files and directories to match.  Ordering matters: the
JNI package form (`net_kdt_pojavlaunch`) and the include-guard prefix
(`POJAVLAUNCHER_`) must be handled before their generic parents, and the
JitPack coordinates under `com.github.PojavLauncherTeam` are protected because
renaming them would break dependency resolution.
"""
import os
import sys
import shutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ---------------------------------------------------------------- protections
# JitPack coordinates that MUST survive the sweep verbatim.
PROTECT = [
    ('com.github.PojavLauncherTeam:portrait-sdp', '@@PROT_SD@@'),
    ('com.github.PojavLauncherTeam:portrait-ssp', '@@PROT_SS@@'),
]
UNPROTECT = [(v, k) for k, v in PROTECT]

# ------------------------------------------------- pre-swept attribution fixes
# Comments/URLs that name another project; rewritten so the generic sweep
# below cannot turn them into links that point at a repo which doesn't exist.
PREFIX = [
    # LWJGL's own source, which this hook derives from - point at upstream
    # rather than at the fork it was read from.
    ('https://github.com/PojavLauncherTeam/lwjgl3/blob/3.3.1/modules/lwjgl/'
     'core/src/generated/c/linux/org_lwjgl_system_linux_DynamicLinkLoader.c',
     'https://github.com/LWJGL/lwjgl3/blob/3.3.1/modules/lwjgl/core/src/'
     'generated/c/linux/org_lwjgl_system_linux_DynamicLinkLoader.c'),
    ('https://github.com/PojavLauncherTeam/lwjgl3/blob/fix_huawei_hang/'
     'modules/lwjgl/core/src/main/java/org/lwjgl/system/SharedLibraryUtil.java',
     'https://github.com/LWJGL/lwjgl3/blob/master/modules/lwjgl/core/src/main/'
     'java/org/lwjgl/system/SharedLibraryUtil.java'),
    ('Please notify PojavLauncherTeam if you need that feature',
     'Please report this to the Eclipse Launcher maintainers if you need that '
     'feature'),
    ("// Our version of exp4j can be built from source at",
     "// exp4j is vendored as a prebuilt snapshot in libs/"),
    ("// https://github.com/PojavLauncherTeam/exp4j", "// math expression "
     "evaluation"),
]

# ------------------------------------------------------------------ the sweep
RENAME = [
    # Java package - dot form (imports, manifest, gradle, resource refs)
    ('net.kdt.pojavlaunch', 'me.shadow.eclipselauncher'),
    # Java package - underscore form (JNI symbol prefix); MUST precede generic
    ('net_kdt_pojavlaunch', 'me_shadow_eclipselauncher'),
    # Classes renamed by hand below (file names follow these)
    ('PojavRendererInit', 'EclipseRendererInit'),
    ('PojavApplication', 'EclipseApplication'),
    ('PojavProfile', 'EclipseProfile'),
    ('PojavCrashReport', 'EclipseCrashReport'),
    ('getPojavStorageRoot', 'getEclipseStorageRoot'),
    # C include guards are POJAVLAUNCHER_*_H - distinct from POJAV_ env vars
    ('POJAVLAUNCHER_', 'ECLIPSELAUNCHER_'),
    # runtime environment variables read by our own jni/ sources
    ('POJAVEXEC_EGL', 'ECLIPSE_EGL'),
    ('POJAV_', 'ECLIPSE_'),
    # shared library module name (Android.mk + System.loadLibrary)
    ('pojavexec', 'eclipsebridge'),
    # Gradle module directory
    ('app_pojavlauncher', 'app_eclipselauncher'),
    # storage path and profile scheme
    ('games/PojavLauncher', 'games/EclipseLauncher'),
    ('pojav://', 'eclipse://'),
    # generic branding
    ('PojavLauncher', 'EclipseLauncher'),
    ('Pojavlauncher', 'EclipseLauncher'),
    ('Pojav', 'Eclipse'),
    ('POJAV', 'ECLIPSE'),
    ('pojav', 'eclipse'),
]

# content we rewrite by hand rather than transform
SKIP_TEXT = {
    'README.md', 'LICENSE', 'dataremoval.md', 'crowdin.yml',
    os.path.join('app_pojavlauncher', 'src', 'main', 'assets', 'about_en.txt'),
    os.path.join('.github', 'FUNDING.yml'),
    os.path.join('.github', 'ISSUE_TEMPLATE', 'bug_report.yml'),
    os.path.join('.github', 'workflows', 'android.yml'),
    # this file *contains* every needle above as a literal - sweeping it would
    # rewrite its own rename table
    os.path.join('scripts', '_codemod.py'),
}
SKIP_EXT = {
    '.so', '.jar', '.png', '.webp', '.ttf', '.otf', '.jpg', '.jpeg', '.gif',
    '.zip', '.aar', '.dex', '.class', '.keystore', '.jks', '.xcassets',
    '.webp', '.ico', '.mp3', '.ogg', '.ttc',
}

# ------------------------------------------------------------ file/dir renames
# deepest first: every rename is applied to the path as it exists right then
RENAMES = [
    ('app_pojavlauncher/src/main/java/net/kdt/pojavlaunch/PojavApplication.java',
     'app_pojavlauncher/src/main/java/net/kdt/pojavlaunch/EclipseApplication.java'),
    ('app_pojavlauncher/src/main/java/net/kdt/pojavlaunch/PojavProfile.java',
     'app_pojavlauncher/src/main/java/net/kdt/pojavlaunch/EclipseProfile.java'),
    ('jre_lwjgl3glfw/src/main/java/org/lwjgl/opengl/PojavRendererInit.java',
     'jre_lwjgl3glfw/src/main/java/org/lwjgl/opengl/EclipseRendererInit.java'),
    ('app_pojavlauncher/src/main/assets/pojavlauncher.png',
     'app_pojavlauncher/src/main/assets/eclipselauncher.png'),
    ('app_pojavlauncher/src/main/assets/pojavtext.png',
     'app_pojavlauncher/src/main/assets/eclipsetext.png'),
    ('app_pojavlauncher/src/main/res/drawable/ic_pojav_full.webp',
     'app_pojavlauncher/src/main/res/drawable/ic_eclipse_full.webp'),
    ('app_pojavlauncher/src/main/res/layout/activity_pojav_launcher.xml',
     'app_pojavlauncher/src/main/res/layout/activity_eclipse_launcher.xml'),
    ('app_pojavlauncher/src/main/res/layout-land/activity_pojav_launcher.xml',
     'app_pojavlauncher/src/main/res/layout-land/activity_eclipse_launcher.xml'),
    ('app_pojavlauncher/src/main/java/net/kdt/pojavlaunch',
     'app_pojavlauncher/src/main/java/me/shadow/eclipselauncher'),
    # module directory last - all text already refers to the new name
    ('app_pojavlauncher', 'app_eclipselauncher'),
]


def is_text_file(path):
    if os.path.splitext(path)[1].lower() in SKIP_EXT:
        return False
    try:
        with open(path, 'rb') as fh:
            fh.read()
        with open(path, 'r', encoding='utf-8') as fh:
            fh.read()
        return True
    except (UnicodeDecodeError, IsADirectoryError, PermissionError):
        return False


def sweep():
    changed, skipped = [], []
    for dp, dns, fns in os.walk(ROOT):
        dns[:] = [d for d in dns if d != '.git']
        for fn in fns:
            full = os.path.join(dp, fn)
            rel = os.path.relpath(full, ROOT)
            if rel in SKIP_TEXT:
                skipped.append(rel)
                continue
            if not is_text_file(full):
                continue
            with open(full, 'r', encoding='utf-8') as fh:
                orig = fh.read()
            s = orig
            for k, v in PROTECT:
                s = s.replace(k, v)
            for k, v in PREFIX:
                s = s.replace(k, v)
            for k, v in RENAME:
                s = s.replace(k, v)
            for k, v in UNPROTECT:
                s = s.replace(k, v)
            if s != orig:
                with open(full, 'w', encoding='utf-8') as fh:
                    fh.write(s)
                changed.append(rel)
    return changed, skipped


def rename():
    done = []
    missing = []
    for old, new in RENAMES:
        op, np = os.path.join(ROOT, old), os.path.join(ROOT, new)
        if not os.path.exists(op):
            missing.append(old)
            continue
        os.makedirs(os.path.dirname(np), exist_ok=True)
        shutil.move(op, np)
        done.append((old, new))
    return done, missing


def main():
    changed, skipped = sweep()
    print(f"swept: {len(changed)} file(s) changed; {len(skipped)} skipped "
          f"(hand-rewritten): {', '.join(sorted(skipped))}")
    done, missing = rename()
    print(f"renamed: {len(done)} path(s)")
    if missing:
        print(f"MISSING (already renamed or never existed): {missing}")
    # leftover java package dirs under net/kdt (only pojavlaunch expected)
    return 0


if __name__ == '__main__':
    sys.exit(main())
