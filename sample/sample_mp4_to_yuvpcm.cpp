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

static int DecodePacket(AVCodecContext *dec, const AVPacket *pkt, AVFrame *frame)
{
    int ret = 0;

    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }

    // get all the available frames from the decoder
//    while (ret >= 0) {
        ret = avcodec_receive_frame(dec, frame);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
            return 0;
        else if(ret < 0)
        {
            fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
            return ret;
        }
//        std::cout << frame->linesize[0] << " --  " << frame->linesize[3] << std::endl;

//    }

    return 0;
}

static void WriteFrameToYUV(const AVFrame *frame, std::ofstream &ofs)
{
    if(NULL == frame)
        return;

    if(!ofs.is_open())
        return;

    for(int i=0; i<frame->height; i++)
        ofs.write((char *)frame->data[0] + frame->linesize[0]*i, frame->width);

    for(int i=0; i<frame->height/2; i++)
        ofs.write((char *)frame->data[1] + frame->linesize[1]*i, frame->width/2);

    for(int i=0; i<frame->height/2; i++)
        ofs.write((char *)frame->data[2] + frame->linesize[2]*i, frame->width/2);


//    ofs.write((char *)frame->data[1], frame->linesize[1] * frame->height/2);
//    ofs.write((char *)frame->data[2], frame->linesize[2] * frame->height/2);
}

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        fprintf(stderr, "usage: %s <input_mp4_file> <output_yuv_file>\n");
        return -1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    int ret = 0;

    //解封装
    AVFormatContext *pFormatCtx = NULL;

    //查找解码器
    AVInfo video_info;

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
        AVCodec *local_codec = avcodec_find_decoder(st->codecpar->codec_id);
        if(NULL == local_codec)
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
        }
        else if (media_type == AVMEDIA_TYPE_AUDIO)
        {
            printf("stream[%d] is audio!\n", i);
            // continue;
        }
        av_dump_format(pFormatCtx, i, input_file, 0);
    }


    //打开视频解码器
    ret = avcodec_open2(video_info.avctx, video_info.codec, NULL);
    if (ret)
    {
        fprintf(stderr, "avcodec_open2 failed!\n");
        return -1;
    }

    //解码
    packet = av_packet_alloc();
    frame = av_frame_alloc();

    video_info.f.open(output_file, std::ios::out | std::ios::trunc);

    int frame_cnt = 0;
    while(av_read_frame(pFormatCtx, packet) >= 0)
    {
        if(packet->stream_index != 0)
            continue;

        ret = DecodePacket(video_info.avctx, packet, frame);
        if (ret < 0)
            break;

        WriteFrameToYUV(frame, video_info.f);

        av_packet_unref(packet);
        av_frame_unref(frame);

        frame_cnt++;
    }

    printf("total frame : %d\n", frame_cnt);

    av_packet_free(&packet);
    av_frame_free(&frame);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);

    printf("done!\n");

    return 0;
}

