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

static void EncodeVideo(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   std::ofstream &f)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %d\n", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        printf("Write packet %3d size=%5d)\n", pkt->pts, pkt->size);
        f.write((char*)pkt->data, pkt->size);
        av_packet_unref(pkt);
    }
}


int main(int argc, char **argv)
{
    if (argc < 5)
    {
        fprintf(stderr, "usage: %s <input_yuv_file> <width> <height> <output_mp4_file>\n");
        return -1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[4];
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    int ret = 0;

    //
    std::ifstream f_in; 
    f_in.open(input_file); 
    if (!f_in.is_open())
    {
        fprintf(stderr, "open file failed!\n");
        return -1;
    }

    //查找解码器
    AVInfo video_info;

    //解码
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;

    //设置编码器
    video_info.codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    video_info.avctx = avcodec_alloc_context3(video_info.codec);
    if (NULL == video_info.avctx)
    {
        fprintf(stderr, "avcodec_alloc_context3 failed!\n");
        return -1;
    }

    //设置编码参数
    video_info.avctx->bit_rate = 400000;
    video_info.avctx->width = width;
    video_info.avctx->height = height;
    video_info.avctx->time_base = (AVRational){1, 25};
    video_info.avctx->framerate = (AVRational){25, 1};
    video_info.avctx->gop_size = 10;
    video_info.avctx->max_b_frames = 1;
    video_info.avctx->pix_fmt = AV_PIX_FMT_YUV420P;


    //打开视频解码器
    ret = avcodec_open2(video_info.avctx, video_info.codec, NULL);
    if (ret)
    {
        fprintf(stderr, "avcodec_open2 failed!\n");
        return -1;
    }

    //解码
    packet = av_packet_alloc();
    //只申请结构体内存，不申请data内存
    frame = av_frame_alloc();

    video_info.f.open(output_file, std::ios::out | std::ios::trunc);


    frame->format = video_info.avctx->pix_fmt;
    frame->width = video_info.avctx->width;
    frame->height = video_info.avctx->height;
    //申请data内存
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    size_t data_size = video_info.avctx->width * video_info.avctx->height * 3 / 2;
    uint8_t data[data_size];
    int frame_cnt = 0;
    while(!f_in.eof())
    {
        ret = av_frame_make_writable(frame);
        if(ret < 0)
        {
            fprintf(stderr, "av_frame_make_writable failed!\n");
            exit(1);
        }

        f_in.read((char *)data, data_size);


        uint8_t *tmp_data = data;
        for(int i=0; i<frame->height; i++)
        {
            memcpy(frame->data[0] + frame->linesize[0]*i, tmp_data, frame->width);
            tmp_data += frame->width;
        }

        for(int i=0; i<frame->height/2; i++)
        {
            memcpy(frame->data[1] + frame->linesize[1]*i, tmp_data, frame->width/2);
            tmp_data += frame->width/2;
        }

        for(int i=0; i<frame->height/2; i++)
        {
            memcpy(frame->data[2] + frame->linesize[2]*i, tmp_data, frame->width/2);
            tmp_data += frame->width/2;
        }

        // for (int y = 0; y < video_info.avctx->height; y++) {
        //     for (int x = 0; x < video_info.avctx->width; x++) {
        //         frame->data[0][y * frame->linesize[0] + x] = x + y + frame_cnt * 3;
        //     }
        // }

        // /* Cb and Cr */
        // for (int y = 0; y < video_info.avctx->height/2; y++) {
        //     for (int x = 0; x < video_info.avctx->width/2; x++) {
        //         frame->data[1][y * frame->linesize[1] + x] = 128 + y + frame_cnt * 2;
        //         frame->data[2][y * frame->linesize[2] + x] = 64 + x + frame_cnt * 5;
        //     }
        // }

        frame->pts = frame_cnt;
        frame_cnt++;

        EncodeVideo(video_info.avctx, frame, packet, video_info.f);

        printf("frame_cnt : %d\n", frame_cnt);
    }


    printf("total frame : %d\n", frame_cnt);

    av_packet_free(&packet);
    av_frame_free(&frame);

    printf("done!\n");

    return 0;
}
