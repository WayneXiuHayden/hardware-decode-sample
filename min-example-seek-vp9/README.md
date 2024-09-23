nvidia gst decoder plugin supports vp9 profile 0 only; it will have problem seeking if profile is not 0

```
gst-launch-1.0 filesrc location=${test_video_file} ! matroskademux ! nvv4l2decoder ! videoconvert ! autovideosink
```

note that we will get `Error: Internal data stream error.` on Jetson Orin because the lacking of hardware acceleration for vp9 on Orin
