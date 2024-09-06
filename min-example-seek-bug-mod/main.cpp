#include "gst/gstbin.h"
#include "gst/gstsegment.h"
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <csignal>
#include <gst/gst.h>
#include <iostream>
#include <mutex>
#include <thread>

std::atomic_bool done{false};

void done_handler(int) { done = true; }

static GstElement *tmp = nullptr;

static GstPadProbeReturn probe_cb(GstPad *pad, GstPadProbeInfo *info,
                                  gpointer user_data) {
  std::cerr << "pad probe" << std::endl;

  GstElement *pipeline = (GstElement *)user_data;

  GstEvent *event = gst_pad_probe_info_get_event(info);
  if (event == NULL || GST_EVENT_TYPE(event) != GST_EVENT_SEGMENT) {
    return GST_PAD_PROBE_PASS;
  }

  GstElement *fakesink = gst_bin_get_by_name(GST_BIN(pipeline), "fakesink");
  assert(fakesink != NULL);

  gst_element_set_state(fakesink, GST_STATE_NULL);
  bool remove_result = gst_bin_remove(GST_BIN(pipeline), fakesink);
  assert(remove_result == true);

  std::string tail_pipeline =
      "identity name=decoder ! "
      // "video/x-vp9, width=1920, height=1080, framerate=15/1 ! "
      "h264parse ! "
      "video/x-h264, width=1920, height=1080, framerate=15/1 ! "
      "nvv4l2decoder ! "
      "nvvidconv ! "
      "video/x-raw ! "
      "appsink name=sink emit-signals=true sync=false";
  GstElement *tail_pipe = gst_parse_launch(tail_pipeline.c_str(), NULL);
  assert(tail_pipe != NULL);

  GstElement *decoder = gst_bin_get_by_name(GST_BIN(tail_pipe), "decoder");
  assert(decoder != NULL);

  GstPad *tmp_pad = gst_element_get_static_pad(decoder, "sink");
  gst_element_add_pad(tail_pipe, gst_ghost_pad_new("sink", tmp_pad));
  gst_object_unref(tmp_pad);

  std::cerr << "link" << std::endl;
  gst_bin_add(GST_BIN(pipeline), tail_pipe);
  bool linked = gst_element_link(tmp, tail_pipe);
  assert(linked == true);

  std::cerr << "start" << std::endl;
  bool ret = gst_element_sync_state_with_parent(tail_pipe);
  assert(ret == true);

  std::cerr << "exit" << std::endl;
  return GST_PAD_PROBE_REMOVE;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "set video file as first arg" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "play file: " << argv[1] << std::endl;
  std::cout << "note that file should contain not less then 5 min of video "
               "vp9, 1920x1080, 15fps"
            << std::endl;

  std::string video_file = argv[1];

  std::signal(SIGINT, done_handler);
  std::signal(SIGTERM, done_handler);

  std::atomic_int cur_frame{0};
  std::condition_variable cond;
  std::atomic_ullong seek_ts = 0;

  // just send signals for consumer and check consumer freezing, nothing
  // interesting
  std::thread producer{[&cond, &cur_frame, &seek_ts]() {
    while (done == false) {
      for (int i = 0; done == false && i < 5 * 60 * 15 /*5 min of video*/;
           ++i) {
        if ((i % 150) == 0) {
          cur_frame = i;
          cond.notify_all();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});

        auto now = std::chrono::high_resolution_clock::now();
        time_t cur_ts = std::chrono::duration_cast<std::chrono::seconds>(
                            now.time_since_epoch())
                            .count();
        if (seek_ts != 0 && cur_ts - seek_ts > 10) {
          std::cerr << "SEEK FREEZING!" << std::endl;
          return;
        }
      }
    }

    cond.notify_all();

    std::cerr << "stop producer" << std::endl;
  }};

  std::this_thread::sleep_for(std::chrono::seconds{5});

  std::thread consumer{[&cond, &cur_frame, &seek_ts, video_file]() {
    // clang-format off
    std::string seek_pipeline =
        "filesrc location=" + video_file + " ! "
        "matroskademux name=demuxer demuxer.video_0 ! "
        "identity name=tmp ! "
        "fakesink name=fakesink";
    // clang-format on

    std::mutex mut;
    std::unique_lock<std::mutex> lock{mut};

    if (gst_is_initialized() == false) {
      gst_init(NULL, NULL);
    }

    while (done == false) {
      cond.wait(lock);

      time_t start_time = (cur_frame / 15) * GST_SECOND;

      GError *err;
      GstElement *pipeline = gst_parse_launch(seek_pipeline.c_str(), &err);
      assert(pipeline && "can't parse pipeline");

      // set state to paused
      GstStateChangeReturn state =
          gst_element_set_state(pipeline, GST_STATE_PAUSED);
      assert(state != GST_STATE_CHANGE_FAILURE &&
             "can't change state of pipeline");

      // link pipeline
      GstBus *bus = gst_element_get_bus(pipeline);
      assert(bus && "can't get bus for pipeline");

      GstMessage *msg = gst_bus_timed_pop_filtered(
          bus, GST_CLOCK_TIME_NONE,
          GstMessageType(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_EOS |
                         GST_MESSAGE_ERROR));
      assert(msg && "invalid message");
      if (msg->type != GST_MESSAGE_ASYNC_DONE) {
        gchar *dbg = nullptr;
        gst_message_parse_error(msg, &err, &dbg);
        std::cerr << dbg << std::endl;
        assert(msg->type == GST_MESSAGE_ASYNC_DONE);
      }
      gst_message_unref(msg);

      // add change pipeline probe at the end
      tmp = gst_bin_get_by_name(GST_BIN(pipeline), "tmp");
      assert(tmp != nullptr && "can't get element for link");

      GstPad *blockpad = gst_element_get_static_pad(tmp, "src");
      assert(blockpad);

      gst_pad_add_probe(blockpad,
                        GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM |
                                        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
                        probe_cb, pipeline, NULL);

      std::cout << "start seek" << std::endl;
      // FIXME freezing here
      bool seek_result = gst_element_seek_simple(
          pipeline, GST_FORMAT_TIME,
          GstSeekFlags(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT |
                       GST_SEEK_FLAG_SNAP_BEFORE),
          start_time);
      if (seek_result == false) {
        std::cerr << "fail to seek" << std::endl;
      }

      msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                       GstMessageType(GST_MESSAGE_ASYNC_DONE |
                                                      GST_MESSAGE_EOS |
                                                      GST_MESSAGE_ERROR));
      assert(msg && "invalid message");
      if (msg->type != GST_MESSAGE_ASYNC_DONE) {
        gchar *dbg = nullptr;
        gst_message_parse_error(msg, &err, &dbg);
        std::cerr << dbg << std::endl;
        assert(msg->type == GST_MESSAGE_ASYNC_DONE);
      }
      gst_message_unref(msg);

      std::cout << "stop seek" << std::endl;

      assert(gst_element_set_state(pipeline, GST_STATE_NULL) !=
                 GST_STATE_CHANGE_FAILURE &&
             "can't stop seek pipeline");

      gst_object_unref(GST_OBJECT(bus));
      gst_object_unref(GST_OBJECT(pipeline));

      // update seek_ts
      auto now = std::chrono::high_resolution_clock::now();
      seek_ts = std::chrono::duration_cast<std::chrono::seconds>(
                    now.time_since_epoch())
                    .count();
    }

    std::cerr << "stop consumer" << std::endl;
  }};

  if (producer.joinable()) {
    producer.join();
  }
  if (consumer.joinable()) {
    consumer.join();
  }

  return EXIT_SUCCESS;
}
