/* reamer
 * Copyright (C) <2012> Edward Hervey <edward@collabora.com>
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

#ifndef __MPEG_VIDEO_META_H__
#define __MPEG_VIDEO_META_H__

#ifndef USE_UNSTABLE_API
#warning "The Mpeg video parsing library is unstable API and may change in future."
#warning "You can define USE_UNSTABLE_API to avoid this warning."
#endif

#include "gst/gst.h"
#include "mpegvideoparser.h"

G_BEGIN_DECLS

typedef struct _MpegVideoMeta MpegVideoMeta;

GType mpeg_video_meta_api_get_type (void);
#define MPEG_VIDEO_META_API_TYPE  (mpeg_video_meta_api_get_type())
#define MPEG_VIDEO_META_INFO  (mpeg_video_meta_get_info())
const MetaInfo * mpeg_video_meta_get_info (void);

/**
 * MpegVideoMeta:
 * @meta: parent #Meta
 * @sequencehdr: the #MpegVideoSequenceHdr if present in the buffer
 * @sequenceext: the #MpegVideoSequenceExt if present in the buffer
 * @sequencedispext: the #MpegVideoSequenceDisplayExt if present in the
 * buffer.
 * @pichdr: the #MpegVideoPictureHdr if present in the buffer.
 * @picext: the #MpegVideoPictureExt if present in the buffer.
 * @quantext: the #MpegVideoQuantMatrixExt if present in the buffer
 *
 * Extra buffer metadata describing the contents of a MPEG1/2 Video frame
 *
 * Can be used by elements (mainly decoders) to avoid having to parse
 * Mpeg video 1/2 packets if it can be done upstream.
 *
 * The various fields are only valid during the lifetime of the #MpegVideoMeta.
 * If elements wish to use those for longer, they are required to make a copy.
 *
 * Since: 1.2
 */
struct _MpegVideoMeta {
  Meta            meta;

  MpegVideoSequenceHdr        *sequencehdr;
  MpegVideoSequenceExt        *sequenceext;
  MpegVideoSequenceDisplayExt *sequencedispext;
  MpegVideoPictureHdr         *pichdr;
  MpegVideoPictureExt         *picext;
  MpegVideoQuantMatrixExt     *quantext;

  uint32_t num_slices;
  size_t slice_offset;
};


#define buffer_get_mpeg_video_meta(b) ((MpegVideoMeta*)buffer_get_meta((b),MPEG_VIDEO_META_API_TYPE))

MpegVideoMeta *
buffer_add_mpeg_video_meta (Buffer * buffer, 
                                const MpegVideoSequenceHdr *seq_hdr,
                                const MpegVideoSequenceExt *seq_ext,
                                const MpegVideoSequenceDisplayExt *disp_ext,
                                const MpegVideoPictureHdr *pic_hdr,
                                const MpegVideoPictureExt *pic_ext,
                                const MpegVideoQuantMatrixExt *quant_ext);

G_END_DECLS

#endif
