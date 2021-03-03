/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2021 Ubuntu <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-myjson
 *
 * FIXME:Describe myjson here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! myjson ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstmyjson.h"

GST_DEBUG_CATEGORY_STATIC (gst_my_json_debug);
#define GST_CAT_DEFAULT gst_my_json_debug

  /* NOTE:
    {
      "project_name":"name",                      DONE -> using property.
      "sample_width":16,
      "frame_rate":10000,
      "compression_type":"mp3",
      "start_time":"0:00:00.000000000",           DONE -> using buffer.
      "duration":"0:00:00.000000000",             DONE -> using buffer.
      "channel_num":1
    }
  */

  /* TODO:
    1. In chain method: check if gpointer custom_data causes memory leak. it cannot be freed at function end because it is required in later elements receiving the buffer.
    2. Complete sample_width, frame_rate, compression_type & channel_num fields.
    3. Check if every buffer should be inserted with json.
    4. Check if custom_json_mutex is required or not. (currently i use it because i dont want to create json while)
  */

#define JSON_MAX_LENGTH                       256

#define JSON_START_STRING                     "{"
#define JSON_TAB_STRING                       "    "
#define JSON_NEW_LINE_STRING                  "\n"
#define JSON_COMMA_STRING                     ","
#define JSON_COLON_STRING                     ":"
#define JSON_QUOTATION_MARK_STRING            "\""
#define JSON_END_STRING                       (JSON_QUOTATION_MARK_STRING JSON_NEW_LINE_STRING "}")

#define JSON_PROJECT_NAME_FIELD_STRING        (JSON_NEW_LINE_STRING JSON_TAB_STRING JSON_QUOTATION_MARK_STRING "project_name" JSON_QUOTATION_MARK_STRING JSON_COLON_STRING JSON_QUOTATION_MARK_STRING)
#define JSON_SAMPLE_WIDTH_FIELD_STRING        (JSON_NEW_LINE_STRING JSON_TAB_STRING JSON_QUOTATION_MARK_STRING "sample_width" JSON_QUOTATION_MARK_STRING JSON_COLON_STRING JSON_QUOTATION_MARK_STRING)
#define JSON_FRAME_RATE_FIELD_STRING          (JSON_NEW_LINE_STRING JSON_TAB_STRING JSON_QUOTATION_MARK_STRING "frame_rate" JSON_QUOTATION_MARK_STRING JSON_COLON_STRING JSON_QUOTATION_MARK_STRING)
#define JSON_COMPRESSION_TYPE_FIELD_STRING    (JSON_NEW_LINE_STRING JSON_TAB_STRING JSON_QUOTATION_MARK_STRING "compression_type" JSON_QUOTATION_MARK_STRING JSON_COLON_STRING JSON_QUOTATION_MARK_STRING)
#define JSON_START_TIME_FIELD_STRING          (JSON_NEW_LINE_STRING JSON_TAB_STRING JSON_QUOTATION_MARK_STRING "start_time" JSON_QUOTATION_MARK_STRING JSON_COLON_STRING JSON_QUOTATION_MARK_STRING)
#define JSON_DURATION_FIELD_STRING            (JSON_NEW_LINE_STRING JSON_TAB_STRING JSON_QUOTATION_MARK_STRING "duration" JSON_QUOTATION_MARK_STRING JSON_COLON_STRING JSON_QUOTATION_MARK_STRING)
#define JSON_CHANNEL_NUM_FIELD_STRING         (JSON_NEW_LINE_STRING JSON_TAB_STRING JSON_QUOTATION_MARK_STRING "channel_num" JSON_QUOTATION_MARK_STRING JSON_COLON_STRING JSON_QUOTATION_MARK_STRING) 

#define JSON_CONSTANT_FILEDS_LENGTH           153 // This is the sum of the length of all constant fields in the json in-order to evaluate easier the total json length legality.

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PROJECT_NAME,

};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_my_json_parent_class parent_class
G_DEFINE_TYPE (GstMyJson, gst_my_json, GST_TYPE_ELEMENT);


static void gst_my_json_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_my_json_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_my_json_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static GstFlowReturn gst_my_json_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);

static gboolean json_prepare(GstMyJson* myjson, GstBuffer* buffer, gpointer custom_json);
static gchar* json_complete_line(gchar* position);

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
myjson_init (GstPlugin * myjson)
{
  /* debug category for filtering log messages
   *
   * exchange the string 'Template myjson' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_my_json_debug, "myjson",
      0, "Template myjson");

  return gst_element_register (myjson, "myjson", GST_RANK_NONE,
      GST_TYPE_MYJSON);
}

/* GObject vmethod implementations */

/* initialize the myjson's class */
static void
gst_my_json_class_init (GstMyJsonClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_my_json_set_property;
  gobject_class->get_property = gst_my_json_get_property;

  g_object_class_install_property (gobject_class, PROP_PROJECT_NAME,
      g_param_spec_string ("project_name", "Project Name", "Project Name",
          "default_project_name", G_PARAM_READWRITE));

  gst_element_class_set_details_simple (gstelement_class,
      "MyJson",
      "FIXME:Generic",
      "FIXME:Generic Template Element", "Ubuntu <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_my_json_init (GstMyJson * myjson)
{
  myjson->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (myjson->sinkpad,
      GST_DEBUG_FUNCPTR (gst_my_json_sink_event));
  gst_pad_set_chain_function (myjson->sinkpad,
      GST_DEBUG_FUNCPTR (gst_my_json_chain));
  GST_PAD_SET_PROXY_CAPS (myjson->sinkpad);
  gst_element_add_pad (GST_ELEMENT (myjson), myjson->sinkpad);

  myjson->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (myjson->srcpad);
  gst_element_add_pad (GST_ELEMENT (myjson), myjson->srcpad);

  /* Initialize members and properties. */
  g_mutex_init(&(myjson->custom_json_mutex));
  g_strlcpy(myjson->project_name, "default_project_name", MAX_PROJECT_NAME_LENGTH);
}

static void
gst_my_json_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMyJson *myjson = GST_MYJSON (object);

  switch (prop_id) {
    case PROP_PROJECT_NAME:
      g_mutex_lock(&(myjson->custom_json_mutex));
      g_strlcpy(myjson->project_name, g_value_get_string (value), MAX_PROJECT_NAME_LENGTH);
      g_mutex_unlock(&(myjson->custom_json_mutex));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_my_json_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstMyJson *myjson = GST_MYJSON (object);

  switch (prop_id) {
    case PROP_PROJECT_NAME:
      g_value_set_string(value, myjson->project_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_my_json_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstMyJson *myjson;
  gboolean ret;

  myjson = GST_MYJSON (parent);
  

  GST_LOG_OBJECT (myjson, "Received %s event: %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME (event), event);
  
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_my_json_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstMyJson *myjson;
  gboolean result = FALSE;
  gpointer custom_json = g_new(gchar, JSON_MAX_LENGTH); // MEMORY LEAK??!?!?! CANT FREE AT THE FUCNTION END BECAUSE USED LATER.
  myjson = GST_MYJSON (parent);

  if (buf)
  { 
    // g_print("PTS: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(GST_BUFFER_PTS(buf)));

    g_mutex_lock(&(myjson->custom_json_mutex));
    result = json_prepare(myjson, buf, custom_json);
    g_mutex_unlock(&(myjson->custom_json_mutex));
    if (TRUE == result)
    {
      gst_buffer_insert_memory(buf, 0, gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY, custom_json, JSON_MAX_LENGTH, 0, strlen(custom_json), NULL, NULL));
    }
  }

  /* just push out the incoming buffer without touching it */
  return gst_pad_push (myjson->srcpad, buf);
}

/* TODO add:custom_data
  {
    "project_name":"name",                      DONE -> using property.
    "sample_width":16,
    "frame_rate":10000,
    "compression_type":"mp3",
    "start_time":"0:00:00.000000000",
    "duration":"0:00:00.000000000",
    "channel_num":1
  }
*/
static gboolean json_prepare(GstMyJson* myjson, GstBuffer* buffer, gpointer custom_json)
{
  if (myjson)
  {
    /* Check if sum of length will be legal (-1 for null char to indicate end of string). */
    // if ((JSON_MAX_LENGTH - JSON_CONSTANT_FILEDS_LENGTH - 1) <  
    //     strlen(myjson->project_name) + 
    //     strlen(myjson->sample_width) + 
    //     strlen(myjson->frame_rate) + 
    //     strlen(myjson->compression_type) + 
    //     MAX_START_TIME_LENGTH + 
    //     MAX_DURATION_LENGTH + 
    //     strlen(myjson->channel_num)) 
    // if ((JSON_MAX_LENGTH - JSON_CONSTANT_FILEDS_LENGTH - 1) <  strlen(myjson->project_name))
    // {
    //   return FALSE;
    // } 

    /* Initialize constant json string start (should not be changed later on). */
    gchar* last_pos = g_stpcpy(custom_json, JSON_START_STRING);

    /* Fill project name field (beginnning of JSON is constant and already prepared). */
    last_pos = g_stpcpy(last_pos, JSON_PROJECT_NAME_FIELD_STRING);
    last_pos = g_stpcpy(last_pos, myjson->project_name);
    last_pos = json_complete_line(last_pos);

    /* Fill sample width fields. */
    last_pos = g_stpcpy(last_pos, JSON_SAMPLE_WIDTH_FIELD_STRING);
    // last_pos = g_stpcpy(last_pos, myjson->sample_width);
    last_pos = json_complete_line(last_pos);

    /* Fill frame rate fields. */
    last_pos = g_stpcpy(last_pos, JSON_FRAME_RATE_FIELD_STRING);
    // last_pos = g_stpcpy(last_pos, myjson->frame_rate);
    last_pos = json_complete_line(last_pos);

    /* Fill compression type fields. */
    last_pos = g_stpcpy(last_pos, JSON_COMPRESSION_TYPE_FIELD_STRING);
    // last_pos = g_stpcpy(last_pos, myjson->compression_type);
    last_pos = json_complete_line(last_pos);

    /* Fill quotation mark fields. */
    last_pos = g_stpcpy(last_pos, JSON_START_TIME_FIELD_STRING);
    g_sprintf(last_pos, "%" GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_PTS(buffer)));
    last_pos = json_complete_line(last_pos + MAX_START_TIME_LENGTH);

    /* Fill duration fields. */
    last_pos = g_stpcpy(last_pos, JSON_DURATION_FIELD_STRING);
    g_sprintf(last_pos, "%" GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_DURATION(buffer)));
    last_pos = json_complete_line(last_pos + MAX_DURATION_LENGTH);

    /* Fill channel num fields. */
    last_pos = g_stpcpy(last_pos, JSON_CHANNEL_NUM_FIELD_STRING);
    // last_pos = g_stpcpy(last_pos, myjson->channel_num);

    g_stpcpy(last_pos, JSON_END_STRING);

    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

static gchar* json_complete_line(gchar* position)
{
    *(position) = *(JSON_QUOTATION_MARK_STRING);
    position += 1;
    *position = *(JSON_COMMA_STRING);
    position += 1;

    return position;
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstmyjson"
#endif

/* gstreamer looks for this structure to register myjsons
 *
 * exchange the string 'Template myjson' with your myjson description
 */
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    myjson,
    "Template myjson",
    myjson_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
