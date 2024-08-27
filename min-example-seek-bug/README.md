# deadlock at gst_element_seek_simple

Sometimes when I use `gst_element_seek_simple` for pipeline that contains
nvv* elements it freezes. It looks like deadlock. Also I can't reproduce the
problem when I start only one pipeline, but it always happens (after several
minutes) when I start two or more pipelines. Assume that there is some problem
with sharing common memory between nvv* elements.

gst have a similar problem with deadlock if [seek without GST_SEEK_FLAG_FLUSH when a pipeline is in PAUSED state](https://gstreamer.freedesktop.org/documentation/application-development/advanced/queryevents.html),
but I use this flag


## system

lsb_release:

```
Distributor ID: Ubuntu
Description:    Ubuntu 18.04.5 LTS
Release:        18.04
Codename:       bionic
```

jetson_release:

```
X Error of failed request:  BadLength (poly request too large or internal Xlib length error)
  Major opcode of failed request:  156 (NV-GLX)
  Minor opcode of failed request:  1 ()
  Serial number of failed request:  19
  Current serial number in output stream:  19
 - NVIDIA Jetson Xavier NX (Developer Kit Version)
   * Jetpack 4.4.1 [L4T 32.4.4]
   * NV Power Mode: MODE_10W_2CORE - Type: 3
   * jetson_stats.service: active
 - Libraries:
   * CUDA: 10.2.89
   * cuDNN: 8.0.0.180
   * TensorRT: 7.1.3.0
   * Visionworks: 1.6.0.501
   * OpenCV: 4.1.1 compiled CUDA: NO
   * VPI: 0.4.4
   * Vulkan: 1.2.70
```

gst-launch-1.0 --version:

```
gst-launch-1.0 version 1.14.5
GStreamer 1.14.5
https://launchpad.net/distros/ubuntu/+source/gstreamer1.0
```


## USAGE

1. compile

```bash
mkdir build
cd build
cmake ..
make
```

2. generate video file for test by [script](generate_video.sh)

```bash
./generate-video.sh
```

3. start __minimum(!)__ 2 instances of `seek_min_ex` app. Set generated video file
as first argument

```bash
./seek_min_ex video.mkv
```

__NOTE__ better start more the 2 instances

4. wait for "SEEK FREEZING!" message from one of started program

__NOTE__ it can takes several minutes


## backtrace

```
* thread #1, name = 'seek_min_ex', stop reason = signal SIGSTOP
  * frame #0: 0x0000007f90d1e310 libpthread.so.0`__GI___pthread_timedjoin_ex(threadid=547876274640, thread_return=0x0000000000000000, a
bstime=0x0000000000000000, block=false) at pthread_join_common.c:89
    frame #1: 0x0000007f90c3e128 libstdc++.so.6`std::thread::join() + 32
    frame #2: 0x0000005564227bdc seek_min_ex`main(argc=2, argv=0x0000007fc4bb2388) at main.cpp:158
    frame #3: 0x0000007f90a25720 libc.so.6`__libc_start_main(main=0x0000000000000000, argc=0, argv=0x0000000000000000, init=<unavailabl
e>, fini=<unavailable>, rtld_fini=<unavailable>, stack_end=<unavailable>) at libc-start.c:310
    frame #4: 0x0000005564227074 seek_min_ex`_start + 52
  thread #2, name = 'seek_min_ex', stop reason = signal SIGSTOP
    frame #0: 0x0000007f90d26690 libpthread.so.0`__lll_lock_wait(futex=0x0000007f8828db40, private=0) at lowlevellock.c:46
    frame #1: 0x0000007f90d1f7d8 libpthread.so.0`__GI___pthread_mutex_lock(mutex=0x0000007f8828db40) at pthread_mutex_lock.c:115
    frame #2: 0x0000007f90e29200 libgstreamer-1.0.so.0`gst_pad_stop_task + 232
    frame #3: 0x0000007f8f3a42e8 libgstnvvideo4linux2.so`___lldb_unnamed_symbol133$$libgstnvvideo4linux2.so + 232
    frame #4: 0x0000007f90e1c69c libgstreamer-1.0.so.0
    frame #5: 0x0000007f90e1cb68 libgstreamer-1.0.so.0
    frame #6: 0x0000007f90e278e0 libgstreamer-1.0.so.0`gst_pad_push_event + 600
    frame #7: 0x0000007f8f6c8890 libgstbase-1.0.so.0
    frame #8: 0x0000007f8f7419ac libgstcoreelements.so
    frame #9: 0x0000007f90e1c69c libgstreamer-1.0.so.0
    frame #10: 0x0000007f90e1cb68 libgstreamer-1.0.so.0
    frame #11: 0x0000007f90e278e0 libgstreamer-1.0.so.0`gst_pad_push_event + 600
    frame #12: 0x0000007f8f637cc4 libgstmatroska.so
    frame #13: 0x0000007f8f646be0 libgstmatroska.so
    frame #14: 0x0000007f8f647a7c libgstmatroska.so
    frame #15: 0x0000007f90e1c69c libgstreamer-1.0.so.0
    frame #16: 0x0000007f90e1cb68 libgstreamer-1.0.so.0
    frame #17: 0x0000007f90e278e0 libgstreamer-1.0.so.0`gst_pad_push_event + 600
    frame #18: 0x0000007f8f6c8f24 libgstbase-1.0.so.0
    frame #19: 0x0000007f90e1c69c libgstreamer-1.0.so.0
    frame #20: 0x0000007f90e1cb68 libgstreamer-1.0.so.0
    frame #21: 0x0000007f90e278e0 libgstreamer-1.0.so.0`gst_pad_push_event + 600
    frame #22: 0x0000007f8f565bf0 libgstvideo-1.0.so.0
    frame #23: 0x0000007f90e1c69c libgstreamer-1.0.so.0
    frame #24: 0x0000007f90e1cb68 libgstreamer-1.0.so.0
    frame #25: 0x0000007f90e278e0 libgstreamer-1.0.so.0`gst_pad_push_event + 600
    frame #26: 0x0000007f8f6c8f24 libgstbase-1.0.so.0
    frame #27: 0x0000007f90e1c69c libgstreamer-1.0.so.0
    frame #28: 0x0000007f90e1cb68 libgstreamer-1.0.so.0
    frame #29: 0x0000007f90e278e0 libgstreamer-1.0.so.0`gst_pad_push_event + 600
    frame #30: 0x0000007f8f6c8f24 libgstbase-1.0.so.0
    frame #31: 0x0000007f90e1c69c libgstreamer-1.0.so.0
    frame #32: 0x0000007f90e1cb68 libgstreamer-1.0.so.0
    frame #33: 0x0000007f90e278e0 libgstreamer-1.0.so.0`gst_pad_push_event + 600
    frame #34: 0x0000007f8f6b1f80 libgstbase-1.0.so.0
    frame #35: 0x0000007f90e01678 libgstreamer-1.0.so.0`gst_element_send_event + 128
    frame #36: 0x0000007f90ddae34 libgstreamer-1.0.so.0
    frame #37: 0x0000007f90e01678 libgstreamer-1.0.so.0`gst_element_send_event + 128
    frame #38: 0x0000005564227720 seek_min_ex`operator(__closure=0x000000558c276af8) at main.cpp:116
    frame #39: 0x0000005564228258 seek_min_ex`std::__invoke_impl<void, main(int, char**)::<lambda()> >((null)=__invoke_other @ 0x000000
7f8ff87918, __f=0x000000558c276af8)> &&) at invoke.h:60
    frame #40: 0x0000005564227de0 seek_min_ex`std::__invoke<main(int, char**)::<lambda()> >(__fn=0x000000558c276af8)> &&) at invoke.h:9
5
    frame #41: 0x00000055642286c8 seek_min_ex`std::thread::_Invoker<std::tuple<main(int, char**)::<lambda()> > >::_M_invoke<0>(this=0x0
00000558c276af8, (null)=_Index_tuple<0> @ 0x0000007f8ff87960) const at thread:234
    frame #42: 0x000000556422867c seek_min_ex`std::thread::_Invoker<std::tuple<main(int, char**)::<lambda()> > >::operator(this=0x00000
0558c276af8)() const at thread:243
    frame #43: 0x0000005564228638 seek_min_ex`std::thread::_State_impl<std::thread::_Invoker<std::tuple<main(int, char**)::<lambda()> >
 > >::_M_run(this=0x000000558c276af0) const at thread:186
    frame #44: 0x0000007f90c3de94 libstdc++.so.6`___lldb_unnamed_symbol208$$libstdc++.so.6 + 28
    frame #45: 0x0000007f90d1d088 libpthread.so.0`start_thread(arg=0x0000007fc4bb207f) at pthread_create.c:463
    frame #46: 0x0000007f90ad4ffc libc.so.6`thread_start at clone.S:78
  thread #3, name = 'nvv4l2decoder11', stop reason = signal SIGSTOP
    frame #0: 0x0000007f90d232a4 libpthread.so.0`__pthread_cond_wait at futex-internal.h:88
    frame #1: 0x0000007f90d23284 libpthread.so.0`__pthread_cond_wait at pthread_cond_wait.c:502
    frame #2: 0x0000007f90d231b0 libpthread.so.0`__pthread_cond_wait(cond=0x0000007f7407bf10, mutex=0x0000007f7407bee0) at pthread_cond
_wait.c:655
    frame #3: 0x0000007f8f072fdc libnvos.so`___lldb_unnamed_symbol66$$libnvos.so + 44
    frame #4: 0x0000007f8e91abf0 libtegrav4l2.so`TegraV4L2_Poll_CPlane + 80
    frame #5: 0x0000007f8e9b5274 libv4l2_nvvideocodec.so`plugin_ioctl + 1084
    frame #6: 0x0000007f8f25d5c0 libv4l2.so.0`v4l2_ioctl + 312
    frame #7: 0x0000007f8f38e8cc libgstnvvideo4linux2.so`___lldb_unnamed_symbol28$$libgstnvvideo4linux2.so + 236
    frame #8: 0x0000007f8f392d1c libgstnvvideo4linux2.so`___lldb_unnamed_symbol55$$libgstnvvideo4linux2.so + 244
    frame #9: 0x0000007f90de5600 libgstreamer-1.0.so.0`gst_buffer_pool_acquire_buffer + 128
    frame #10: 0x0000007f8f3a6d80 libgstnvvideo4linux2.so`___lldb_unnamed_symbol156$$libgstnvvideo4linux2.so + 200
    frame #11: 0x0000007f90e57eb4 libgstreamer-1.0.so.0
    frame #12: 0x0000007f9095a558 libglib-2.0.so.0
    frame #13: 0x0000007f90959a64 libglib-2.0.so.0
    frame #14: 0x0000007f90d1d088 libpthread.so.0`start_thread(arg=0x0000007f8ff86f68) at pthread_create.c:463
    frame #15: 0x0000007f90ad4ffc libc.so.6`thread_start at clone.S:78
  thread #4, name = 'demuxer:sink', stop reason = signal SIGSTOP
    frame #0: 0x0000007f90ad1400 libc.so.6`syscall at syscall.S:38
    frame #1: 0x0000007f90979b84 libglib-2.0.so.0`g_cond_wait + 60
    frame #2: 0x0000007f90e580dc libgstreamer-1.0.so.0
    frame #3: 0x0000007f9095a558 libglib-2.0.so.0
    frame #4: 0x0000007f90959a64 libglib-2.0.so.0
    frame #5: 0x0000007f90d1d088 libpthread.so.0`start_thread(arg=0x0000007f8e706aa8) at pthread_create.c:463
    frame #6: 0x0000007f90ad4ffc libc.so.6`thread_start at clone.S:78
  thread #5, name = 'NVMDecBufProcT', stop reason = signal SIGSTOP
    frame #0: 0x0000007f90d232a4 libpthread.so.0`__pthread_cond_wait at futex-internal.h:88
    frame #1: 0x0000007f90d23284 libpthread.so.0`__pthread_cond_wait at pthread_cond_wait.c:502
    frame #2: 0x0000007f90d231b0 libpthread.so.0`__pthread_cond_wait(cond=0x0000007f74089400, mutex=0x0000007f740893d0) at pthread_cond
_wait.c:655
    frame #3: 0x0000007f8f072fdc libnvos.so`___lldb_unnamed_symbol66$$libnvos.so + 44
    frame #4: 0x0000007f8e8dee9c libnvmmlite_video.so`___lldb_unnamed_symbol24$$libnvmmlite_video.so + 132
    frame #5: 0x0000007f8f072628 libnvos.so`___lldb_unnamed_symbol52$$libnvos.so + 56
    frame #6: 0x0000007f90d1d088 libpthread.so.0`start_thread(arg=0x0000007f861f8bcf) at pthread_create.c:463
    frame #7: 0x0000007f90ad4ffc libc.so.6`thread_start at clone.S:78
  thread #6, name = 'NVMDecDisplayT', stop reason = signal SIGSTOP
    frame #0: 0x0000007f90d232a4 libpthread.so.0`__pthread_cond_wait at futex-internal.h:88
    frame #1: 0x0000007f90d23284 libpthread.so.0`__pthread_cond_wait at pthread_cond_wait.c:502
    frame #2: 0x0000007f90d231b0 libpthread.so.0`__pthread_cond_wait(cond=0x0000007f740284a0, mutex=0x0000007f74028470) at pthread_cond
_wait.c:655
    frame #3: 0x0000007f8f072fdc libnvos.so`___lldb_unnamed_symbol66$$libnvos.so + 44
    frame #4: 0x0000007f8e8dc560 libnvmmlite_video.so`___lldb_unnamed_symbol11$$libnvmmlite_video.so + 96
    frame #5: 0x0000007f8f072628 libnvos.so`___lldb_unnamed_symbol52$$libnvos.so + 56
    frame #6: 0x0000007f90d1d088 libpthread.so.0`start_thread(arg=0x0000007f861f8bcf) at pthread_create.c:463
    frame #7: 0x0000007f90ad4ffc libc.so.6`thread_start at clone.S:78
  thread #7, name = 'NVMDecFrmStatsT', stop reason = signal SIGSTOP
    frame #0: 0x0000007f90d232a4 libpthread.so.0`__pthread_cond_wait at futex-internal.h:88
    frame #1: 0x0000007f90d23284 libpthread.so.0`__pthread_cond_wait at pthread_cond_wait.c:502
    frame #2: 0x0000007f90d231b0 libpthread.so.0`__pthread_cond_wait(cond=0x0000007f74028510, mutex=0x0000007f740284e0) at pthread_cond
_wait.c:655
    frame #3: 0x0000007f8f072fdc libnvos.so`___lldb_unnamed_symbol66$$libnvos.so + 44
    frame #4: 0x0000007f8e8e0584 libnvmmlite_video.so`___lldb_unnamed_symbol26$$libnvmmlite_video.so + 92
    frame #5: 0x0000007f8f072628 libnvos.so`___lldb_unnamed_symbol52$$libnvos.so + 56
    frame #6: 0x0000007f90d1d088 libpthread.so.0`start_thread(arg=0x0000007f861f8bcf) at pthread_create.c:463
    frame #7: 0x0000007f90ad4ffc libc.so.6`thread_start at clone.S:78
  thread #8, name = 'NVMDecVPRFlrSzT', stop reason = signal SIGSTOP
    frame #0: 0x0000007f90d232a4 libpthread.so.0`__pthread_cond_wait at futex-internal.h:88
    frame #1: 0x0000007f90d23284 libpthread.so.0`__pthread_cond_wait at pthread_cond_wait.c:502
    frame #2: 0x0000007f90d231b0 libpthread.so.0`__pthread_cond_wait(cond=0x0000007f74028820, mutex=0x0000007f740287f0) at pthread_cond
_wait.c:655
    frame #3: 0x0000007f8f072fdc libnvos.so`___lldb_unnamed_symbol66$$libnvos.so + 44
    frame #4: 0x0000007f8e8db888 libnvmmlite_video.so`___lldb_unnamed_symbol4$$libnvmmlite_video.so + 88
    frame #5: 0x0000007f8f072628 libnvos.so`___lldb_unnamed_symbol52$$libnvos.so + 56
    frame #6: 0x0000007f90d1d088 libpthread.so.0`start_thread(arg=0x0000007f861f8bcf) at pthread_create.c:463
    frame #7: 0x0000007f90ad4ffc libc.so.6`thread_start at clone.S:78
  thread #9, name = 'V4L2_DecThread', stop reason = signal SIGSTOP
    frame #0: 0x0000007f90d232a4 libpthread.so.0`__pthread_cond_wait at futex-internal.h:88
    frame #1: 0x0000007f90d23284 libpthread.so.0`__pthread_cond_wait at pthread_cond_wait.c:502
    frame #2: 0x0000007f90d231b0 libpthread.so.0`__pthread_cond_wait(cond=0x0000007f7407bf80, mutex=0x0000007f7407bf50) at pthread_cond
_wait.c:655
    frame #3: 0x0000007f8f072fdc libnvos.so`___lldb_unnamed_symbol66$$libnvos.so + 44
    frame #4: 0x0000007f8e92bfd0 libtegrav4l2.so`___lldb_unnamed_symbol61$$libtegrav4l2.so + 72
    frame #5: 0x0000007f8f072628 libnvos.so`___lldb_unnamed_symbol52$$libnvos.so + 56
    frame #6: 0x0000007f90d1d088 libpthread.so.0`start_thread(arg=0x0000007f861f954f) at pthread_create.c:463
    frame #7: 0x0000007f90ad4ffc libc.so.6`thread_start at clone.S:78
```
