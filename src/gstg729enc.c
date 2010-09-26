/* GladSToNe g729 Encoder
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
 * SECTION:element-g729enc
 * @see_also: g729dec
 *
 * This element encodes audio as a G729 stream.
 *
 * <refsect2>
 * <title>Example pipelines</title>
 * |[
 * TODO
 * ]| send a G729 rtp stream.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gst/gsttagsetter.h>
#include <gst/tag/tag.h>
#include <gst/audio/audio.h>
#include "gstg729enc.h"

#include "typedef.h"
#include "ld8a.h"

GST_DEBUG_CATEGORY_STATIC (g729enc_debug);
#define GST_CAT_DEFAULT g729enc_debug

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "rate = (int) 8000, "
        "channels = (int) 1, "
        "endianness = (int) BYTE_ORDER, "
        "signed = (boolean) TRUE, " "width = (int) 16, " "depth = (int) 16")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/G729, "
        "rate = (int) 8000, " "channels = (int) 1")
    );

static const GstElementDetails g729enc_details =
GST_ELEMENT_DETAILS ("g729enc",
    "Codec/Encoder/Audio",
    "Encodes audio in G729 format",
    "Gibrovacco <gibrovacco@gmail.com>");

#define DEFAULT_MODE            GST_G729_ENC_ANNEXA
#define DEFAULT_VAD             FALSE
#define DEFAULT_DTX             FALSE

enum
{
  PROP_0,
  PROP_MODE,
  PROP_VAD,
  PROP_DTX,
};

#define GST_TYPE_G729_ENC_MODE (gst_g729_enc_mode_get_type())
static GType
gst_g729_enc_mode_get_type (void)
{
  static GType g729_enc_mode_type = 0;
  static const GEnumValue g729_enc_modes[] = {
    {GST_G729_ENC_ANNEXA, "AnnexA", "annexa"},
    {0, NULL, NULL},
  };
  if (G_UNLIKELY (g729_enc_mode_type == 0)) {
    g729_enc_mode_type = g_enum_register_static ("GstG729EncMode",
        g729_enc_modes);
  }
  return g729_enc_mode_type;
}

static void gst_g729_enc_finalize (GObject * object);

static gboolean gst_g729_enc_sinkevent (GstPad * pad, GstEvent * event);
static GstFlowReturn gst_g729_enc_chain (GstPad * pad, GstBuffer * buf);
static gboolean gst_g729_enc_setup (GstG729Enc * enc);

static void gst_g729_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_g729_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_g729_enc_change_state (GstElement * element,
    GstStateChange transition);

static GstFlowReturn gst_g729_enc_encode (GstG729Enc * enc, gboolean flush);

static void
gst_g729_enc_setup_interfaces (GType g729enc_type)
{
  static const GInterfaceInfo tag_setter_info = { NULL, NULL, NULL };

  g_type_add_interface_static (g729enc_type, GST_TYPE_TAG_SETTER,
      &tag_setter_info);

  GST_DEBUG_CATEGORY_INIT (g729enc_debug, "g729enc", 0, "G729 encoder");
}

GST_BOILERPLATE_FULL (GstG729Enc, gst_g729_enc, GstElement, GST_TYPE_ELEMENT,
    gst_g729_enc_setup_interfaces);

static void
gst_g729_enc_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
  gst_element_class_set_details (element_class, &g729enc_details);
}

static void
gst_g729_enc_class_init (GstG729EncClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_g729_enc_set_property;
  gobject_class->get_property = gst_g729_enc_get_property;

  g_object_class_install_property (gobject_class, PROP_MODE,
      g_param_spec_enum ("mode", "Mode", "The encoding mode",
          GST_TYPE_G729_ENC_MODE, GST_G729_ENC_ANNEXA,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_VAD,
      g_param_spec_boolean ("vad", "VAD",
          "Enable voice activity detection", DEFAULT_VAD, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DTX,
      g_param_spec_boolean ("dtx", "DTX",
          "Enable discontinuous transmission", DEFAULT_DTX, G_PARAM_READWRITE));

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_g729_enc_finalize);

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_g729_enc_change_state);
}

static void
gst_g729_enc_finalize (GObject * object)
{
  GstG729Enc *enc;

  enc = GST_G729_ENC (object);

  g_free (enc->last_message);
  g_object_unref (enc->adapter);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_g729_enc_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstG729Enc *enc;
  GstStructure *structure;

  enc = GST_G729_ENC (GST_PAD_PARENT (pad));
  enc->setup = FALSE;

  structure = gst_caps_get_structure (caps, 0);

  gst_g729_enc_setup (enc);

  return enc->setup;
}


static GstCaps *
gst_g729_enc_sink_getcaps (GstPad * pad)
{
  GstCaps *caps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));
  GstCaps *peercaps = NULL;
  GstG729Enc *enc = GST_G729_ENC (gst_pad_get_parent_element (pad));

  peercaps = gst_pad_peer_get_caps (enc->srcpad);

  if (peercaps) {
    if (!gst_caps_is_empty (peercaps) && !gst_caps_is_any (peercaps)) {
    }
    gst_caps_unref (peercaps);
  }

  gst_object_unref (enc);

  return caps;
}


static gboolean
gst_g729_enc_convert_src (GstPad * pad, GstFormat src_format, gint64 src_value,
    GstFormat * dest_format, gint64 * dest_value)
{
  gboolean res = TRUE;
  GstG729Enc *enc;
  gint64 avg;

  enc = GST_G729_ENC (GST_PAD_PARENT (pad));

  if (enc->samples_in == 0 || enc->bytes_out == 0 )
    return FALSE;

  avg = (enc->bytes_out ) / (enc->samples_in);

  switch (src_format) {
    case GST_FORMAT_BYTES:
      switch (*dest_format) {
        case GST_FORMAT_TIME:
          *dest_value = src_value * GST_SECOND / avg;
          break;
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_TIME:
      switch (*dest_format) {
        case GST_FORMAT_BYTES:
          *dest_value = src_value * avg / GST_SECOND;
          break;
        default:
          res = FALSE;
      }
      break;
    default:
      res = FALSE;
  }
  return res;
}

static gboolean
gst_g729_enc_convert_sink (GstPad * pad, GstFormat src_format,
    gint64 src_value, GstFormat * dest_format, gint64 * dest_value)
{
  gboolean res = TRUE;
  guint scale = 1;
  gint bytes_per_sample=2;
  GstG729Enc *enc;

  enc = GST_G729_ENC (GST_PAD_PARENT (pad));

  switch (src_format) {
    case GST_FORMAT_BYTES:
      switch (*dest_format) {
        case GST_FORMAT_DEFAULT:
          if (bytes_per_sample == 0)
            return FALSE;
          *dest_value = src_value / bytes_per_sample;
          break;
        case GST_FORMAT_TIME:
        {
          gint byterate = bytes_per_sample * SAMPLE_RATE;

          if (byterate == 0)
            return FALSE;
          *dest_value = src_value * GST_SECOND / byterate;
          break;
        }
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_DEFAULT:
      switch (*dest_format) {
        case GST_FORMAT_BYTES:
          *dest_value = src_value * bytes_per_sample;
          break;
        case GST_FORMAT_TIME:
          *dest_value = src_value * GST_SECOND / SAMPLE_RATE;
          break;
        default:
          res = FALSE;
      }
      break;
    case GST_FORMAT_TIME:
      switch (*dest_format) {
        case GST_FORMAT_BYTES:
          scale = bytes_per_sample;
          /* fallthrough */
        case GST_FORMAT_DEFAULT:
          *dest_value = src_value * scale * SAMPLE_RATE / GST_SECOND;
          break;
        default:
          res = FALSE;
      }
      break;
    default:
      res = FALSE;
  }
  return res;
}

static gint64
gst_g729_enc_get_latency (GstG729Enc * enc)
{
  return 30 * GST_MSECOND;
}

static const GstQueryType *
gst_g729_enc_get_query_types (GstPad * pad)
{
  static const GstQueryType gst_g729_enc_src_query_types[] = {
    GST_QUERY_POSITION,
    GST_QUERY_DURATION,
    GST_QUERY_CONVERT,
    GST_QUERY_LATENCY,
    0
  };

  return gst_g729_enc_src_query_types;
}

static gboolean
gst_g729_enc_src_query (GstPad * pad, GstQuery * query)
{
  gboolean res = TRUE;
  GstG729Enc *enc;

  enc = GST_G729_ENC (gst_pad_get_parent (pad));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
    {
      GstFormat fmt, req_fmt;
      gint64 pos, val;

      gst_query_parse_position (query, &req_fmt, NULL);
      if ((res = gst_pad_query_peer_position (enc->sinkpad, &req_fmt, &val))) {
        gst_query_set_position (query, req_fmt, val);
        break;
      }

      fmt = GST_FORMAT_TIME;
      if (!(res = gst_pad_query_peer_position (enc->sinkpad, &fmt, &pos)))
        break;

      if ((res =
              gst_pad_query_peer_convert (enc->sinkpad, fmt, pos, &req_fmt,
                  &val)))
        gst_query_set_position (query, req_fmt, val);

      break;
    }
    case GST_QUERY_DURATION:
    {
      GstFormat fmt, req_fmt;
      gint64 dur, val;

      gst_query_parse_duration (query, &req_fmt, NULL);
      if ((res = gst_pad_query_peer_duration (enc->sinkpad, &req_fmt, &val))) {
        gst_query_set_duration (query, req_fmt, val);
        break;
      }

      fmt = GST_FORMAT_TIME;
      if (!(res = gst_pad_query_peer_duration (enc->sinkpad, &fmt, &dur)))
        break;

      if ((res =
              gst_pad_query_peer_convert (enc->sinkpad, fmt, dur, &req_fmt,
                  &val))) {
        gst_query_set_duration (query, req_fmt, val);
      }
      break;
    }
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (!(res = gst_g729_enc_convert_src (pad, src_fmt, src_val, &dest_fmt,
                  &dest_val)))
        goto error;
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      break;
    }
    case GST_QUERY_LATENCY:
    {
      gboolean live;
      GstClockTime min_latency, max_latency;
      gint64 latency;

      if ((res = gst_pad_peer_query (pad, query))) {
        gst_query_parse_latency (query, &live, &min_latency, &max_latency);

        latency = gst_g729_enc_get_latency (enc);

        /* add our latency */
        min_latency += latency;
        if (max_latency != -1)
          max_latency += latency;

        gst_query_set_latency (query, live, min_latency, max_latency);
      }
      break;
    }
    default:
      res = gst_pad_peer_query (pad, query);
      break;
  }

error:

  gst_object_unref (enc);

  return res;
}

static gboolean
gst_g729_enc_sink_query (GstPad * pad, GstQuery * query)
{
  gboolean res = TRUE;
  GstG729Enc *enc;

  enc = GST_G729_ENC (GST_PAD_PARENT (pad));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (!(res =
              gst_g729_enc_convert_sink (pad, src_fmt, src_val, &dest_fmt,
                  &dest_val)))
        goto error;
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      break;
    }
    default:
      res = gst_pad_query_default (pad, query);
      break;
  }

error:
  return res;
}

static void
gst_g729_enc_init (GstG729Enc * enc, GstG729EncClass * klass)
{
  enc->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_element_add_pad (GST_ELEMENT (enc), enc->sinkpad);
  gst_pad_set_event_function (enc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_g729_enc_sinkevent));
  gst_pad_set_chain_function (enc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_g729_enc_chain));
  gst_pad_set_setcaps_function (enc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_g729_enc_sink_setcaps));
  gst_pad_set_getcaps_function (enc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_g729_enc_sink_getcaps));
  gst_pad_set_query_function (enc->sinkpad,
      GST_DEBUG_FUNCPTR (gst_g729_enc_sink_query));

  enc->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_query_function (enc->srcpad,
      GST_DEBUG_FUNCPTR (gst_g729_enc_src_query));
  gst_pad_set_query_type_function (enc->srcpad,
      GST_DEBUG_FUNCPTR (gst_g729_enc_get_query_types));
  gst_element_add_pad (GST_ELEMENT (enc), enc->srcpad);

  enc->mode = DEFAULT_MODE;
  enc->vad = DEFAULT_VAD;
  enc->dtx = DEFAULT_DTX;

  enc->setup = FALSE;

  enc->adapter = gst_adapter_new ();

  Init_Pre_Process();
  Init_Coder_ld8a();

}

static void
gst_g729_enc_set_last_msg (GstG729Enc * enc, const gchar * msg)
{
  g_free (enc->last_message);
  enc->last_message = g_strdup (msg);
  GST_WARNING_OBJECT (enc, "%s", msg);
  g_object_notify (G_OBJECT (enc), "last-message");
}

static gboolean
gst_g729_enc_setup (GstG729Enc * enc)
{
  enc->setup = FALSE;

  switch (enc->mode) {
    case GST_G729_ENC_ANNEXA:
      GST_LOG_OBJECT (enc, "Annex A mode");
    default:
      break;
  }

  /*Initialize G729 encoder */

  if (enc->vad) {
    //TODO
  }

  if (enc->dtx) {
    //TODO
  }

  if (enc->dtx && !enc->vad) {
    gst_g729_enc_set_last_msg (enc,
        "Warning: dtx is useless without vad, vbr or abr");
  }

  enc->setup = TRUE;

  return TRUE;
}

/* push out the buffer and do internal bookkeeping */
static GstFlowReturn
gst_g729_enc_push_buffer (GstG729Enc * enc, GstBuffer * buffer)
{
  guint size;

  size = GST_BUFFER_SIZE (buffer);

  enc->bytes_out += size;

  GST_DEBUG_OBJECT (enc, "pushing output buffer of size %u", size);

  return gst_pad_push (enc->srcpad, buffer);
}

static gboolean
gst_g729_enc_sinkevent (GstPad * pad, GstEvent * event)
{
  gboolean res = TRUE;
  GstG729Enc *enc;

  enc = GST_G729_ENC (gst_pad_get_parent (pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      gst_g729_enc_encode (enc, TRUE);
      res = gst_pad_event_default (pad, event);
      break;
    case GST_EVENT_TAG:
    {
      GstTagList *list;

      gst_event_parse_tag (event, &list);
      if (enc->tags) {
        gst_tag_list_insert (enc->tags, list,
            gst_tag_setter_get_tag_merge_mode (GST_TAG_SETTER (enc)));
      }
      res = gst_pad_event_default (pad, event);
      break;
    }
    default:
      res = gst_pad_event_default (pad, event);
      break;
  }

  gst_object_unref (enc);

  return res;
}

static gint g729_encode_frame (GstG729Enc* filter, gint16* in, gchar* out){
  int i,j,index;
  extern Word16 *new_speech;
  Word16 prm[PRM_SIZE],serial[SERIAL_SIZE];

  memcpy(new_speech,in,RAW_FRAME_BYTES);

  Pre_Process(new_speech,L_FRAME);
  Coder_ld8a(prm);
  prm2bits_ld8k(prm, serial);

  memset (out,0x0,G729_FRAME_BYTES);

  for(i=0;i<10;i++){
    for(j=0;j<8;j++){
      index=2+(i*8)+j;
      out[i]|=serial[index]==0x81?(1<<(7-j)):0;
    }
  }

  return G729_FRAME_BYTES;
}

static GstFlowReturn
gst_g729_enc_encode (GstG729Enc * enc, gboolean flush)
{
  GstFlowReturn ret = GST_FLOW_OK;

  if (flush && gst_adapter_available (enc->adapter) % RAW_FRAME_BYTES != 0) {
    guint diff = gst_adapter_available (enc->adapter) % RAW_FRAME_BYTES;
    GstBuffer *buf = gst_buffer_new_and_alloc (diff);

    memset (GST_BUFFER_DATA (buf), 0, diff);
    gst_adapter_push (enc->adapter, buf);
  }

  while (gst_adapter_available (enc->adapter) >= RAW_FRAME_BYTES) {
    gint16 *data=NULL;
    gint out=0;
    GstBuffer *outbuf;

    data = (gint16 *) gst_adapter_take (enc->adapter, RAW_FRAME_BYTES);

    enc->samples_in += RAW_FRAME_SAMPLES;


    enc->frameno++;
    enc->frameno_out++;

    ret = gst_pad_alloc_buffer_and_set_caps (enc->srcpad,
        GST_BUFFER_OFFSET_NONE, G729_FRAME_BYTES, GST_PAD_CAPS (enc->srcpad), &outbuf);

    if ((GST_FLOW_OK != ret))
      goto done;

    out = g729_encode_frame (enc, data, (gchar *) GST_BUFFER_DATA (outbuf));

    g_free (data);
    g_assert (out == G729_FRAME_BYTES);

    GST_BUFFER_TIMESTAMP (outbuf) = enc->start_ts +
        gst_util_uint64_scale_int ((enc->frameno_out - 1) * RAW_FRAME_SAMPLES,
        GST_SECOND, SAMPLE_RATE);
    GST_BUFFER_DURATION (outbuf) = gst_util_uint64_scale_int (RAW_FRAME_SAMPLES,
        GST_SECOND, SAMPLE_RATE);
    GST_BUFFER_OFFSET_END (outbuf) = enc->granulepos_offset +
        ((enc->frameno_out) * RAW_FRAME_SAMPLES);
    GST_BUFFER_OFFSET (outbuf) =
        gst_util_uint64_scale_int (GST_BUFFER_OFFSET_END (outbuf), GST_SECOND,
        SAMPLE_RATE);

    ret = gst_g729_enc_push_buffer (enc, outbuf);

    if ((GST_FLOW_OK != ret) && (GST_FLOW_NOT_LINKED != ret))
      goto done;
  }

done:

  return ret;
}

static GstFlowReturn
gst_g729_enc_chain (GstPad * pad, GstBuffer * buf)
{
  GstG729Enc *enc;
  GstFlowReturn ret = GST_FLOW_OK;

  enc = GST_G729_ENC (GST_PAD_PARENT (pad));

  if (!enc->setup)
    goto not_setup;

  /* Save the timestamp of the first buffer. This will be later
   * used as offset for all following buffers */
  if (enc->start_ts == GST_CLOCK_TIME_NONE) {
    if (GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
      enc->start_ts = GST_BUFFER_TIMESTAMP (buf);
      enc->granulepos_offset = gst_util_uint64_scale
          (GST_BUFFER_TIMESTAMP (buf), SAMPLE_RATE, GST_SECOND);
    } else {
      enc->start_ts = 0;
      enc->granulepos_offset = 0;
    }
  }

  /* Check if we have a continous stream, if not drop some samples or the buffer or
   * insert some silence samples */
  if (enc->next_ts != GST_CLOCK_TIME_NONE &&
      GST_BUFFER_TIMESTAMP (buf) < enc->next_ts) {
    guint64 diff = enc->next_ts - GST_BUFFER_TIMESTAMP (buf);
    guint64 diff_bytes;

    GST_WARNING_OBJECT (enc, "Buffer is older than previous "
        "timestamp + duration (%" GST_TIME_FORMAT "< %" GST_TIME_FORMAT
        "), cannot handle. Clipping buffer.",
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
        GST_TIME_ARGS (enc->next_ts));

    diff_bytes = GST_CLOCK_TIME_TO_FRAMES (diff, SAMPLE_RATE) * 2;
    if (diff_bytes >= GST_BUFFER_SIZE (buf)) {
      gst_buffer_unref (buf);
      return GST_FLOW_OK;
    }
    buf = gst_buffer_make_metadata_writable (buf);
    GST_BUFFER_DATA (buf) += diff_bytes;
    GST_BUFFER_SIZE (buf) -= diff_bytes;

    GST_BUFFER_TIMESTAMP (buf) += diff;
    if (GST_BUFFER_DURATION_IS_VALID (buf))
      GST_BUFFER_DURATION (buf) -= diff;
  }

  if (enc->next_ts != GST_CLOCK_TIME_NONE
      && GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
    guint64 max_diff =
        gst_util_uint64_scale (RAW_FRAME_SAMPLES, GST_SECOND, SAMPLE_RATE);

    if (GST_BUFFER_TIMESTAMP (buf) != enc->next_ts &&
        GST_BUFFER_TIMESTAMP (buf) - enc->next_ts > max_diff) {
      GST_WARNING_OBJECT (enc,
          "Discontinuity detected: %" G_GUINT64_FORMAT " > %" G_GUINT64_FORMAT,
          GST_BUFFER_TIMESTAMP (buf) - enc->next_ts, max_diff);

      gst_g729_enc_encode (enc, TRUE);

      enc->frameno_out = 0;
      enc->start_ts = GST_BUFFER_TIMESTAMP (buf);
      enc->granulepos_offset = gst_util_uint64_scale
          (GST_BUFFER_TIMESTAMP (buf), SAMPLE_RATE, GST_SECOND);
    }
  }

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buf)
      && GST_BUFFER_DURATION_IS_VALID (buf))
    enc->next_ts = GST_BUFFER_TIMESTAMP (buf) + GST_BUFFER_DURATION (buf);
  else
    enc->next_ts = GST_CLOCK_TIME_NONE;

  GST_DEBUG_OBJECT (enc, "received buffer of %u bytes", GST_BUFFER_SIZE (buf));

  /* push buffer to adapter */
  gst_adapter_push (enc->adapter, buf);
  buf = NULL;

  ret = gst_g729_enc_encode (enc, FALSE);

done:

  if (buf)
    gst_buffer_unref (buf);

  return ret;

  /* ERRORS */
not_setup:
  {
    GST_ELEMENT_ERROR (enc, CORE, NEGOTIATION, (NULL),
        ("encoder not initialized (input is not audio?)"));
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto done;
  }

}


static void
gst_g729_enc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstG729Enc *enc;

  enc = GST_G729_ENC (object);

  switch (prop_id) {
    case PROP_MODE:
      g_value_set_enum (value, enc->mode);
      break;
    case PROP_VAD:
      g_value_set_boolean (value, enc->vad);
      break;
    case PROP_DTX:
      g_value_set_boolean (value, enc->dtx);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_g729_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstG729Enc *enc;

  enc = GST_G729_ENC (object);

  switch (prop_id) {
    case PROP_MODE:
      enc->mode = g_value_get_enum (value);
      break;
    case PROP_VAD:
      enc->vad = g_value_get_boolean (value);
      break;
    case PROP_DTX:
      enc->dtx = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_g729_enc_change_state (GstElement * element, GstStateChange transition)
{
  GstG729Enc *enc = GST_G729_ENC (element);
  GstStateChangeReturn res;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      enc->tags = gst_tag_list_new ();
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      //g729_bits_init (&enc->bits);
      enc->frameno = 0;
      enc->frameno_out = 0;
      enc->samples_in = 0;
      enc->start_ts = GST_CLOCK_TIME_NONE;
      enc->next_ts = GST_CLOCK_TIME_NONE;
      enc->granulepos_offset = 0;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      /* fall through */
    default:
      break;
  }

  res = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (res == GST_STATE_CHANGE_FAILURE)
    return res;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      enc->setup = FALSE;
      if (enc->state) {
        //g729_encoder_destroy (enc->state);
        enc->state = NULL;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_tag_list_free (enc->tags);
      enc->tags = NULL;
    default:
      break;
  }

  return res;
}
