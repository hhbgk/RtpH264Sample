package com.rtp.h264.sample;

/**
 * Author: bob
 * Date: 16-9-13 15:16
 * Version: V1
 * Description:
 */
public class RtpClient {
    static{
        System.loadLibrary("rtp");
    }

    private native void _init();
    private native boolean _send(String filePath);
    private native boolean _destroy();
    public RtpClient(){
        _init();
    }

    public boolean send(String filePath){
        return _send(filePath);
    }

    public boolean release(){
        return _destroy();
    }
}
