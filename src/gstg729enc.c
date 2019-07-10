/* GladSToNe g729 Encoder
 * Copyright (C) <2009> Gibrovacco <gibrovacco@gmail.com>
 * Copyright (C) 2016 Sebastian Dröge <sebastian@centricular.com>
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

#include "gstg729enc.h"
#include <string.h>

void Init_Cod_cng(void);

GST_DEBUG_CATEGORY_EXTERN (g729enc_debug);
#define GST_CAT_DEFAULT g729enc_debug

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string)" GST_AUDIO_NE (S16) ", "
        "rate = (int) 8000, "
        "channels = (int) 1, "
        "layout = (string) interleaved")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/G729, "
        "rate = (int) 8000, " "channels = (int) 1")
    );

#define DEFAULT_VAD             FALSE

enum
{
  PROP_0,
  PROP_VAD,
};

static void gst_g729_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_g729_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_g729_enc_handle_frame (GstAudioEncoder * aenc, GstBuffer * buffer);
static gboolean gst_g729_enc_set_format (GstAudioEncoder * aenc, GstAudioInfo * info);
static gboolean gst_g729_enc_stop (GstAudioEncoder * aenc);

G_DEFINE_TYPE (GstG729Enc, gst_g729_enc, GST_TYPE_AUDIO_ENCODER);

static void
gst_g729_enc_class_init (GstG729EncClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstAudioEncoderClass *gstaudioencoder_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstaudioencoder_class = (GstAudioEncoderClass *) klass;

  gobject_class->set_property = gst_g729_enc_set_property;
  gobject_class->get_property = gst_g729_enc_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_VAD,
      g_param_spec_boolean ("vad", "VAD",
          "Enable voice activity detection", DEFAULT_VAD, G_PARAM_READWRITE));

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
  gst_element_class_set_static_metadata (gstelement_class, "G729 audio encoder",
    "Codec/Encoder/Audio",
    "Encodes audio in G729 format",
    "Gibrovacco <gibrovacco@gmail.com>");

  gstaudioencoder_class->handle_frame = GST_DEBUG_FUNCPTR (gst_g729_enc_handle_frame);
  gstaudioencoder_class->set_format = GST_DEBUG_FUNCPTR (gst_g729_enc_set_format);
  gstaudioencoder_class->stop = GST_DEBUG_FUNCPTR (gst_g729_enc_stop);
}

static void
gst_g729_enc_init (GstG729Enc * enc)
{
  gst_audio_encoder_set_drainable (GST_AUDIO_ENCODER (enc), FALSE);
  gst_audio_encoder_set_latency (GST_AUDIO_ENCODER (enc), 30 * GST_MSECOND, 30 * GST_MSECOND);

  enc->vad = DEFAULT_VAD;

  Init_Pre_Process();
  Init_Coder_ld8a();
  Init_Cod_cng();
}

static gboolean
gst_g729_enc_stop (GstAudioEncoder * aenc)
{
  GstG729Enc* enc = GST_G729_ENC (aenc);

  enc->frameno = 0;

  return TRUE;
}

static gboolean
gst_g729_enc_set_format (GstAudioEncoder * aenc, GstAudioInfo * info)
{
  GstCaps *caps;
  gboolean ret;

  gst_audio_encoder_set_frame_max (aenc, 1);
  gst_audio_encoder_set_frame_samples_min (aenc, RAW_FRAME_SAMPLES);
  gst_audio_encoder_set_frame_samples_max (aenc, RAW_FRAME_SAMPLES);
  gst_audio_encoder_set_hard_min (aenc, TRUE);

  caps = gst_pad_get_pad_template_caps (GST_AUDIO_ENCODER_SRC_PAD (aenc));
  ret = gst_audio_encoder_set_output_format (aenc, caps);
  gst_caps_unref (caps);

  return ret;
}

static gint g729_encode_frame (GstG729Enc* enc, const gint16* in, guint8* out){
  int i,j,index;
  extern Word16 *new_speech;
  gint ret = G729_FRAME_BYTES;

  memcpy(new_speech,in,RAW_FRAME_BYTES);

  Pre_Process(new_speech,L_FRAME);
  Coder_ld8a(enc->parameters, enc->frameno, enc->vad);
  prm2bits_ld8k(enc->parameters, enc->encoder_output);

  memset (out,0x0,G729_FRAME_BYTES);

  for(i=0;i<10;i++){
    for(j=0;j<8;j++){
      index=2+(i*8)+j;
      out[i]|=enc->encoder_output[index]==0x81?(1<<(7-j)):0;
    }
  }

  switch (enc->parameters[0]){
    case G729_SID_FRAME:
      GST_DEBUG_OBJECT (enc, "SID detected");
      ret = G729_SID_BYTES;
      break;
    case G729_SILENCE_FRAME:
      GST_DEBUG_OBJECT (enc, "No-transmission detected");
      ret = G729_SILENCE_BYTES;
      break;
  }

  return ret;
}

static GstFlowReturn
gst_g729_enc_handle_frame (GstAudioEncoder * aenc, GstBuffer * buf)
{
  GstG729Enc* enc = GST_G729_ENC (aenc);
  GstFlowReturn ret = GST_FLOW_OK;
  GstMapInfo imap, omap;
  GstBuffer *outbuf;
  gint out;

  outbuf = gst_audio_encoder_allocate_output_buffer (GST_AUDIO_ENCODER (enc), G729_FRAME_BYTES);

  if (!outbuf)
    goto done;

  if (enc->frameno == 32767){
    enc->frameno = 256;
  } else {
    enc->frameno++;
  }

  gst_buffer_map (buf, &imap, GST_MAP_READ);
  gst_buffer_map (outbuf, &omap, GST_MAP_WRITE);

  out = g729_encode_frame (enc, (const gint16 *) imap.data, omap.data);

  gst_buffer_unmap (buf, &imap);
  gst_buffer_unmap (outbuf, &omap);

  if(out == G729_SILENCE_BYTES){
    gst_buffer_unref(outbuf);
    outbuf = NULL;
  } else {
    gst_buffer_set_size (outbuf, out);
  }

  ret = gst_audio_encoder_finish_frame (GST_AUDIO_ENCODER (enc), outbuf, RAW_FRAME_SAMPLES);

done:

  return ret;
}

static void
gst_g729_enc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstG729Enc *enc;

  enc = GST_G729_ENC (object);

  switch (prop_id) {
    case PROP_VAD:
      g_value_set_boolean (value, enc->vad);
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
    case PROP_VAD:
      enc->vad = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

