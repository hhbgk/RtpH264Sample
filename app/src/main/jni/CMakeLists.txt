cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(rtp)

add_library( rtp-lib

             SHARED

             rtp_jni.c )

target_link_libraries( rtp-lib
                       rtp
                       log )