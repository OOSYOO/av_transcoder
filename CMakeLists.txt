cmake_minimum_required(VERSION 3.16)
project(av_transcoder)

set(CMAKE_CXX_STANDARD 11)


include("cmake/ffmpeg.cmake")

include_directories(${CMAKE_SOURCE_DIR}/src/ffmpeg)

add_executable(av_transcoder main.cpp)
target_link_libraries(av_transcoder ${FFMPEG_LIBRARIES})


add_subdirectory(sample)