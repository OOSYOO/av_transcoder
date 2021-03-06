//
// Created by 沈悦 on 2021/5/14.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include "ffmpeg_common.h"
#include "util/util.h"

/*
1.解封装
2.查找stream
3.打开解码器
*/

struct AVInfo
{
    AVCodecContext *avctx;
    AVCodec *codec;
    std::ofstream f;

    // void DestoryResource()
    ~AVInfo()
    {
        avcodec_free_context(&avctx);
        avctx = NULL;
        f.close();
    }
};

static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
            { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
            { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
            { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
            { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
            { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}

static void WriteFrameToYUV(const AVFrame *frame, std::ofstream &ofs)
{
    if (NULL == frame)
        return;

    if (!ofs.is_open())
        return;

    for (int i = 0; i < frame->height; i++)
        ofs.write((char *)frame->data[0] + frame->linesize[0] * i, frame->width);

    for (int i = 0; i < frame->height / 2; i++)
        ofs.write((char *)frame->data[1] + frame->linesize[1] * i, frame->width / 2);

    for (int i = 0; i < frame->height / 2; i++)
        ofs.write((char *)frame->data[2] + frame->linesize[2] * i, frame->width / 2);

}

static void WriteFrameToPCM(const AVCodecContext *dec_ctx, const AVFrame *frame, std::ofstream &ofs)
{
    if (NULL == frame)
        return;

    if (!ofs.is_open())
        return;

    int data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
    if (data_size < 0)
    {
        /* This should not occur, checking just for paranoia */
        fprintf(stderr, "Failed to calculate data size\n");
        exit(1);
    }

    for (int i = 0; i < frame->nb_samples; i++)
        for (int ch = 0; ch < dec_ctx->channels; ch++)
            ofs.write((char *)frame->data[ch] + data_size * i, data_size);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <input_mp4_file> <output_yuv_file>\n");
        return -1;
    }

    const char *input_file = argv[1];
    std::string output_yuv_file = std::string(argv[2]) + ".yuv";
    std::string output_pcm_file = std::string(argv[2]) + ".pcm";
    int ret = 0;

    //解封装
    AVFormatContext *pFormatCtx = NULL;

    //查找解码器
    AVInfo video_info;
    AVInfo audio_info;

    //解码
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;

    std::vector<AVCodec *> codecs_;

    // 解封装
    pFormatCtx = avformat_alloc_context();
    if (nullptr == pFormatCtx)
    {
        fprintf(stderr, "alloc context failed!\n");
        return -1;
    }

    // 打开流并读取头。
    //容器->Stream
    ret = avformat_open_input(&pFormatCtx, input_file, NULL, NULL);
    if (ret)
    {
        fprintf(stderr, "open input stream failed!\n");
        return -1;
    }

    // 读取流信息
    ret = avformat_find_stream_info(pFormatCtx, NULL);
    if (ret)
    {
        fprintf(stderr, "find stream failed!\n");
        return -1;
    }

    for (uint32_t i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        AVStream *st = pFormatCtx->streams[i];

        // 找到与codec_id对应的已经注册过的解码器
        AVCodec *local_codec = avcodec_find_decoder(st->codec->codec_id);
        if (NULL == local_codec)
        {
            fprintf(stderr, "avcodec_find_decoder failed!\n");
            return -1;
        }

        AVMediaType media_type = st->codecpar->codec_type;
        if (media_type == AVMEDIA_TYPE_VIDEO)
        {
            printf("stream[%d] is video!\n", i);
            video_info.codec = local_codec;
            video_info.avctx = avcodec_alloc_context3(local_codec);
            if (NULL == video_info.avctx)
            {
                fprintf(stderr, "avcodec_alloc_context3 failed!\n");
                return -1;
            }

            //同步AVCodecParameters
            ret = avcodec_parameters_to_context(video_info.avctx, pFormatCtx->streams[i]->codecpar);
            if (ret)
            {
                fprintf(stderr, "avcodec_parameters_to_context failed!\n");
                return -1;
            }

            //打开视频解码器
            ret = avcodec_open2(video_info.avctx, video_info.codec, NULL);
            if (ret)
            {
                fprintf(stderr, "avcodec_open2 failed!\n");
                return -1;
            }
        }
        else if (media_type == AVMEDIA_TYPE_AUDIO)
        {
            printf("stream[%d] is audio!\n", i);

            audio_info.codec = local_codec;
            audio_info.avctx = avcodec_alloc_context3(local_codec);
            if (NULL == audio_info.avctx)
            {
                fprintf(stderr, "avcodec_alloc_context3 failed!\n");
                return -1;
            }

            //同步AVCodecParameters
            ret = avcodec_parameters_to_context(audio_info.avctx, pFormatCtx->streams[i]->codecpar);
            if (ret)
            {
                fprintf(stderr, "avcodec_parameters_to_context failed!\n");
                return -1;
            }

            //打开视频解码器
            ret = avcodec_open2(audio_info.avctx, audio_info.codec, NULL);
            if (ret)
            {
                fprintf(stderr, "avcodec_open2 failed!\n");
                return -1;
            }
        }
        av_dump_format(pFormatCtx, i, input_file, 0);
    }

    //解码
    packet = av_packet_alloc();
    frame = av_frame_alloc();

    video_info.f.open(output_yuv_file, std::ios::out | std::ios::trunc);
    audio_info.f.open(output_pcm_file, std::ios::out | std::ios::trunc);

    int frame_cnt = 0;
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        AVCodecContext *avctx = NULL;
        if (packet->stream_index == 0)
        {
            avctx = video_info.avctx;
        }
        else
        {
            avctx = audio_info.avctx;
            // continue;
        }

        ret = avcodec_send_packet(avctx, packet);
        if (ret < 0)
        {
            fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
            return ret;
        }

        // get all the available frames from the decoder
        while (ret >= 0)
        {
            ret = avcodec_receive_frame(avctx, frame);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                break;
            else if (ret < 0)
            {
                fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
                return ret;
            }

            if (packet->stream_index == 0)
            {
                WriteFrameToYUV(frame, video_info.f);
                frame_cnt++;
            }
            else
            {
                WriteFrameToPCM(avctx, frame, audio_info.f);
            }

            // av_packet_unref(packet);
            // av_frame_unref(frame);
        }
    }

#if 1
    enum AVSampleFormat sfmt;
    int n_channels = 0;
    const char *fmt;


    sfmt = audio_info.avctx->sample_fmt;

    if (av_sample_fmt_is_planar(sfmt)) {
        const char *packed = av_get_sample_fmt_name(sfmt);
        printf("Warning: the sample format the decoder produced is planar "
               "(%s). This example will output the first channel only.\n",
               packed ? packed : "?");
        sfmt = av_get_packed_sample_fmt(sfmt);
    }

    n_channels = audio_info.avctx->channels;
    if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
        printf("end...\n");

    printf("Play the output audio file with the command:\n"
           "ffplay -f %s -ac %d -ar %d %s\n",
           fmt, n_channels, audio_info.avctx->sample_rate,
           "out11.pcm");
#endif

    printf("total frame : %d\n", frame_cnt);

    av_packet_free(&packet);
    av_frame_free(&frame);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);

    printf("done!\n");

    return 0;
}
