# RtpH264Sample
A simple Android application, package H264 data and stream over RTP by UDP. And you can open VLC for testing use SDP file.

###Test
```C
m=video 5004 RTP/AVP 96
a=rtpmap:96 H264/90000
a=framerate:29
c=IN IP4 192.168.8.174
```

###Reference
1.[H.264码流结构及码流封装成RTP包分析](http://blog.csdn.net/maxwell_nc/article/details/50267593)和[ H.264码流整个RTP封包过程](http://blog.csdn.net/maxwell_nc/article/details/50315675)

2.[用实例分析H264 RTP payload](http://blog.csdn.net/zblue78/article/details/5948538)

3.[RTP](https://tools.ietf.org/html/rfc3550) & [RTP：实时应用程序传输协议](http://www.supmen.com/26qo5mkdp0.html)
