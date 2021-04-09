#include <iostream>
#include <vector>
#include <unistd.h>
#include "ffmpeg_common.h"


static int decode_packet(AVCodecContext *dec, const AVPacket *pkt, AVFrame *frame)
{
    int ret = 0;

    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }

    // get all the available frames from the decoder
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec, frame);
        if (ret < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;

            fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
            return ret;
        }

        av_frame_unref(frame);
        if (ret < 0)
            return ret;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    AVFormatContext *pFormatCtx = nullptr;
    std::vector<AVCodec *> codecs_;

    // 申请ctx
    pFormatCtx = avformat_alloc_context();
    if (nullptr == pFormatCtx)
    {
        fprintf(stderr, "alloc context failed!\n");
        return -1;
    }

    // 打开流并读取头，没打开codec
    ret = avformat_open_input(&pFormatCtx, argv[1], NULL, NULL);
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
        AVMediaType media_type = st->codecpar->codec_type;
        if (media_type == AVMEDIA_TYPE_VIDEO)
        {
            printf("stream[%d] is video!\n", i);
        }
        else if (media_type == AVMEDIA_TYPE_AUDIO)
        {
            printf("stream[%d] is audio!\n", i);
            // continue;
        }
        // 找到与codec_id对应的已经注册过的解码器
        AVCodec *local_codec = avcodec_find_decoder(st->codecpar->codec_id);
        if(NULL == local_codec)
        {
            fprintf(stderr, "avcodec_find_decoder failed!\n");
            return -1;
        }
        codecs_.push_back(local_codec);
        av_dump_format(pFormatCtx, i, argv[1], 0);
    }

    AVCodecContext *dec_ctx = avcodec_alloc_context3(codecs_[0]);
    if (NULL == dec_ctx)
    {
        fprintf(stderr, "avcodec_alloc_context3 failed!\n");
        return -1;
    }

    ret = avcodec_parameters_to_context(dec_ctx, pFormatCtx->streams[0]->codecpar);
    if (ret)
    {
        fprintf(stderr, "avcodec_parameters_to_context failed!\n");
        return -1;
    }

    // 初始化dec_ctx
    ret = avcodec_open2(dec_ctx, codecs_[0], NULL);
    if (ret)
    {
        fprintf(stderr, "avcodec_open2 failed!\n");
        return -1;
    }

    //解码
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    int frame_cnt = 0;
    while(av_read_frame(pFormatCtx, packet) >= 0)
    {
        if(packet->stream_index != 0)
            continue;

        ret = decode_packet(dec_ctx, packet, frame);
        av_packet_unref(packet);
        if (ret < 0)
            break;

        frame_cnt++;
    }

    printf("total frame : %d\n", frame_cnt);

    av_packet_free(&packet);
    av_frame_free(&frame);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);

    return 0;
}
