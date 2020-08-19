//
// Created by 王龙 on 2020-01-26.
//

#ifndef FFMPEGRTMP_VIDEOCHANNEL_H
#define FFMPEGRTMP_VIDEOCHANNEL_H


#include "BaseChannel.h"

#include "LogUtils.h"
#include "GlobalContexts.h"
#include "EGLDisplayYUV.h"
#include "ShaderYUV.h"

extern "C" {
    #include <libswscale/swscale.h>
    #include <libavutil/frame.h>
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
};

typedef void (*RenderCallback) (uint8_t * , int , int, int);

class VideoChannel : public BaseChannel {
public:
    VideoChannel(int streamIndex, AVCodecContext *pContext, GlobalContexts *global_context);
    ~VideoChannel();

    void start();

    void stop();

    void pause();

    void video_decode();

    void video_player();

    void setRenderCallback(RenderCallback renderCallback);
    void adjustVideoScaleResolution();
    void setTimeBase(AVRational base);
    void setAvgFrameRate(AVRational rate);
    void setReferenceClock(double * value);
private:
    void avFrameDataToYUVData(AVFrame *frame, uint8_t *dst_data[]);
    void avFrameDataToYUVDataScale(uint8_t *src_data[], int src_width, int src_height,
                                   uint8_t *dst_data[], int line_size[]);

    pthread_t pid_video_decode;
    pthread_t pid_video_player;
    RenderCallback renderCallback;
    AVRational time_base;
    AVRational avg_frame_rate;
    double *clock = 0;
    GlobalContexts *global_context = 0;
    EGLDisplayYUV *eglDisplayYuv = NULL;
    ShaderYUV * shaderYuv = NULL;
    uint8_t *yuv_data[3];
    uint16_t video_width;
    uint16_t video_height;
    uint16_t scale_video_width;
    uint16_t scale_video_height;
};


#endif //FFMPEGRTMP_VIDEOCHANNEL_H
