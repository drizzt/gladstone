/* GladSToNe g729 Decoder
 * Copyright (C) <2009> Gibrovacco <gibrovacco@gmail.com>
 *
 * It is possible to redistribute this code using the LGPL license:
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
 *
 * Alternatively, you can redistribute at your choice using the MIT license,
 * reported below:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


/**
 * SECTION:element-g729dec
 * @see_also: g729enc
 *
 * This element decodes a G729 stream to raw integer audio.
 *
 * <refsect2>
 * <title>Example pipelines</title>
 * TODO
 * documentation of g729dec.
 * </refsect2>
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "gstg729dec.h"
#include <string.h>
#include <gst/tag/tag.h>

GST_DEBUG_CATEGORY_STATIC (g729dec_debug);
#define GST_CAT_DEFAULT g729dec_debug

static const GstElementDetails g729_dec_details =
GST_ELEMENT_DETAILS ("G729 audio decoder",
    "Codec/Decoder/Audio",
    "decode g729 streams to audio",
    "Gibro Vacco <gibrovacco@gmail.com>");

enum
{
  ARG_0,
};

static GstStaticPadTemplate g729_dec_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "audio/x-raw-int,"
        "rate = (int) 8000,"
        "channels = (int) 1,"
        "endianness = (int) BYTE_ORDER,"
        "signed = (boolean) true, " "width = (int) 16, " "depth = (int) 16")
    );

static GstStaticPadTemplate g729_dec_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/G729, "
        "rate = (int) 8000, "
        "channels = (int) 1")
    );

GST_BOILERPLATE (GstG729Dec, gst_g729_dec, GstElement, GST_TYPE_ELEMENT);

static gboolean g729_dec_sink_event (GstPad * pad, GstEvent * event);
static GstFlowReturn g729_dec_chain (GstPad * pad, GstBuffer * buf);
static GstStateChangeReturn g729_dec_change_state (GstElement * element,
    GstStateChange transition);

static gboolean g729_dec_src_event (GstPad * pad, GstEvent * event);
static gboolean g729_dec_src_query (GstPad * pad, GstQuery * query);
static gboolean g729_dec_sink_query (GstPad * pad, GstQuery * query);
static const GstQueryType *g729_get_src_query_types (GstPad * pad);
static const GstQueryType *g729_get_sink_query_types (GstPad * pad);
static gboolean g729_dec_convert (GstPad * pad,
    GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value);

static void gst_g729_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_g729_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static GstFlowReturn g729_dec_chain_parse_data (GstG729Dec * dec,
    GstBuffer * buf, GstClockTime timestamp, GstClockTime duration);

static void
gst_g729_dec_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&g729_dec_src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&g729_dec_sink_factory));
  gst_element_class_set_details (element_class, &g729_dec_details);
}

static void
gst_g729_dec_class_init (GstG729DecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_g729_dec_set_property;
  gobject_class->get_property = gst_g729_dec_get_property;

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (g729_dec_change_state);

  GST_DEBUG_CATEGORY_INIT (g729dec_debug, "g729dec", 0,
      "g729 decoding element");
}

static void
gst_g729_dec_reset (GstG729Dec * dec)
{
  gst_segment_init (&dec->segment, GST_FORMAT_UNDEFINED);
  dec->granulepos = -1;
  dec->packetno = 0;

  if (dec->state) {
    //TODO: uninitialize decoder
    dec->state = NULL;
  }

  //ref code specific
  Init_Decod_ld8a();
  Init_Post_Filter();
  Init_Post_Process();

  memset(dec->synth_buf,0,sizeof(dec->synth_buf));
  dec->synth = dec->synth_buf + M;
}

static void
gst_g729_dec_init (GstG729Dec * dec, GstG729DecClass * g_class)
{
  dec->sinkpad =
      gst_pad_new_from_static_template (&g729_dec_sink_factory, "sink");
  gst_pad_set_chain_function (dec->sinkpad,
      GST_DEBUG_FUNCPTR (g729_dec_chain));
  gst_pad_set_event_function (dec->sinkpad,
      GST_DEBUG_FUNCPTR (g729_dec_sink_event));
  gst_pad_set_query_type_function (dec->sinkpad,
      GST_DEBUG_FUNCPTR (g729_get_sink_query_types));
  gst_pad_set_query_function (dec->sinkpad,
      GST_DEBUG_FUNCPTR (g729_dec_sink_query));
  gst_element_add_pad (GST_ELEMENT (dec), dec->sinkpad);

  dec->srcpad =
      gst_pad_new_from_static_template (&g729_dec_src_factory, "src");
  gst_pad_use_fixed_caps (dec->srcpad);
  gst_pad_set_event_function (dec->srcpad,
      GST_DEBUG_FUNCPTR (g729_dec_src_event));
  gst_pad_set_query_type_function (dec->srcpad,
      GST_DEBUG_FUNCPTR (g729_get_src_query_types));
  gst_pad_set_query_function (dec->srcpad,
      GST_DEBUG_FUNCPTR (g729_dec_src_query));
  gst_element_add_pad (GST_ELEMENT (dec), dec->srcpad);

  gst_g729_dec_reset (dec);
}

static gboolean
g729_dec_convert (GstPad * pad,
    GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value)
{
  gboolean res = TRUE;
  GstG729Dec *dec;

  dec = GST_G729_DEC (gst_pad_get_parent (pad));

  if (dec->packetno < 1) {
    res = FALSE;
    goto cleanup;
  }

  if (src_format == *dest_format) {
    *dest_value = src_value;
    res = TRUE;
    goto cleanup;
  }

  if (pad == dec->sinkpad &&
      (src_format == GST_FORMAT_BYTES || *dest_format == GST_FORMAT_BYTES)) {
    res = FALSE;
    goto cleanup;
  }

  switch (src_format) {
    case GST_FORMAT_TIME:
      switch (*dest_format) {
        default:
          res = FALSE;
          break;
      }
      break;
    case GST_FORMAT_DEFAULT:
      switch (*dest_format) {
        default:
          res = FALSE;
          break;
      }
      break;
    case GST_FORMAT_BYTES:
      switch (*dest_format) {
        default:
          res = FALSE;
          break;
      }
      break;
    default:
      res = FALSE;
      break;
  }

cleanup:
  gst_object_unref (dec);
  return res;
}

static const GstQueryType *
g729_get_sink_query_types (GstPad * pad)
{
  static const GstQueryType g729_dec_sink_query_types[] = {
    GST_QUERY_CONVERT,
    0
  };

  return g729_dec_sink_query_types;
}

static gboolean
g729_dec_sink_query (GstPad * pad, GstQuery * query)
{
  GstG729Dec *dec;
  gboolean res;

  dec = GST_G729_DEC (gst_pad_get_parent (pad));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      res = g729_dec_convert (pad, src_fmt, src_val, &dest_fmt, &dest_val);
      if (res) {
        gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      }
      break;
    }
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

  gst_object_unref (dec);
  return res;
}

static const GstQueryType *
g729_get_src_query_types (GstPad * pad)
{
  static const GstQueryType g729_dec_src_query_types[] = {
    GST_QUERY_POSITION,
    GST_QUERY_DURATION,
    0
  };

  return g729_dec_src_query_types;
}

static gboolean
g729_dec_src_query (GstPad * pad, GstQuery * query)
{
  GstG729Dec *dec;
  gboolean res = FALSE;

  dec = GST_G729_DEC (gst_pad_get_parent (pad));

  switch (GST_QUERY_TYPE (query)) {
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

  gst_object_unref (dec);
  return res;
}

static gboolean
g729_dec_src_event (GstPad * pad, GstEvent * event)
{
  gboolean res = FALSE;
  GstG729Dec *dec = GST_G729_DEC (gst_pad_get_parent (pad));

  GST_LOG_OBJECT (dec, "handling %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      {
        GstFormat format, tformat;
        gdouble rate;
        GstEvent *real_seek;
        GstSeekFlags flags;
        GstSeekType cur_type, stop_type;
        gint64 cur, stop;
        gint64 tcur, tstop;

        gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur,
            &stop_type, &stop);

        /* we have to ask our peer to seek to time here as we know
         * nothing about how to generate a granulepos from the src
         * formats or anything.
         *
         * First bring the requested format to time
         */
        tformat = GST_FORMAT_TIME;
        if (!(res = g729_dec_convert (pad, format, cur, &tformat, &tcur)))
          break;
        if (!(res = g729_dec_convert (pad, format, stop, &tformat, &tstop)))
          break;

        /* then seek with time on the peer */
        real_seek = gst_event_new_seek (rate, GST_FORMAT_TIME,
            flags, cur_type, tcur, stop_type, tstop);

        GST_LOG_OBJECT (dec, "seek to %" GST_TIME_FORMAT, GST_TIME_ARGS (tcur));

        res = gst_pad_push_event (dec->sinkpad, real_seek);
        gst_event_unref (event);
        break;
      }
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (dec);
  return res;
}

static gboolean
g729_dec_sink_event (GstPad * pad, GstEvent * event)
{
  GstG729Dec *dec;
  gboolean ret = FALSE;

  dec = GST_G729_DEC (gst_pad_get_parent (pad));

  GST_LOG_OBJECT (dec, "handling %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NEWSEGMENT:{
      GstFormat format;
      gdouble rate, arate;
      gint64 start, stop, time;
      gboolean update;

      gst_event_parse_new_segment_full (event, &update, &rate, &arate, &format,
          &start, &stop, &time);

      if (format != GST_FORMAT_TIME)
        goto newseg_wrong_format;

      if (rate <= 0.0)
        goto newseg_wrong_rate;

      if (update) {
        /* time progressed without data, see if we can fill the gap with
         * some concealment data */
        if (dec->segment.last_stop < start) {
          GstClockTime duration;

          duration = start - dec->segment.last_stop;
          g729_dec_chain_parse_data (dec, NULL, dec->segment.last_stop,
              duration);
        }
      }

      /* now configure the values */
      gst_segment_set_newsegment_full (&dec->segment, update,
          rate, arate, GST_FORMAT_TIME, start, stop, time);

      dec->granulepos = -1;

      GST_DEBUG_OBJECT (dec, "segment now: cur = %" GST_TIME_FORMAT " [%"
          GST_TIME_FORMAT " - %" GST_TIME_FORMAT "]",
          GST_TIME_ARGS (dec->segment.last_stop),
          GST_TIME_ARGS (dec->segment.start),
          GST_TIME_ARGS (dec->segment.stop));

      ret = gst_pad_push_event (dec->srcpad, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (dec);
  return ret;

  /* ERRORS */
newseg_wrong_format:
  {
    GST_DEBUG_OBJECT (dec, "received non TIME newsegment");
    gst_object_unref (dec);
    return FALSE;
  }
newseg_wrong_rate:
  {
    GST_DEBUG_OBJECT (dec, "negative rates not supported yet");
    gst_object_unref (dec);
    return FALSE;
  }
}

static guint32 g729_decode(GstG729Dec * dec, guchar* g729_data, guchar* pcm_data)
{
  Word16  parm[PRM_SIZE+1];             /* Synthesis parameters        */
  Word16  serial[SERIAL_SIZE];          /* Serial stream               */
  Word16  Az_dec[MP1*2];                /* Decoded Az for post-filter  */
  Word16  T2[2];                        /* Pitch lag for 2 subframes   */

  Word16  i, frame;

  /*
   * One (2-byte) synchronization word
   * One (2-byte) size word
   * 80 words (2-byte) containing 80 bits.
   */
  ((guchar*)serial)[0]=0x21;
  ((guchar*)serial)[1]=0x6B;
  ((guchar*)serial)[2]=0x50;
  ((guchar*)serial)[3]=0x00;
  for(i=0;i<SERIAL_SIZE-2;i++){
    ((guchar*)serial)[5+i*2]=0x00;
    ((guchar*)serial)[4+i*2]=(g729_data[i/8]&(1<<(7-i%8)))!=0?0x81:0x7F;
  }

  bits2prm_ld8k( &serial[2], &parm[1]);

  parm[0] = 0;           /* No frame erasure */
  for (i=2; i < SERIAL_SIZE; i++)
    if (serial[i] == 0 ) parm[0] = 1; /* frame erased     */

  /* check pitch parity and put 1 in parm[4] if parity error */

  parm[4] = Check_Parity_Pitch(parm[3], parm[4]);

  Decod_ld8a(parm, dec->synth, Az_dec, T2);
  Post_Filter(dec->synth, Az_dec, T2);        /* Post-filter */
  Post_Process(dec->synth, L_FRAME);

  memcpy(pcm_data,dec->synth,RAW_FRAME_BYTES);

  return RAW_FRAME_BYTES;
}

static GstFlowReturn
g729_dec_chain_parse_data (GstG729Dec * dec, GstBuffer * buf,
    GstClockTime timestamp, GstClockTime duration)
{
  GstFlowReturn res = GST_FLOW_OK;
  //TODO: need an adapter here
  gint i, fpp=GST_BUFFER_SIZE(buf)/G729_FRAME_BYTES;
  guint size;
  guint8 *data;

  if(fpp*G729_FRAME_BYTES!=GST_BUFFER_SIZE(buf)){
    printf("throwing away %d bytes!\n",GST_BUFFER_SIZE(buf)-fpp*G729_FRAME_BYTES);
  }

  if (timestamp != -1) {
    dec->segment.last_stop = timestamp;
    dec->granulepos = -1;
  }

  if (buf) {
    data = GST_BUFFER_DATA (buf);
    size = GST_BUFFER_SIZE (buf);


    if (!GST_BUFFER_TIMESTAMP_IS_VALID (buf)
        && GST_BUFFER_OFFSET_END_IS_VALID (buf)) {
      dec->granulepos = GST_BUFFER_OFFSET_END (buf);
      GST_DEBUG_OBJECT (dec,
          "Taking granulepos from upstream: %" G_GUINT64_FORMAT,
          dec->granulepos);
    }

    /* copy timestamp */
  } else {
    /* concealment data, pass NULL as the bits parameters */
    GST_DEBUG_OBJECT (dec, "creating concealment data");
  }


  /* now decode each frame */
  for (i = 0; i < fpp; i++) {
    GstBuffer *outbuf;
    gint16 *out_data;

    res = gst_pad_alloc_buffer_and_set_caps (dec->srcpad,
        GST_BUFFER_OFFSET_NONE, RAW_FRAME_BYTES,
        GST_PAD_CAPS (dec->srcpad), &outbuf);

    if (res != GST_FLOW_OK) {
      GST_DEBUG_OBJECT (dec, "buf alloc flow: %s", gst_flow_get_name (res));
      return res;
    }

    out_data = (gint16 *) GST_BUFFER_DATA (outbuf);

    res = g729_decode (dec, data+i*G729_FRAME_BYTES, out_data);

    if (dec->granulepos == -1) {
      if (dec->segment.format != GST_FORMAT_TIME) {
        GST_WARNING_OBJECT (dec, "segment not initialized or not TIME format");
        dec->granulepos = RAW_FRAME_SAMPLES;
      } else {
        dec->granulepos = dec->segment.last_stop * SAMPLE_RATE / GST_SECOND + RAW_FRAME_SAMPLES;
      }
      GST_DEBUG_OBJECT (dec, "granulepos=%" G_GINT64_FORMAT, dec->granulepos);
    }

    GST_BUFFER_OFFSET (outbuf) = dec->granulepos - RAW_FRAME_SAMPLES;
    GST_BUFFER_OFFSET_END (outbuf) = dec->granulepos;
    GST_BUFFER_TIMESTAMP (outbuf) = timestamp;
    GST_BUFFER_DURATION (outbuf) = FRAME_DURATION;

    dec->granulepos += RAW_FRAME_SAMPLES;
    dec->segment.last_stop += FRAME_DURATION;

    GST_LOG_OBJECT (dec, "pushing buffer with ts=%" GST_TIME_FORMAT ", dur=%"
        GST_TIME_FORMAT, GST_TIME_ARGS (timestamp),
        GST_TIME_ARGS (FRAME_DURATION));

    res = gst_pad_push (dec->srcpad, outbuf);

    if (res != GST_FLOW_OK) {
      GST_DEBUG_OBJECT (dec, "flow: %s", gst_flow_get_name (res));
      break;
    }
    timestamp = -1;
  }

  return res;
}

static GstFlowReturn
g729_dec_chain (GstPad * pad, GstBuffer * buf)
{
  GstFlowReturn res;
  GstG729Dec *dec;

  dec = GST_G729_DEC (gst_pad_get_parent (pad));
  switch (dec->packetno) {
    case 0:
      {
        GstCaps *caps;
        caps = gst_caps_new_simple (
            "audio/x-raw-int",
            "rate", G_TYPE_INT, SAMPLE_RATE,
            "channels", G_TYPE_INT, 1,
            "signed", G_TYPE_BOOLEAN, TRUE,
            "endianness", G_TYPE_INT, G_BYTE_ORDER,
            "width", G_TYPE_INT, 16, "depth", G_TYPE_INT, 16, NULL);

        if(!gst_pad_set_caps (dec->srcpad, caps)){
          GST_ELEMENT_ERROR (GST_ELEMENT (dec), STREAM, DECODE,
              (NULL), ("couldn't negotiate format"));
          res=GST_FLOW_NOT_NEGOTIATED;
        }else{
          res=GST_FLOW_OK;
          gst_caps_unref (caps);
        }
      }
      break;
    default:
      res =
        g729_dec_chain_parse_data (dec, buf, GST_BUFFER_TIMESTAMP (buf),
            GST_BUFFER_DURATION (buf));
      break;
  }

  dec->packetno++;

  gst_buffer_unref (buf);
  gst_object_unref (dec);

  return res;
}

static void
gst_g729_dec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstG729Dec *g729dec;

  g729dec = GST_G729_DEC (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_g729_dec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstG729Dec *g729dec;

  g729dec = GST_G729_DEC (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static GstStateChangeReturn
g729_dec_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstG729Dec *dec = GST_G729_DEC (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    default:
      break;
  }

  ret = parent_class->change_state (element, transition);
  if (ret != GST_STATE_CHANGE_SUCCESS)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_g729_dec_reset (dec);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}
