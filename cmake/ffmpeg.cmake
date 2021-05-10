
if(APPLE)
    MESSAGE(STATUS “This is APPLE.”)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
elseif(UNIX)
    MESSAGE(STATUS “This is Linux.”)
    include_directories(${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/include)
    link_directories(${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/libcentos)
endif()

set(FFMPEG_LIBRARIES avformat avfilter avcodec avutil swresample swscale z bz2 pthread)