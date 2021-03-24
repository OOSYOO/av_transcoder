#include <iostream>
#include "ffmpeg_common.h"


int main(int argc, char **argv) 
{
    int ret = 0;
    AVFormatContext *pFormatCtx = nullptr;


    pFormatCtx = avformat_alloc_context();
    if(nullptr == pFormatCtx)
    {
        fprintf(stderr, "alloc context failed!\n");
        return -1;
    }

    ret = avformat_open_input(&pFormatCtx, argv[1], NULL, NULL);
    if(ret)
    {
        fprintf(stderr, "open input stream failed!\n");
        return -1;   
    }

    av_dump_format(pFormatCtx, 0, argv[1], 0);

    avformat_free_context(pFormatCtx);

    return 0;
}
