#!/bin/bash

gst-launch-1.0 videotestsrc num-buffers=4500 ! video/x-raw, format=BGR, width=1920, height=1080, framerate=15/1 ! videoconvert ! nvvidconv ! "video/x-raw(memory:NVMM), format=NV12" ! nvv4l2vp9enc maxperf-enable=1 control-rate=0 bitrate=8000000 ! video/x-vp9, stream-format=byte-stream ! matroskamux ! filesink location=video.mkv
