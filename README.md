<p align="center">
  <img src="app_eclipselauncher/src/main/assets/eclipselauncher.png" width="130" height="150" alt="Eclipse Launcher logo">
</p>

<h1 align="center">Eclipse Launcher</h1>

<p align="center">
  Play Minecraft: Java Edition on Android.<br>
  <b>v1.0</b> · by <b>Shadow</b>
</p>

<p align="center">
  <a href="https://github.com/ShadowMaybe/Eclipse/releases"><img alt="Latest release" src="https://img.shields.io/github/v/release/ShadowMaybe/Eclipse?include_prereleases&label=release"></a>
  <a href="https://github.com/ShadowMaybe/Eclipse/actions"><img alt="Build" src="https://img.shields.io/github/actions/workflow/status/ShadowMaybe/Eclipse/android.yml?branch=main&label=build"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/badge/license-LGPLv3-blue"></a>
</p>

---

## Downloads

Grab a build from the **[Releases page](https://github.com/ShadowMaybe/Eclipse/releases)**.

Every build is published as a GitHub Release asset with a `SHA256SUMS.txt`:

| Channel | Tag | Meaning |
|---|---|---|
| Stable | `v1.0` | Tagged release |
| Rolling | `nightly` | Latest `main` build, marked prerelease |

CI deliberately publishes to **Releases** rather than Actions artifacts, so builds
are not subject to artifact storage limits.

---

## Building

No local toolchain setup is required — **everything builds in GitHub Actions**.

1. Push a tag to cut a stable release:
   ```bash
   git tag v1.0
   git push origin v1.0
   ```
2. Or push to `main` to update the `nightly` prerelease.

The workflow (`/.github/workflows/android.yml`) will:

* fetch the JRE bundles from [`ShadowMaybe/JRE-Builds-Android`](https://github.com/ShadowMaybe/JRE-Builds-Android/releases)
* build the LWJGL component jar (`:jre_lwjgl3glfw:build`)
* build the launcher (`:app_eclipselauncher:assembleRelease`)
* publish the APK and its checksums to a Release

### Local build (optional)

```bash
./gradlew :app_eclipselauncher:assembleRelease
```

You need JDK 17+, Gradle 8.11 and the Android SDK (AGP 8.7.2, `compileSdk 34`).

### Signing

Release builds sign with the committed `debug.keystore` by default so a plain
build always yields an installable APK. To sign with your own key, set these
repository secrets (or environment variables locally):

| Variable | Purpose |
|---|---|
| `ECLIPSE_KEYSTORE` | Path to your `.jks` / `.keystore` |
| `ECLIPSE_KEYSTORE_PASSWORD` | Keystore password |
| `ECLIPSE_KEY_ALIAS` | Key alias |
| `ECLIPSE_KEY_PASSWORD` | Key password |

---

## Project layout

| Path | What it is |
|---|---|
| `app_eclipselauncher/` | The launcher app (`me.shadow.eclipselauncher`) |
| `app_eclipselauncher/src/main/jni/` | Launcher native code (EGL/GL bridge, input, hooks) |
| `app_eclipselauncher/src/main/jniLibs/` | Prebuilt renderer/audio natives |
| `jre_lwjgl3glfw/` | LWJGL classes patched for Android, packaged into the app assets |
| `scripts/` | Build/CI helpers |

### Related repositories

| Repository | Purpose |
|---|---|
| [`JRE-Builds-Android`](https://github.com/ShadowMaybe/JRE-Builds-Android) | OpenJDK 8/17/21/25 bundles for Android |
| [`eclipseglfw`](https://github.com/ShadowMaybe/eclipseglfw) | Window, surface and input shim |
| [`eclipseexec`](https://github.com/ShadowMaybe/eclipseexec) | Native graphics bootstrap |
| [`eclipsesdl`](https://github.com/ShadowMaybe/eclipsesdl) | SDL 3.4.16 adapted for Android |

---

## Known gaps in v1.0

* **Forge and OptiFine in-app installation is not yet available.** The
  AWT/Caciocavallo subsystem (which rendered those installers' Swing UIs) has
  been removed. Fabric, Quilt, modpacks and BTA install normally.
  A headless installer that reads `version.json` out of the installer jar is
  planned for a follow-up.
* The **LTW** renderer is built at CI time from a pinned
  [`MojoLauncher/LTW`](https://github.com/MojoLauncher/LTW) revision instead of being
  fetched from the archived upstream release. It will be replaced by **Cobalt** in a
  follow-up.
* `libgl4es_114.so` will be rebranded to **OmniGL** in a follow-up.

---

## Credits and third-party components

Eclipse Launcher is a derivative work of
[**PojavLauncher**](https://github.com/PojavLauncherTeam/PojavLauncher) (LGPLv3),
which in turn derives from **Boardwalk**. Substantial parts of this codebase
originate there; see [`LICENSE`](LICENSE).

Third-party components and their licenses:

| Component | License |
|---|---|
| [GL4ES](https://github.com/ptitSeb/gl4es) | MIT |
| [Mesa / OSMesa](https://gitlab.freedesktop.org/mesa/mesa) | MIT |
| [OpenJDK](https://openjdk.java.net/legal/gplv2+ce.html) | GPLv2 + CE |
| [LWJGL 3](https://github.com/LWJGL/lwjgl3) | BSD-3 |
| [LTW (Large Thin Wrapper)](https://github.com/MojoLauncher/LTW) | LGPL-3.0 |
| [OpenAL Soft](https://github.com/kcat/openal-soft) | LGPL-2.1 |
| [pro-grade](https://github.com/pro-grade/pro-grade) | Apache-2.0 |
| [xz](https://tukaani.org/xz/) | Public domain / BSD-0 |
| [exp4j](https://github.com/fasseg/exp4j) | Apache-2.0 |
| [htmlcleaner](https://htmlcleaner.sourceforge.net/) | BSD |
| [bytehook](https://github.com/bytedance/android-bytehook) | Apache-2.0 |
| [commons-codec](https://commons.apache.org/proper/commons-codec/) | Apache-2.0 |
| [Gson](https://github.com/google/gson) | Apache-2.0 |
| [Exagear Apache Commons](https://github.com/eknal/ExagearApacheCommons) | Apache-2.0 |

Thanks to [MCHeads](https://mc-heads.net) for Minecraft avatars.

Not affiliated with Minecraft, Mojang or Microsoft.
