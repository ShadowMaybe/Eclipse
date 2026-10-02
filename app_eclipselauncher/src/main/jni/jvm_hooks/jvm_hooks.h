//
// Created by maks on 23.01.2025.
//

#ifndef ECLIPSELAUNCHER_JVM_HOOKS_H
#define ECLIPSELAUNCHER_JVM_HOOKS_H

#include <jni.h>

void installEMUIIteratorMititgation(JNIEnv *env);
void installLwjglDlopenHook(JNIEnv *env);
void hookExec(JNIEnv *env);

#endif //ECLIPSELAUNCHER_JVM_HOOKS_H
