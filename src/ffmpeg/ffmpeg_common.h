#pragma once

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libavutil/opt.h>
#include <libavutil/version.h>
#include <libavutil/dict.h>
#include <libavutil/display.h>
#include <libavutil/file.h>

#ifdef __cplusplus
}
#endif //__cplusplus

#undef av_err2str
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
