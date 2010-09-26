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


#ifndef __GST_G729_DEC_H__
#define __GST_G729_DEC_H__

#include <gst/gst.h>
#include "g729common.h"

//ref code includes:
#include "typedef.h"
#include "ld8a.h"
#include "basic_op.h"

G_BEGIN_DECLS

#define GST_TYPE_G729_DEC \
  (gst_g729_dec_get_type())
#define GST_G729_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_G729_DEC,GstG729Dec))
#define GST_G729_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_G729_DEC,GstG729DecClass))
#define GST_IS_G729_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_G729_DEC))
#define GST_IS_G729_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_G729_DEC))

typedef struct _GstG729Dec GstG729Dec;
typedef struct _GstG729DecClass GstG729DecClass;

struct _GstG729Dec {
  GstElement            element;

  /* pads */
  GstPad                *sinkpad;
  GstPad                *srcpad;

  void                  *state;

  guint64               packetno;

  GstSegment            segment;    /* STREAM LOCK */
  gint64                granulepos; /* -1 = needs to be set from current time */

  //ref code specific
  Word16 synth_buf[SERIAL_SIZE+M];
  Word16 *synth; 

};

struct _GstG729DecClass {
  GstElementClass parent_class;
};

GType gst_g729_dec_get_type (void);

G_END_DECLS

#endif /* __GST_G729_DEC_H__ */
