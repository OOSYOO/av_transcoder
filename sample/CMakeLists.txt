cmake_minimum_required(VERSION 3.16)
project(sample)


add_executable(sample_video_decode sample_video_decode.cpp)
target_link_libraries(sample_video_decode ${FFMPEG_LIBRARIES})