//
// Created by maks on 24.09.2022.
//

#include <stdlib.h>
#include <android/log.h>
#include <assert.h>
#include <string.h>
#include "environ.h"
#define TAG __FILE_NAME__
#include <log.h>

struct eclipse_environ_s *eclipse_environ;
__attribute__((constructor)) void env_init() {
    char* strptr_env = getenv("ECLIPSE_ENVIRON");
    if(strptr_env == NULL) {
        LOGI("No environ found, creating...");
        eclipse_environ = malloc(sizeof(struct eclipse_environ_s));
        assert(eclipse_environ);
        memset(eclipse_environ, 0 , sizeof(struct eclipse_environ_s));
        if(asprintf(&strptr_env, "%p", eclipse_environ) == -1) abort();
        setenv("ECLIPSE_ENVIRON", strptr_env, 1);
        free(strptr_env);
    }else{
        LOGI("Found existing environ: %s", strptr_env);
        eclipse_environ = (void*) strtoul(strptr_env, NULL, 0x10);
    }
    LOGI("%p", eclipse_environ);
}