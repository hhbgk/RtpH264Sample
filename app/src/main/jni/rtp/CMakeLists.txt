aux_source_directory(. DIR_LIB_SRCS)

find_library(log-lib log)

add_library(rtp_h264 ${DIR_LIB_SRCS})

target_link_libraries(rtp_h264 ${log-lib})