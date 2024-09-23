#include <gst/gst.h>
#include <iostream>

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            g_printerr("Error: %s\n", error->message);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    if (argc != 2) {
        g_printerr("Usage: %s <VP9 video file>\n", argv[0]);
        return -1;
    }

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    GstElement *pipeline = gst_pipeline_new("video-player");
    GstElement *source = gst_element_factory_make("filesrc", "file-source");
    GstElement *demuxer = gst_element_factory_make("matroskademux", "demuxer");
    GstElement *decoder = gst_element_factory_make("nvv4l2decoder", "decoder");
    GstElement *conv = gst_element_factory_make("videoconvert", "converter");
    GstElement *sink = gst_element_factory_make("autovideosink", "video-output");

    if (!pipeline || !source || !demuxer || !decoder || !conv || !sink) {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }

    g_object_set(G_OBJECT(source), "location", argv[1], NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, demuxer, decoder, conv, sink, NULL);
    gst_element_link(source, demuxer);
    gst_element_link_many(decoder, conv, sink, NULL);

    g_signal_connect(demuxer, "pad-added", G_CALLBACK(+[](GstElement *src, GstPad *new_pad, GstElement *decoder) {
        GstPad *sink_pad = gst_element_get_static_pad(decoder, "sink");
        gst_pad_link(new_pad, sink_pad);
        gst_object_unref(sink_pad);
    }), decoder);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_main_loop_unref(loop);

    return 0;
}
