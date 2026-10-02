/*
 * V3 input bridge implementation.
 *
 * Status:
 * - Active development
 * - Works with some bugs:
 *  + Modded versions gives broken stuff..
 *
 * 
 * - Implements glfwSetCursorPos() to handle grab camera pos correctly.
 */
 
#include <assert.h>
#include <dlfcn.h>
#include <jni.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <math.h>

#define TAG __FILE_NAME__
#include "log.h"
#include "utils.h"
#include "environ/environ.h"
#include "jvm_hooks/jvm_hooks.h"

#define EVENT_TYPE_CHAR 1000
#define EVENT_TYPE_CHAR_MODS 1001
#define EVENT_TYPE_CURSOR_ENTER 1002
#define EVENT_TYPE_KEY 1005
#define EVENT_TYPE_MOUSE_BUTTON 1006
#define EVENT_TYPE_SCROLL 1007

#define TRY_ATTACH_ENV(env_name, vm, error_message, then) JNIEnv* env_name;\
do {                                                                       \
    env_name = get_attached_env(vm);                                       \
    if(env_name == NULL) {                                                 \
        printf(error_message);                                             \
        then                                                               \
    }                                                                      \
} while(0)

static void registerFunctions(JNIEnv *env);

jint JNI_OnLoad(JavaVM* vm, __attribute__((unused)) void* reserved) {
    if (eclipse_environ->dalvikJavaVMPtr == NULL) {
        LOGI("Saving DVM environ...");
        //Save dalvik global JavaVM pointer
        eclipse_environ->dalvikJavaVMPtr = vm;
        JNIEnv *dvEnv;
        (*vm)->GetEnv(vm, (void**) &dvEnv, JNI_VERSION_1_4);
        eclipse_environ->bridgeClazz = (*dvEnv)->NewGlobalRef(dvEnv,(*dvEnv) ->FindClass(dvEnv,"org/lwjgl/glfw/CallbackBridge"));
        eclipse_environ->method_accessAndroidClipboard = (*dvEnv)->GetStaticMethodID(dvEnv, eclipse_environ->bridgeClazz, "accessAndroidClipboard", "(ILjava/lang/String;)Ljava/lang/String;");
        eclipse_environ->method_onGrabStateChanged = (*dvEnv)->GetStaticMethodID(dvEnv, eclipse_environ->bridgeClazz, "onGrabStateChanged", "(Z)V");
        eclipse_environ->method_onDirectInputEnable = (*dvEnv)->GetStaticMethodID(dvEnv, eclipse_environ->bridgeClazz, "onDirectInputEnable", "()V");
        eclipse_environ->isUseStackQueueCall = JNI_FALSE;
    } else if (eclipse_environ->dalvikJavaVMPtr != vm) {
        LOGI("Saving JVM environ...");
        eclipse_environ->runtimeJavaVMPtr = vm;
        JNIEnv *vmEnv;
        (*vm)->GetEnv(vm, (void**) &vmEnv, JNI_VERSION_1_4);
        eclipse_environ->vmGlfwClass = (*vmEnv)->NewGlobalRef(vmEnv, (*vmEnv)->FindClass(vmEnv, "org/lwjgl/glfw/GLFW"));
        eclipse_environ->method_glftSetWindowAttrib = (*vmEnv)->GetStaticMethodID(vmEnv, eclipse_environ->vmGlfwClass, "glfwSetWindowAttrib", "(JII)V");
        eclipse_environ->method_internalWindowSizeChanged = (*vmEnv)->GetStaticMethodID(vmEnv, eclipse_environ->vmGlfwClass, "internalWindowSizeChanged", "(J)V");
        eclipse_environ->method_internalChangeMonitorSize = (*vmEnv)->GetStaticMethodID(vmEnv, eclipse_environ->vmGlfwClass, "internalChangeMonitorSize", "(II)V");
        jfieldID field_keyDownBuffer = (*vmEnv)->GetStaticFieldID(vmEnv, eclipse_environ->vmGlfwClass, "keyDownBuffer", "Ljava/nio/ByteBuffer;");
        jobject keyDownBufferJ = (*vmEnv)->GetStaticObjectField(vmEnv, eclipse_environ->vmGlfwClass, field_keyDownBuffer);
        eclipse_environ->keyDownBuffer = (*vmEnv)->GetDirectBufferAddress(vmEnv, keyDownBufferJ);
        jfieldID field_mouseDownBuffer = (*vmEnv)->GetStaticFieldID(vmEnv, eclipse_environ->vmGlfwClass, "mouseDownBuffer", "Ljava/nio/ByteBuffer;");
        jobject mouseDownBufferJ = (*vmEnv)->GetStaticObjectField(vmEnv, eclipse_environ->vmGlfwClass, field_mouseDownBuffer);
        eclipse_environ->mouseDownBuffer = (*vmEnv)->GetDirectBufferAddress(vmEnv, mouseDownBufferJ);
        hookExec(vmEnv);
        installLwjglDlopenHook(vmEnv);
        installEMUIIteratorMititgation(vmEnv);
    }

    if(eclipse_environ->dalvikJavaVMPtr == vm) {
        //perform in all DVM instances, not only during first ever set up
        JNIEnv *env;
        (*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4);
        registerFunctions(env);
    }
    eclipse_environ->isGrabbing = JNI_FALSE;
    
    return JNI_VERSION_1_4;
}

#define ADD_CALLBACK_WWIN(NAME) \
JNIEXPORT jlong JNICALL Java_org_lwjgl_glfw_GLFW_nglfwSet##NAME##Callback(JNIEnv * env, jclass cls, jlong window, jlong callbackptr) { \
    void** oldCallback = (void**) &eclipse_environ->GLFW_invoke_##NAME; \
    eclipse_environ->GLFW_invoke_##NAME = (GLFW_invoke_##NAME##_func*) (uintptr_t) callbackptr; \
    return (jlong) (uintptr_t) *oldCallback; \
}

ADD_CALLBACK_WWIN(Char)
ADD_CALLBACK_WWIN(CharMods)
ADD_CALLBACK_WWIN(CursorEnter)
ADD_CALLBACK_WWIN(CursorPos)
ADD_CALLBACK_WWIN(Key)
ADD_CALLBACK_WWIN(MouseButton)
ADD_CALLBACK_WWIN(Scroll)

#undef ADD_CALLBACK_WWIN

void updateMonitorSize(int width, int height) {
    (*eclipse_environ->glfwThreadVmEnv)->CallStaticVoidMethod(eclipse_environ->glfwThreadVmEnv, eclipse_environ->vmGlfwClass, eclipse_environ->method_internalChangeMonitorSize, width, height);
}
void updateWindowSize(void* window) {
    (*eclipse_environ->glfwThreadVmEnv)->CallStaticVoidMethod(eclipse_environ->glfwThreadVmEnv, eclipse_environ->vmGlfwClass, eclipse_environ->method_internalWindowSizeChanged, (jlong)window);
}

void eclipsePumpEvents(void* window) {
    if(eclipse_environ->shouldUpdateMouse) {
        eclipse_environ->GLFW_invoke_CursorPos(window, floor(eclipse_environ->cursorX),
                                             floor(eclipse_environ->cursorY));
    }
    if(eclipse_environ->shouldUpdateMonitorSize) {
        updateWindowSize(window);
    }

    size_t index = eclipse_environ->outEventIndex;
    size_t targetIndex = eclipse_environ->outTargetIndex;

    while (targetIndex != index) {
        GLFWInputEvent event = eclipse_environ->events[index];
        switch (event.type) {
            case EVENT_TYPE_CHAR:
                if(eclipse_environ->GLFW_invoke_Char) eclipse_environ->GLFW_invoke_Char(window, event.i1);
                break;
            case EVENT_TYPE_CHAR_MODS:
                if(eclipse_environ->GLFW_invoke_CharMods) eclipse_environ->GLFW_invoke_CharMods(window, event.i1, event.i2);
                break;
            case EVENT_TYPE_KEY:
                if(eclipse_environ->GLFW_invoke_Key) eclipse_environ->GLFW_invoke_Key(window, event.i1, event.i2, event.i3, event.i4);
                break;
            case EVENT_TYPE_MOUSE_BUTTON:
                if(eclipse_environ->GLFW_invoke_MouseButton) eclipse_environ->GLFW_invoke_MouseButton(window, event.i1, event.i2, event.i3);
                break;
            case EVENT_TYPE_SCROLL:
                if(eclipse_environ->GLFW_invoke_Scroll) eclipse_environ->GLFW_invoke_Scroll(window, event.i1, event.i2);
                break;
        }

        index++;
        if (index >= EVENT_WINDOW_SIZE)
            index -= EVENT_WINDOW_SIZE;
    }

    // The out target index is updated by the rewinder
}

/** Prepare the library for sending out callbacks to all windows */
void eclipseStartPumping() {
    size_t counter = atomic_load_explicit(&eclipse_environ->eventCounter, memory_order_acquire);
    size_t index = eclipse_environ->outEventIndex;

    unsigned targetIndex = index + counter;
    if (targetIndex >= EVENT_WINDOW_SIZE)
        targetIndex -= EVENT_WINDOW_SIZE;

    // Only accessed by one unique thread, no need for atomic store
    eclipse_environ->inEventCount = counter;
    eclipse_environ->outTargetIndex = targetIndex;

    //PumpEvents is called for every window, so this logic should be there in order to correctly distribute events to all windows.
    if((eclipse_environ->cLastX != eclipse_environ->cursorX || eclipse_environ->cLastY != eclipse_environ->cursorY) && eclipse_environ->GLFW_invoke_CursorPos) {
        eclipse_environ->cLastX = eclipse_environ->cursorX;
        eclipse_environ->cLastY = eclipse_environ->cursorY;
        eclipse_environ->shouldUpdateMouse = true;
    }
    if(eclipse_environ->shouldUpdateMonitorSize) {
        // Perform a monitor size update here to avoid doing it on every single window
        updateMonitorSize(eclipse_environ->savedWidth, eclipse_environ->savedHeight);
        // Mark the monitor size as consumed (since GLFW was made aware of it)
        eclipse_environ->monitorSizeConsumed = true;
    }
}

/** Prepare the library for the next round of new events */
void eclipseStopPumping() {
    eclipse_environ->outEventIndex = eclipse_environ->outTargetIndex;

    // New events may have arrived while pumping, so remove only the difference before the start and end of execution
    atomic_fetch_sub_explicit(&eclipse_environ->eventCounter, eclipse_environ->inEventCount, memory_order_acquire);
    // Make sure the next frame won't send mouse or monitor updates if it's unnecessary
    eclipse_environ->shouldUpdateMouse = false;
    // Only reset the update flag if the monitor size was consumed by eclipseStartPumping. This
    // will delay the update to next frame if it had occured between eclipseStartPumping and eclipseStopPumping,
    // but it's better than not having it apply at all
    if(eclipse_environ->shouldUpdateMonitorSize && eclipse_environ->monitorSizeConsumed) {
        eclipse_environ->shouldUpdateMonitorSize = false;
        eclipse_environ->monitorSizeConsumed = false;
    }

}

JNIEXPORT void JNICALL
Java_org_lwjgl_glfw_GLFW_nglfwGetCursorPos(JNIEnv *env, __attribute__((unused)) jclass clazz, __attribute__((unused)) jlong window, jobject xpos,
                                          jobject ypos) {
    *(double*)(*env)->GetDirectBufferAddress(env, xpos) = eclipse_environ->cursorX;
    *(double*)(*env)->GetDirectBufferAddress(env, ypos) = eclipse_environ->cursorY;
}

JNIEXPORT void JNICALL JavaCritical_org_lwjgl_glfw_GLFW_nglfwGetCursorPosA(__attribute__((unused)) jlong window, jint lengthx, jdouble* xpos, jint lengthy, jdouble* ypos) {
    *xpos = eclipse_environ->cursorX;
    *ypos = eclipse_environ->cursorY;
}

JNIEXPORT void JNICALL
Java_org_lwjgl_glfw_GLFW_nglfwGetCursorPosA(JNIEnv *env, __attribute__((unused)) jclass clazz, __attribute__((unused)) jlong window,
                                            jdoubleArray xpos, jdoubleArray ypos) {
    (*env)->SetDoubleArrayRegion(env, xpos, 0,1, &eclipse_environ->cursorX);
    (*env)->SetDoubleArrayRegion(env, ypos, 0,1, &eclipse_environ->cursorY);
}

JNIEXPORT void JNICALL JavaCritical_org_lwjgl_glfw_GLFW_glfwSetCursorPos(__attribute__((unused)) jlong window, jdouble xpos,
                                                                         jdouble ypos) {
    eclipse_environ->cLastX = eclipse_environ->cursorX = xpos;
    eclipse_environ->cLastY = eclipse_environ->cursorY = ypos;
}

JNIEXPORT void JNICALL
Java_org_lwjgl_glfw_GLFW_glfwSetCursorPos(__attribute__((unused)) JNIEnv *env, __attribute__((unused)) jclass clazz, __attribute__((unused)) jlong window, jdouble xpos,
                                          jdouble ypos) {
    JavaCritical_org_lwjgl_glfw_GLFW_glfwSetCursorPos(window, xpos, ypos);
}



void sendData(int type, int i1, int i2, int i3, int i4) {
    GLFWInputEvent *event = &eclipse_environ->events[eclipse_environ->inEventIndex];
    event->type = type;
    event->i1 = i1;
    event->i2 = i2;
    event->i3 = i3;
    event->i4 = i4;

    if (++eclipse_environ->inEventIndex >= EVENT_WINDOW_SIZE)
        eclipse_environ->inEventIndex -= EVENT_WINDOW_SIZE;

    atomic_fetch_add_explicit(&eclipse_environ->eventCounter, 1, memory_order_acquire);
}

void critical_set_stackqueue(jboolean use_input_stack_queue) {
    eclipse_environ->isUseStackQueueCall = (int) use_input_stack_queue;
}

void noncritical_set_stackqueue(__attribute__((unused)) JNIEnv *env, __attribute__((unused)) jclass clazz, jboolean use_input_stack_queue) {
    critical_set_stackqueue(use_input_stack_queue);
}

JNIEXPORT jstring JNICALL Java_org_lwjgl_glfw_CallbackBridge_nativeClipboard(JNIEnv* env, __attribute__((unused)) jclass clazz, jint action, jbyteArray copySrc) {
#ifdef DEBUG
    LOGD("Debug: Clipboard access is going on\n", eclipse_environ->isUseStackQueueCall);
#endif

    JNIEnv *dalvikEnv;
    (*eclipse_environ->dalvikJavaVMPtr)->AttachCurrentThread(eclipse_environ->dalvikJavaVMPtr, &dalvikEnv, NULL);
    assert(dalvikEnv != NULL);
    assert(eclipse_environ->bridgeClazz != NULL);

    LOGD("Clipboard: Converting string\n");
    char *copySrcC;
    jstring copyDst = NULL;
    if (copySrc) {
        copySrcC = (char *)((*env)->GetByteArrayElements(env, copySrc, NULL));
        copyDst = (*dalvikEnv)->NewStringUTF(dalvikEnv, copySrcC);
    }

    LOGD("Clipboard: Calling 2nd\n");
    jstring pasteDst = convertStringJVM(dalvikEnv, env, (jstring) (*dalvikEnv)->CallStaticObjectMethod(dalvikEnv, eclipse_environ->bridgeClazz, eclipse_environ->method_accessAndroidClipboard, action, copyDst));

    if (copySrc) {
        (*dalvikEnv)->DeleteLocalRef(dalvikEnv, copyDst);
        (*env)->ReleaseByteArrayElements(env, copySrc, (jbyte *)copySrcC, 0);
    }
    (*eclipse_environ->dalvikJavaVMPtr)->DetachCurrentThread(eclipse_environ->dalvikJavaVMPtr);
    return pasteDst;
}

JNIEXPORT jboolean JNICALL JavaCritical_org_lwjgl_glfw_CallbackBridge_nativeSetInputReady(jboolean inputReady) {
#ifdef DEBUG
    LOGD("Debug: Changing input state, isReady=%d, eclipse_environ->isUseStackQueueCall=%d\n", inputReady, eclipse_environ->isUseStackQueueCall);
#endif
    LOGI("Input ready: %i", inputReady);
    eclipse_environ->isInputReady = inputReady;
    return eclipse_environ->isUseStackQueueCall;
}

JNIEXPORT jboolean JNICALL Java_org_lwjgl_glfw_CallbackBridge_nativeSetInputReady(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jboolean inputReady) {
    return JavaCritical_org_lwjgl_glfw_CallbackBridge_nativeSetInputReady(inputReady);
}

JNIEXPORT void JNICALL Java_org_lwjgl_glfw_CallbackBridge_nativeSetGrabbing(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jboolean grabbing) {
    TRY_ATTACH_ENV(dvm_env, eclipse_environ->dalvikJavaVMPtr, "nativeSetGrabbing failed!\n", return;);
    (*dvm_env)->CallStaticVoidMethod(dvm_env, eclipse_environ->bridgeClazz, eclipse_environ->method_onGrabStateChanged, grabbing);
    eclipse_environ->isGrabbing = grabbing;
}

JNIEXPORT jboolean JNICALL
Java_org_lwjgl_glfw_CallbackBridge_nativeEnableGamepadDirectInput(__attribute__((unused)) JNIEnv *env, __attribute__((unused))  jclass clazz) {
    TRY_ATTACH_ENV(dvm_env, eclipse_environ->dalvikJavaVMPtr, "nativeEnableGamepadDirectInput failed!\n", return JNI_FALSE;);
    (*dvm_env)->CallStaticVoidMethod(dvm_env, eclipse_environ->bridgeClazz, eclipse_environ->method_onDirectInputEnable);
    return JNI_TRUE;
}

jboolean critical_send_char(jchar codepoint) {
    if (eclipse_environ->GLFW_invoke_Char && eclipse_environ->isInputReady) {
        if (eclipse_environ->isUseStackQueueCall) {
            sendData(EVENT_TYPE_CHAR, codepoint, 0, 0, 0);
        } else {
            eclipse_environ->GLFW_invoke_Char((void*) eclipse_environ->showingWindow, (unsigned int) codepoint);
        }
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

jboolean noncritical_send_char(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jchar codepoint) {
    return critical_send_char(codepoint);
}

jboolean critical_send_char_mods(jchar codepoint, jint mods) {
    if (eclipse_environ->GLFW_invoke_CharMods && eclipse_environ->isInputReady) {
        if (eclipse_environ->isUseStackQueueCall) {
            sendData(EVENT_TYPE_CHAR_MODS, (int) codepoint, mods, 0, 0);
        } else {
            eclipse_environ->GLFW_invoke_CharMods((void*) eclipse_environ->showingWindow, codepoint, mods);
        }
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

jboolean noncritical_send_char_mods(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jchar codepoint, jint mods) {
    return critical_send_char_mods(codepoint, mods);
}
/*
JNIEXPORT void JNICALL Java_org_lwjgl_glfw_CallbackBridge_nativeSendCursorEnter(JNIEnv* env, jclass clazz, jint entered) {
    if (eclipse_environ->GLFW_invoke_CursorEnter && eclipse_environ->isInputReady) {
        eclipse_environ->GLFW_invoke_CursorEnter(eclipse_environ->showingWindow, entered);
    }
}
*/

void critical_send_cursor_pos(jfloat x, jfloat y) {
#ifdef DEBUG
    LOGD("Sending cursor position \n");
#endif
    if (eclipse_environ->GLFW_invoke_CursorPos && eclipse_environ->isInputReady) {
#ifdef DEBUG
        LOGD("eclipse_environ->GLFW_invoke_CursorPos && eclipse_environ->isInputReady \n");
#endif
        if (!eclipse_environ->isCursorEntered) {
            if (eclipse_environ->GLFW_invoke_CursorEnter) {
                eclipse_environ->isCursorEntered = true;
                if (eclipse_environ->isUseStackQueueCall) {
                    sendData(EVENT_TYPE_CURSOR_ENTER, 1, 0, 0, 0);
                } else {
                    eclipse_environ->GLFW_invoke_CursorEnter((void*) eclipse_environ->showingWindow, 1);
                }
            } else if (eclipse_environ->isGrabbing) {
                // Some Minecraft versions does not use GLFWCursorEnterCallback
                // This is a smart check, as Minecraft will not in grab mode if already not.
                eclipse_environ->isCursorEntered = true;
            }
        }

        if (!eclipse_environ->isUseStackQueueCall) {
            eclipse_environ->GLFW_invoke_CursorPos((void*) eclipse_environ->showingWindow, (double) (x), (double) (y));
        } else {
            eclipse_environ->cursorX = x;
            eclipse_environ->cursorY = y;
        }
    }
}

void noncritical_send_cursor_pos(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz,  jfloat x, jfloat y) {
    critical_send_cursor_pos(x, y);
}
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
void critical_send_key(jint key, jint scancode, jint action, jint mods) {
    if (eclipse_environ->GLFW_invoke_Key && eclipse_environ->isInputReady) {
        eclipse_environ->keyDownBuffer[max(0, key-31)] = (jbyte) action;
        if (eclipse_environ->isUseStackQueueCall) {
            sendData(EVENT_TYPE_KEY, key, scancode, action, mods);
        } else {
            eclipse_environ->GLFW_invoke_Key((void*) eclipse_environ->showingWindow, key, scancode, action, mods);
        }
    }
}
void noncritical_send_key(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jint key, jint scancode, jint action, jint mods) {
    critical_send_key(key, scancode, action, mods);
}

void critical_send_mouse_button(jint button, jint action, jint mods) {
    if (eclipse_environ->GLFW_invoke_MouseButton && eclipse_environ->isInputReady) {
        eclipse_environ->mouseDownBuffer[max(0, button)] = (jbyte) action;
        if (eclipse_environ->isUseStackQueueCall) {
            sendData(EVENT_TYPE_MOUSE_BUTTON, button, action, mods, 0);
        } else {
            eclipse_environ->GLFW_invoke_MouseButton((void*) eclipse_environ->showingWindow, button, action, mods);
        }
    }
}

void noncritical_send_mouse_button(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jint button, jint action, jint mods) {
    critical_send_mouse_button(button, action, mods);
}

void critical_send_screen_size(jint width, jint height) {
    eclipse_environ->savedWidth = width;
    eclipse_environ->savedHeight = height;
    // Even if there was call to eclipseStartPumping that consumed the size, this call
    // might happen right after it (or right before eclipseStopPumping)
    // So unmark the size as "consumed"
    eclipse_environ->monitorSizeConsumed = false;
    eclipse_environ->shouldUpdateMonitorSize = true;
    // Don't use the direct updates  for screen dimensions.
    // This is done to ensure that we have predictable conditions to correctly call
    // updateMonitorSize() and updateWindowSize() while on the render thread with an attached
    // JNIEnv.
}

void noncritical_send_screen_size(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jint width, jint height) {
    critical_send_screen_size(width, height);
}

void critical_send_scroll(jdouble xoffset, jdouble yoffset) {
    if (eclipse_environ->GLFW_invoke_Scroll && eclipse_environ->isInputReady) {
        if (eclipse_environ->isUseStackQueueCall) {
            sendData(EVENT_TYPE_SCROLL, (int)xoffset, (int)yoffset, 0, 0);
        } else {
            eclipse_environ->GLFW_invoke_Scroll((void*) eclipse_environ->showingWindow, (double) xoffset, (double) yoffset);
        }
    }
}

void noncritical_send_scroll(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jdouble xoffset, jdouble yoffset) {
    critical_send_scroll(xoffset, yoffset);
}


JNIEXPORT void JNICALL Java_org_lwjgl_glfw_GLFW_nglfwSetShowingWindow(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jlong window) {
    eclipse_environ->showingWindow = (jlong) window;
}

JNIEXPORT void JNICALL Java_org_lwjgl_glfw_CallbackBridge_nativeSetWindowAttrib(__attribute__((unused)) JNIEnv* env, __attribute__((unused)) jclass clazz, jint attrib, jint value) {
    // Check for stack queue no longer necessary here as the JVM crash's origin is resolved
    if (!eclipse_environ->showingWindow) {
        // If the window is not shown, there is nothing to do yet.
        return;
    }

    // We cannot use eclipse_environ->runtimeJNIEnvPtr_JRE here because that environment is attached
    // on the thread that loaded eclipsebridge (which is the thread that first references the GLFW class)
    // But this method is only called from the Android UI thread

    // Technically the better solution would be to have a permanently attached env pointer stored
    // in environ for the Android UI thread but this is the only place that uses it
    // (very rarely, only in lifecycle callbacks) so i dont care

    TRY_ATTACH_ENV(jvm_env, eclipse_environ->runtimeJavaVMPtr, "nativeSetWindowAttrib failed: %i", return;);

    (*jvm_env)->CallStaticVoidMethod(
            jvm_env, eclipse_environ->vmGlfwClass,
            eclipse_environ->method_glftSetWindowAttrib,
            (jlong) eclipse_environ->showingWindow, attrib, value
    );

    // Attaching every time is annoying, so stick the attachment to the Android GUI thread around
}
const static JNINativeMethod critical_fcns[] = {
        {"nativeSetUseInputStackQueue", "(Z)V", critical_set_stackqueue},
        {"nativeSendChar", "(C)Z", critical_send_char},
        {"nativeSendCharMods", "(CI)Z", critical_send_char_mods},
        {"nativeSendKey", "(IIII)V", critical_send_key},
        {"nativeSendCursorPos", "(FF)V", critical_send_cursor_pos},
        {"nativeSendMouseButton", "(III)V", critical_send_mouse_button},
        {"nativeSendScroll", "(DD)V", critical_send_scroll},
        {"nativeSendScreenSize", "(II)V", critical_send_screen_size}
};

const static JNINativeMethod noncritical_fcns[] = {
        {"nativeSetUseInputStackQueue", "(Z)V", noncritical_set_stackqueue},
        {"nativeSendChar", "(C)Z", noncritical_send_char},
        {"nativeSendCharMods", "(CI)Z", noncritical_send_char_mods},
        {"nativeSendKey", "(IIII)V", noncritical_send_key},
        {"nativeSendCursorPos", "(FF)V", noncritical_send_cursor_pos},
        {"nativeSendMouseButton", "(III)V", noncritical_send_mouse_button},
        {"nativeSendScroll", "(DD)V", noncritical_send_scroll},
        {"nativeSendScreenSize", "(II)V", noncritical_send_screen_size}
};


static bool criticalNativeAvailable;

void dvm_testCriticalNative(void* arg0, void* arg1, void* arg2, void* arg3) {
    if(arg0 != 0 && arg2 == 0 && arg3 == 0) {
        criticalNativeAvailable = false;
    }else if (arg0 == 0 && arg1 == 0){
        criticalNativeAvailable = true;
    }else {
        criticalNativeAvailable = false; // just to be safe
    }
}

static bool tryCriticalNative(JNIEnv *env) {
    static const JNINativeMethod testJNIMethod[] = {
            { "testCriticalNative", "(II)V", dvm_testCriticalNative}
    };
    jclass criticalNativeTest = (*env)->FindClass(env, "me/shadow/eclipselauncher/CriticalNativeTest");
    if(criticalNativeTest == NULL) {
        LOGD("No CriticalNativeTest class found !");
        (*env)->ExceptionClear(env);
        return false;
    }
    jmethodID criticalNativeTestMethod = (*env)->GetStaticMethodID(env, criticalNativeTest, "invokeTest", "()V");
    (*env)->RegisterNatives(env, criticalNativeTest, testJNIMethod, 1);
    (*env)->CallStaticVoidMethod(env, criticalNativeTest, criticalNativeTestMethod);
    (*env)->UnregisterNatives(env, criticalNativeTest);
    return criticalNativeAvailable;
}

static void registerFunctions(JNIEnv *env) {
    bool use_critical_cc = tryCriticalNative(env);
    jclass bridge_class = (*env)->FindClass(env, "org/lwjgl/glfw/CallbackBridge");
    if(use_critical_cc) {
        LOGI("CriticalNative is available. Enjoy the 4.6x times faster input!");
    }else{
        LOGI("CriticalNative is not available. Upgrade, maybe?");
    }
    (*env)->RegisterNatives(env,
                            bridge_class,
                            use_critical_cc ? critical_fcns : noncritical_fcns,
                            sizeof(critical_fcns)/sizeof(critical_fcns[0]));
}

JNIEXPORT jlong JNICALL
Java_org_lwjgl_glfw_GLFW_internalGetGamepadDataPointer(JNIEnv *env, jclass clazz) {
    return (jlong) &eclipse_environ->gamepadState;
}

JNIEXPORT jobject JNICALL
Java_org_lwjgl_glfw_CallbackBridge_nativeCreateGamepadButtonBuffer(JNIEnv *env, jclass clazz) {
    return (*env)->NewDirectByteBuffer(env, &eclipse_environ->gamepadState.buttons, sizeof(eclipse_environ->gamepadState.buttons));
}

JNIEXPORT jobject JNICALL
Java_org_lwjgl_glfw_CallbackBridge_nativeCreateGamepadAxisBuffer(JNIEnv *env, jclass clazz) {
    return (*env)->NewDirectByteBuffer(env, &eclipse_environ->gamepadState.axes, sizeof(eclipse_environ->gamepadState.axes));
}