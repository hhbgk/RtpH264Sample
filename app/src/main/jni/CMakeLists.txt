cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(rtp)

aux_source_directory(. JNI_SRCS)

add_library(rtp SHARED ${JNI_SRCS})

target_link_libraries(rtp log rtp_h264)