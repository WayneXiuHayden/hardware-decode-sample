#include <chrono>
#include <cstdlib>
#include <gst/app/app.h>
#include <gst/gst.h>
#include <thread>


#define HW_PIPELINE


// nvv4l2h264enc could re-order PTS during encoding
// clang-format off
const double PTS_list[] = {
  0,
  0.1,
  0.2,
  0.3,
  0.4,
  0.5,
  0.6,
  0.7,
  0.8,
  0.9,
  0.99,
  1.08,
  1.17,
  1.26,
  1.35,     //1.44,
  1.44,     //1.63,
  // 1.53,     // with 1.53 here, issue remains actually 100% (1.35 1.53 1.44 1.63)
  1.63,     //1.72,
  1.72,     //1.35,
  1.81,
  1.9,
  2.0,
  2.1,
  2.2,
  2.3,
  2.4,
  2.5,
  2.6,
  2.7,
  2.8,
  2.9,
  3.0,
};
// clang-format on


int main() {
  gst_init(NULL, NULL);


  {
    // generate video: grayscale, 1600x1300, 10 FPS
    fprintf(stderr, "generate test video...\n");


    GError *    err      = nullptr;
    GstElement *pipeline = gst_parse_launch(
        "appsrc name=appsrc format=3 is-live=TRUE block=TRUE ! "
        "video/x-raw, format=GRAY8, width=1600, height=1300, framerate=10/1 ! "
        "identity silent=FALSE ! "
#ifdef HW_PIPELINE
        "nvvidconv ! "
        "video/x-raw(memory:NVMM) ! "
        "identity sleep-time=5000 ! "
        // "nvv4l2h264enc maxperf-enable=1 control-rate=0 bitrate=8000000 ! "
        "nvv4l2vp9enc maxperf-enable=1 control-rate=0 bitrate=8000000 ! "
        "video/x-vp9, stream-format=byte-stream ! "
#else
        "videoconvert ! "
        // "x264enc tune=zerolatency speed-preset=ultrafast ! "
        "vp9enc ! "
#endif
        // "video/x-h264, profile=baseline ! "
        // "h264parse ! "
        "identity silent=FALSE ! "
        "matroskamux ! "
        "filesink sync=FALSE location=video.mkv",
        &err);

    if (err) {
      fprintf(stderr, "can not create gst pipeline\n");
      return EXIT_FAILURE;
    }


    g_signal_connect(pipeline,
                     "deep-notify",
                     G_CALLBACK(gst_object_default_deep_notify),
                     NULL);


    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) ==
        GST_STATE_CHANGE_FAILURE) {
      fprintf(stderr, "can not start gst pipeline\n");
      return EXIT_FAILURE;
    }


    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "appsrc");
    if (appsrc == NULL) {
      fprintf(stderr, "can not get appsrc from gst pipeline\n");
      return EXIT_FAILURE;
    }


    for (size_t i = 0; i < sizeof(PTS_list) / sizeof(double); ++i) {
      GstBuffer *buf = gst_buffer_new_and_alloc(1600 * 1300);

      if (buf == NULL) {
        fprintf(stderr, "can not copy buffer\n");
        return EXIT_FAILURE;
      }

      buf = gst_buffer_make_writable(buf);

      GST_BUFFER_PTS(buf)      = PTS_list[i] * GST_SECOND;
      GST_BUFFER_DTS(buf)      = GST_CLOCK_TIME_NONE;
      GST_BUFFER_DURATION(buf) = GST_CLOCK_TIME_NONE;


      if (gst_app_src_push_buffer(GST_APP_SRC(appsrc), buf) != GST_FLOW_OK) {
        fprintf(stderr, "can not push buffer to appsink\n");
        return EXIT_FAILURE;
      }


      // XXX if add some delay between pushing frames, then problem can not
      // be reproducedi
      // at least 16 milliseconds for this case
      // actually even with this sleep, the PTS reordering is still possible (1.26 1.44 1.35 1.63)
      // std::this_thread::sleep_for(std::chrono::milliseconds{16});
    }


    gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);


    fprintf(stderr, "video generation is done.\n");
  }


  // read the video, get timestamps and check for duplicates

  GError *    err = NULL;
  GstElement *pipeline =
      gst_parse_launch("filesrc location=video.mkv ! "
                       "matroskademux ! "
                      //  "h264parse ! "
                      //  "nvv4l2decoder ! "
                      //  "avdec_h264 ! "
                       "vp9dec ! "
                       "videoconvert ! "
                       "appsink name=appsink max-buffers=5 sync=FALSE",
                       &err);

  if (err) {
    fprintf(stderr, "can not create gst pipeline\n");
    return EXIT_FAILURE;
  }


  if (gst_element_set_state(pipeline, GST_STATE_PLAYING) ==
      GST_STATE_CHANGE_FAILURE) {
    fprintf(stderr, "can not start gst pipeline\n");
    return EXIT_FAILURE;
  }


  gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);


  GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "appsink");
  if (appsink == NULL) {
    fprintf(stderr, "can not get appsink from gst pipeline\n");
    return EXIT_FAILURE;
  }


  fprintf(stderr, "start processing...\n");

  time_t prev_ts = GST_CLOCK_TIME_NONE;
  for (;;) {
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    if (sample == nullptr) {
      if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
        break;
      } else {
        fprintf(stderr, "can not get next frame from pipeline\n");
        return EXIT_FAILURE;
      }
    }


    GstBuffer *buf = gst_sample_get_buffer(sample);

    time_t ts = GST_BUFFER_PTS(buf);


    if (prev_ts == ts) {
      fprintf(stderr, "duplicate %li\n", ts);
    }


    gst_sample_unref(sample);


    prev_ts = ts;
  }


  fprintf(stderr, "Done.\n");

  return EXIT_SUCCESS;
}
