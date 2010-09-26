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

#ifndef __GST_G729_ENC_H__
#define __GST_G729_ENC_H__


#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include "g729common.h"

G_BEGIN_DECLS

#define GST_TYPE_G729_ENC \
  (gst_g729_enc_get_type())
#define GST_G729_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_G729_ENC,GstG729Enc))
#define GST_G729_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_G729_ENC,GstG729EncClass))
#define GST_IS_G729_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_G729_ENC))
#define GST_IS_G729_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_G729_ENC))


typedef enum
{
  GST_G729_ENC_ANNEXA,
} GstG729Mode;

typedef struct _GstG729Enc GstG729Enc;
typedef struct _GstG729EncClass GstG729EncClass;

struct _GstG729Enc {
  GstElement            element;

  /* pads */
  GstPad                *sinkpad,
                        *srcpad;

  gint                  packet_count;
  gint                  n_packets;

  void                  *state;
  GstG729Mode          mode;
  GstAdapter            *adapter;

  gboolean              vad;
  gboolean              dtx;

  gboolean              setup;

  guint64               samples_in;
  guint64               bytes_out;

  GstTagList            *tags;

  gchar                 *last_message;

  guint64               frameno;
  guint64               frameno_out;

  guint8                *comments;
  gint                  comment_len;

  /* Timestamp and granulepos tracking */
  GstClockTime     start_ts;
  GstClockTime     next_ts;
  guint64          granulepos_offset;
};

struct _GstG729EncClass {
  GstElementClass parent_class;

  /* signals */
  void (*frame_encoded) (GstElement *element);
};

GType gst_g729_enc_get_type (void);

G_END_DECLS

#endif /* __GST_G729ENC_H__ */
