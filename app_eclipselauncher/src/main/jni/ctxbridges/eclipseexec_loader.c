//
// Dynamic binding to libeclipseexec.so. See eclipseexec_loader.h for why this is dynamic.
//
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eclipseexec.h"        // the vendored contract header - the whole API we bind
#include "eclipseexec_loader.h"

static struct {
    void *lib;

    bool (*prepare_egl)(const char *path, bool use_bypass, bool force_gles, int gles_major);
    void (*set_native_dir)(const char *dir);
    void (*set_display_params)(int width, int height, float hz);
    void (*set_use_turnip)(bool enable);
    void (*set_affinity_enabled)(bool enable);
    void *(*acq_egl_handle)(void);
    const char *(*last_error)(void);
    void (*make_bigcore_affine)(void);

    bool attempted;
    bool ready;
} g_exec;

static const char *env_or(const char *name, const char *fallback) {
    const char *value = getenv(name);
    return (value == NULL || value[0] == '\0') ? fallback : value;
}

bool eclipseexec_load(void) {
    if(g_exec.attempted) return g_exec.ready;
    g_exec.attempted = true;

    g_exec.lib = dlopen("libeclipseexec.so", RTLD_NOW | RTLD_LOCAL);
    if(g_exec.lib == NULL) {
        printf("eclipseexec: libeclipseexec.so not present (%s)\n", dlerror());
        return false;
    }

    // POSIX says a void* -> function pointer cast through dlsym is fine; doing it
    // explicitly per member keeps this a clean C conversion instead of an implicit one.
    g_exec.prepare_egl = (bool (*)(const char *, bool, bool, int))
            dlsym(g_exec.lib, "eclipseexec_prepare_egl");
    g_exec.set_native_dir = (void (*)(const char *))
            dlsym(g_exec.lib, "eclipseexec_set_native_dir");
    g_exec.set_display_params = (void (*)(int, int, float))
            dlsym(g_exec.lib, "eclipseexec_set_display_params");
    g_exec.set_use_turnip = (void (*)(bool))
            dlsym(g_exec.lib, "eclipseexec_set_use_turnip");
    g_exec.set_affinity_enabled = (void (*)(bool))
            dlsym(g_exec.lib, "eclipseexec_set_affinity_enabled");
    g_exec.acq_egl_handle = (void *(*)(void))
            dlsym(g_exec.lib, "eclipseexec_acq_egl_handle");
    g_exec.last_error = (const char *(*)(void))
            dlsym(g_exec.lib, "eclipseexec_last_error");
    g_exec.make_bigcore_affine = (void (*)(void))
            dlsym(g_exec.lib, "eclipseexec_make_bigcore_affine");

    if(g_exec.acq_egl_handle == NULL || g_exec.make_bigcore_affine == NULL) {
        printf("eclipseexec: expected symbols missing, not using it\n");
        dlclose(g_exec.lib);
        g_exec.lib = NULL;
        return false;
    }

    if(g_exec.set_native_dir != NULL) {
        const char *native_dir = env_or("ECLIPSE_NATIVEDIR", NULL);
        if(native_dir != NULL) g_exec.set_native_dir(native_dir);
    }

    if(g_exec.set_display_params != NULL) {
        g_exec.set_display_params(
                (int) strtol(env_or("AWTSTUB_WIDTH", "0"), NULL, 10),
                (int) strtol(env_or("AWTSTUB_HEIGHT", "0"), NULL, 10),
                0.0f); // refresh rate: zero is passed through, the driver keeps its own
    }

    if(g_exec.set_use_turnip != NULL) {
        g_exec.set_use_turnip(getenv("ECLIPSE_LOAD_TURNIP") != NULL);
    }

    // On by default in eclipseexec; ask for it explicitly so a future default change does
    // not silently take the render-thread pin away from us.
    if(g_exec.set_affinity_enabled != NULL) g_exec.set_affinity_enabled(true);

    // EGL. Same expression dlsym_EGL() has always passed to loader_dlopen(): the ECLIPSE_EGL
    // override when the LTW renderer set it, "libEGL.so" otherwise. prepare_egl() has no NULL
    // default - an empty path is an error there - so the fallback has to be a name dlopen
    // can find. use_bypass stays off: the system EGL must load in the default namespace,
    // which is what the launcher already does, and the Turnip bypass is load_vulkan()'s job.
    if(g_exec.prepare_egl != NULL) {
        const char *path = env_or("ECLIPSE_EGL", "libEGL.so");

        // Mirrors gl_init_context()'s eglBindAPI_p() choice: everything except
        // opengles3_desktopgl binds EGL_OPENGL_ES_API. eclipseexec records it in its
        // renderspec so the SDL fork asks the driver for the same kind of context.
        const char *renderer = getenv("ECLIPSE_RENDERER");
        bool force_gles = (renderer == NULL) || strncmp(renderer, "opengles3_desktopgl", 19) != 0;

        int gles_major = 0;
        if(force_gles) {
            const char *libgl_es = getenv("LIBGL_ES");
            if(libgl_es != NULL) {
                char *end = NULL;
                long parsed = strtol(libgl_es, &end, 10);
                if(end != libgl_es && parsed >= 0 && parsed <= 16) gles_major = (int) parsed;
            }
        }

        if(!g_exec.prepare_egl(path, false, force_gles, gles_major)) {
            // Not fatal: dlsym_EGL() falls back to loader_dlopen() and the launch proceeds.
            printf("eclipseexec: prepare_egl(%s) failed: %s\n", path, eclipseexec_error());
        }
    }

    g_exec.ready = true;
    printf("eclipseexec: loaded, EGL path %s, turnip=%s\n",
           env_or("ECLIPSE_EGL", "libEGL.so"),
           getenv("ECLIPSE_LOAD_TURNIP") ? "on" : "off");
    return true;
}

void eclipseexec_pin_render_thread(void) {
    if(!g_exec.ready || g_exec.make_bigcore_affine == NULL) return;
    // eclipseexec dedupes per thread, so the repeat cost is a thread-local test.
    g_exec.make_bigcore_affine();
}

void *eclipseexec_egl_handle(void) {
    if(!g_exec.ready || g_exec.acq_egl_handle == NULL) return NULL;
    return g_exec.acq_egl_handle();
}

const char *eclipseexec_error(void) {
    if(g_exec.lib == NULL || g_exec.last_error == NULL) return NULL;
    return g_exec.last_error();
}
