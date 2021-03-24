include_directories(/usr/local/include)
link_directories(/usr/local/lib)
# include_directories(${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/include)
# link_directories(${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/lib)

set(FFMPEG_LIBRARIES avformat avfilter avcodec avutil)