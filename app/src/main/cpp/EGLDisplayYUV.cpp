//
// Created by 王龙 on 2020-03-25.
//

#include "EGLDisplayYUV.h"

EGLDisplayYUV::EGLDisplayYUV(ANativeWindow *window, GlobalContexts *context) {
    this->nativeWindow = window;
    this->global_context = context;
}

EGLDisplayYUV::~EGLDisplayYUV() {

}

int EGLDisplayYUV::eglOpen() {
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY ) {
        LOGV("eglGetDisplay failure.");
        return -1;
    }
    LOGV("eglGetDisplay ok");
    this->global_context->eglDisplay = eglDisplay;

    EGLint majorVersion;
    EGLint minorVersion;
    EGLBoolean success = eglInitialize(eglDisplay, &majorVersion,
                                       &minorVersion);
    if (!success) {
        LOGV("eglInitialize failure.");
        return -1;
    }
    LOGV("eglInitialize ok");

    EGLint numConfigs;
    EGLConfig config;
    static const EGLint CONFIG_ATTRIBS[] = { EGL_RED_SIZE, 8,
                                             EGL_GREEN_SIZE, 8,
                                             EGL_BLUE_SIZE, 8,
                                             EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                             EGL_NONE // the end
    };
    success = eglChooseConfig(eglDisplay, CONFIG_ATTRIBS, &config, 1, &numConfigs);
    if (!success) {
        LOGV("eglChooseConfig failure.");
        return -1;
    }
    LOGV("eglChooseConfig ok");

    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext elgContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT,
                                             attribs);
    if (elgContext == EGL_NO_CONTEXT ) {
        LOGV("eglCreateContext failure, error is %d", eglGetError());
        return -1;
    }
    LOGV("eglCreateContext ok");
    this->global_context->eglContext = elgContext;

    EGLint eglFormat;
    success = eglGetConfigAttrib(eglDisplay, config, EGL_NATIVE_VISUAL_ID,
                                 &eglFormat);
    if (!success) {
        LOGV("eglGetConfigAttrib failure.");
        return -1;
    }
    LOGV("eglGetConfigAttrib ok");

    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, config,
                                                   this->nativeWindow, 0);
    if (EGL_NO_SURFACE == eglSurface) {
        LOGV("eglCreateWindowSurface failure.");
        return -1;
    }
    LOGV("eglCreateWindowSurface ok");
    this->global_context->eglSurface = eglSurface;
    return 0;
}

int EGLDisplayYUV::eglClose() {
    EGLBoolean success = eglDestroySurface(this->global_context->eglDisplay, this->global_context->eglSurface);
    if (!success) {
        LOGV("eglDestroySurface failure.");
    }

    success = eglDestroyContext(this->global_context->eglDisplay, this->global_context->eglContext);
    if (!success) {
        LOGV("eglDestroySurface failure.");
    }

    success = eglTerminate(this->global_context->eglDisplay);
    if (!success) {
        LOGV("eglDestroySurface failure.");
    }

    this->global_context->eglSurface = NULL;
    this->global_context->eglContext = NULL;
    this->global_context->eglDisplay = NULL;
    return 0;
}
