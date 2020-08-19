#include <jni.h>
#include <string>
#include <pthread.h>
#include <android/native_window_jni.h> // 是为了 渲染到屏幕支持的

#include "LogUtils.h"
#include "FFmpegPlayer.h"
#include "GlobalContexts.h"

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/time.h>
}

const char *  calssName = "com/wl/ffmpegplayer/FFmpegPlayer";

JavaVM * javaVm = NULL;
FFmpegPlayer * player = NULL;
AVCallback * callback = NULL;
ANativeWindow * nativeWindow = NULL;
GlobalContexts *global_context = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化 互斥锁

void preparePlayer(JNIEnv* env, jobject clazz, jstring data);
void startPlayer(JNIEnv* env, jobject clazz);
void stopPlayer(JNIEnv* env, jobject clazz);
void pausePlayer(JNIEnv* env, jobject clazz);
void releasePlayer(JNIEnv* env, jobject clazz);
void setPlayerSurface(JNIEnv* env, jobject clazz, jobject surface);
void setSurfaceSize(JNIEnv* env, jobject clazz, jint width, jint height);

static const JNINativeMethod method_table[] = {// javap -s -p + class文件自动生成函数签名
        {
                "prepareFFmpegPlayer",                    "(Ljava/lang/String;)V",
                (void* )preparePlayer
        },
        {
                "startFFmpegPlayer",                    "()V",
                (void* )startPlayer
        },
        {
                "stopFFmpegPlayer",                    "()V",
                (void* )stopPlayer
        },
        {
                "releaseFFmpegPlayer",                    "()V",
                (void* )releasePlayer
        },
        {
                "pauseFFmpegPlayer",                    "()V",
                (void* )pausePlayer
        },
        {
                "setFFmpegSurface",                    "(Landroid/view/Surface;)V",
                (void* )setPlayerSurface
        },
        {
                "setSurfaceSize",                    "(II)V",
                (void* )setSurfaceSize
        },
};

/**
 * 专门渲染的函数
 * @param src_data 解码后的视频 rgba 数据
 * @param width 宽信息
 * @param height 高信息
 * @param src_liinesize 行数size相关信息
 */
void renderFrame(uint8_t * src_data, int width, int height, int src_liinesize) {
    pthread_mutex_lock(&mutex);

    if (!nativeWindow) {
        pthread_mutex_unlock(&mutex);
    }

    // 设置窗口属性
    ANativeWindow_setBuffersGeometry(nativeWindow, width, height , WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer windowBuffer;
    if (ANativeWindow_lock(nativeWindow, &windowBuffer, 0)) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }

    // 填数据到buffer，其实就是修改数据
    uint8_t * dst_data = static_cast<uint8_t *>(windowBuffer.bits);
    int lineSize = windowBuffer.stride * 4; // RGBA == 4,windowBuffer.stride代表像素点
    // 下面就是逐行Copy了
    for (int i = 0; i < windowBuffer.height; ++i) {
        // 一行Copy
        memcpy(dst_data + i * lineSize, src_data + i * src_liinesize, lineSize);
    }

    ANativeWindow_unlockAndPost(nativeWindow);
    pthread_mutex_unlock(&mutex);
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGD("JNI_OnLoad in");
    javaVm = vm;
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        return result;
    }

    jclass clazz;
    clazz = env->FindClass(calssName);
    if(NULL == clazz) {
        LOGD("OnLoad clazz == null");
        return result;
    }
    if (env->RegisterNatives(clazz, method_table, sizeof(method_table) / sizeof(method_table[0])) < 0) {
        LOGD("OnLoad RegisterNatives fail");
        return result;
    }
    LOGD("JNI_OnLoad out");
    return JNI_VERSION_1_6;
}

void preparePlayer(JNIEnv* env, jobject clazz, jstring data) {
    LOGD("preparePlayer in");
    if(NULL == global_context) {
        global_context = new GlobalContexts();
    }
    callback = new AVCallback(javaVm, env, clazz);
    const char * data_source = env->GetStringUTFChars(data, NULL);
    player = new FFmpegPlayer(data_source, callback, global_context);
    player->setRenderCallback(renderFrame);
    player->prepare();
    env->ReleaseStringUTFChars(data, data_source);
    LOGD("preparePlayer out");
}

void startPlayer(JNIEnv* env, jobject clazz) {
    LOGD("startFFmpegPlayer in");
    if(NULL != player) {
        player->start();
    }
    LOGD("startFFmpegPlayer out");
}

void stopPlayer(JNIEnv* env, jobject clazz) {
    LOGD("stopPlayer in");
    if(NULL != player) {
        player->stop();
    }
    LOGD("stopPlayer out");
}

void releasePlayer(JNIEnv* env, jobject clazz) {
    LOGD("releasePlayer in");
    if(NULL != player) {
        delete player;
        player = NULL;
    }
    if(NULL != callback) {
        delete callback;
        callback = NULL;
    }
    if(NULL != global_context) {
        delete global_context;
        global_context = NULL;
    }
    LOGD("releasePlayer out");
}

void pausePlayer(JNIEnv* env, jobject clazz) {
    LOGD("pausePlayer in");
    if(NULL != player) {
        player->pause();
    }
    LOGD("pausePlayer out");
}

void setPlayerSurface(JNIEnv* env, jobject clazz, jobject surface) {
    LOGD("setPlayerSurface in");
    pthread_mutex_lock(&mutex);

    if (nativeWindow) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = 0;
    }

    // 创建新的窗口用于视频显示
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    if(NULL == global_context) {
        global_context = new GlobalContexts();
    }
    global_context->nativeWindow = nativeWindow;
    pthread_mutex_unlock(&mutex);
    LOGD("setPlayerSurface out");
}

void setSurfaceSize(JNIEnv* env, jobject clazz, jint width, jint height) {
    LOGD("setSurfaceSize in");
    pthread_mutex_lock(&mutex);
    if(NULL == global_context) {
        global_context = new GlobalContexts();
    }
    global_context->gl_window_width = width;
    global_context->gl_window_height = height;
    pthread_mutex_unlock(&mutex);
    LOGD("setSurfaceSize out");
}