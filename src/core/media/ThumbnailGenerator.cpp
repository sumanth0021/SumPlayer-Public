#include "core/media/ThumbnailGenerator.h"
#include <iostream>

#include <QImage>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

namespace SumPlayer
{
    bool ThumbnailGenerator::generate(const std::string& videoPath,
                                       const std::string& outputPath)
    {
        AVFormatContext* formatCtx = nullptr;

        if (avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr) < 0)
        {
            std::cout << "[ThumbnailGenerator] Could not open: " << videoPath << std::endl;
            return false;
        }

        if (avformat_find_stream_info(formatCtx, nullptr) < 0)
        {
            avformat_close_input(&formatCtx);
            return false;
        }

        int videoStreamIndex = av_find_best_stream(
            formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

        if (videoStreamIndex < 0)
        {
            avformat_close_input(&formatCtx);
            return false;
        }

        AVStream* stream = formatCtx->streams[videoStreamIndex];
        AVCodecParameters* params = stream->codecpar;

        const AVCodec* codec = avcodec_find_decoder(params->codec_id);
        if (!codec)
        {
            avformat_close_input(&formatCtx);
            return false;
        }

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx)
        {
            avformat_close_input(&formatCtx);
            return false;
        }

        avcodec_parameters_to_context(codecCtx, params);

        if (avcodec_open2(codecCtx, codec, nullptr) < 0)
        {
            avcodec_free_context(&codecCtx);
            avformat_close_input(&formatCtx);
            return false;
        }

        // Jump in a bit so the thumbnail is not just a black intro frame.
        int64_t seekTargetSeconds = 12;
        AVRational streamTimeBase = stream->time_base;
        int64_t seekTarget = av_rescale_q(
            seekTargetSeconds * AV_TIME_BASE,
            AVRational{1, AV_TIME_BASE},
            streamTimeBase
        );

        av_seek_frame(formatCtx, videoStreamIndex, seekTarget, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codecCtx);

        AVPacket* packet = av_packet_alloc();
        AVFrame*  frame  = av_frame_alloc();
        bool gotFrame = false;

        while (av_read_frame(formatCtx, packet) >= 0 && !gotFrame)
        {
            if (packet->stream_index == videoStreamIndex)
            {
                if (avcodec_send_packet(codecCtx, packet) >= 0)
                {
                    if (avcodec_receive_frame(codecCtx, frame) >= 0)
                    {
                        gotFrame = true;
                    }
                }
            }
            av_packet_unref(packet);
        }

        bool success = false;

        if (gotFrame)
        {
            // Qt wants RGB pixels; FFmpeg might hand us basically any video format.
            const int thumbWidth  = 480;
            const int thumbHeight = 270;

            SwsContext* swsCtx = sws_getContext(
                codecCtx->width, codecCtx->height, (AVPixelFormat)frame->format,
                thumbWidth, thumbHeight, AV_PIX_FMT_RGB24,
                SWS_LANCZOS, nullptr, nullptr, nullptr
            );

            if (swsCtx)
            {
                AVFrame* rgbFrame = av_frame_alloc();
                int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, thumbWidth, thumbHeight, 1);
                uint8_t* buffer = (uint8_t*)av_malloc(bufSize);

                av_image_fill_arrays(
                    rgbFrame->data, rgbFrame->linesize,
                    buffer, AV_PIX_FMT_RGB24, thumbWidth, thumbHeight, 1
                );

                sws_scale(
                    swsCtx,
                    frame->data, frame->linesize,
                    0, codecCtx->height,
                    rgbFrame->data, rgbFrame->linesize
                );

                QImage image(rgbFrame->data[0], thumbWidth, thumbHeight,
                             rgbFrame->linesize[0], QImage::Format_RGB888);
                success = image.save(QString::fromStdString(outputPath), "JPG",92);

                av_free(buffer);
                av_frame_free(&rgbFrame);
                sws_freeContext(swsCtx);
            }
        }

        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);

        return success;
    }
}
