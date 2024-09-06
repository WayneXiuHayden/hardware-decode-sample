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

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "set video file as first arg" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "play file: " << argv[1] << std::endl;
  std::cout << "note that file should contain not less then 5 min of video "
               "vp9/h264, 1920x1080, 15fps"
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
        "tee name=t ! "
            "queue max-size-buffers=30 max-size-time=1000000000 max-size-bytes=0 ! "
        "h264parse ! "
        "nvv4l2decoder ! "
            "queue max-size-buffers=30 max-size-time=1000000000 max-size-bytes=0 ! "
        "nvvidconv ! "
        "video/x-raw ! "
        "appsink name=sink emit-signals=true sync=false";
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

      std::cout << "start seek" << std::endl;
      // std::cout << "current frame: " << cur_frame <<  ", start seek to " << start_time / GST_SECOND << " seconds" << std::endl;
      // auto seek_start = std::chrono::high_resolution_clock::now();
      // FIXME freezing here
      bool seek_result = gst_element_seek_simple(
          pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, start_time);
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

      // auto seek_end = std::chrono::high_resolution_clock::now();
      // auto seek_duration = std::chrono::duration_cast<std::chrono::milliseconds>(seek_end - seek_start);
      // std::cout << "seek operation took " << seek_duration.count() << " ms" << std::endl;

      // Query position after seek
      // gint64 current_position;
      // if (!gst_element_query_position(pipeline, GST_FORMAT_TIME, &current_position)) {
      //     std::cerr << "Unable to query current position" << std::endl;
      // } else {
      //     std::cout << "Current position after seek: " << current_position / GST_SECOND << " seconds" << std::endl;
      // }

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
