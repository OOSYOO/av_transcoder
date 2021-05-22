cmake_minimum_required(VERSION 3.16)
project(sample)


add_executable(sample_video_decode sample_video_decode.cpp)
target_link_libraries(sample_video_decode ${FFMPEG_LIBRARIES})

add_executable(sample_video_encode sample_video_encode.cpp)
target_link_libraries(sample_video_encode ${FFMPEG_LIBRARIES})

add_executable(sample_mp4_to_yuvpcm sample_mp4_to_yuvpcm.cpp)
target_link_libraries(sample_mp4_to_yuvpcm ${FFMPEG_LIBRARIES})

add_executable(sample_yuvpcm_to_mp4 sample_yuvpcm_to_mp4.cpp)
target_link_libraries(sample_yuvpcm_to_mp4 ${FFMPEG_LIBRARIES})

add_executable(sample_h264_to_rtp sample_h264_to_rtp.cpp ../src/rtpencoder/rtpencoder.cpp)
target_link_libraries(sample_h264_to_rtp ${FFMPEG_LIBRARIES})

