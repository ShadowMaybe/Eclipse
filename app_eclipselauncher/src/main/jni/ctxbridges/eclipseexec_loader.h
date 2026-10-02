//
// Dynamic binding to libeclipseexec.so.
//
// eclipseexec ships inside an AAR, which puts its .so in the APK but gives ndk-build
// nothing to link against - there is no prefab metadata in the artifact and no header
// search path reaches it at build time. So every call has to be resolved at run time,
// and every one of them has to be optional: a build that fetches no AAR, or a device
// where the library will not load, has to keep working on the loader it has today.
//
#ifndef ECLIPSE_EXEC_LOADER_H
#define ECLIPSE_EXEC_LOADER_H

#include <stdbool.h>

/**
 * Load libeclipseexec.so and hand it the renderer facts the environment already carries:
 * where the native libraries are, which EGL library to resolve against, whether this
 * device got Turnip, and how the display is sized. Returns true when the library is up.
 *
 * Idempotent and never fails the caller - on any problem it records why and returns false.
 */
bool eclipseexec_load(void);

/** Move the calling thread onto the fastest CPU in the device. A no-op when unavailable. */
void eclipseexec_pin_render_thread(void);

/**
 * The handle eclipseexec loaded the GL driver through, or NULL when it has none - either
 * because eclipseexec_load() failed or because prepare_egl() did not. The caller then
 * falls back to resolving EGL the way it did before this integration.
 */
void *eclipseexec_egl_handle(void);

/** Eclipseexec's own description of the last failure, or NULL when it recorded none. */
const char *eclipseexec_error(void);

#endif // ECLIPSE_EXEC_LOADER_H
