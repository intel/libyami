/*
 * reamer
 * Copyright (C) 2012 Edward Hervey <edward@collabora.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mpegvideometa.h"

DEBUG_CATEGORY (mpegv_meta_debug);
#define CAT_DEFAULT mpegv_meta_debug

static bool
mpeg_video_meta_init (MpegVideoMeta * mpeg_video_meta,
    void* params, Buffer * buffer)
{
  mpeg_video_meta->sequencehdr = NULL;
  mpeg_video_meta->sequenceext = NULL;
  mpeg_video_meta->sequencedispext = NULL;
  mpeg_video_meta->pichdr = NULL;
  mpeg_video_meta->picext = NULL;
  mpeg_video_meta->quantext = NULL;

  return TRUE;
}

static void
mpeg_video_meta_free (MpegVideoMeta * mpeg_video_meta,
    Buffer * buffer)
{
  if (mpeg_video_meta->sequencehdr)
    g_slice_free (MpegVideoSequenceHdr, mpeg_video_meta->sequencehdr);
  if (mpeg_video_meta->sequenceext)
    g_slice_free (MpegVideoSequenceExt, mpeg_video_meta->sequenceext);
  if (mpeg_video_meta->sequencedispext)
    g_slice_free (MpegVideoSequenceDisplayExt,
        mpeg_video_meta->sequencedispext);
  if (mpeg_video_meta->pichdr)
    g_slice_free (MpegVideoPictureHdr, mpeg_video_meta->pichdr);
  if (mpeg_video_meta->picext)
    g_slice_free (MpegVideoPictureExt, mpeg_video_meta->picext);
  if (mpeg_video_meta->quantext)
    g_slice_free (MpegVideoQuantMatrixExt, mpeg_video_meta->quantext);
}

GType
mpeg_video_meta_api_get_type (void)
{
  static volatile GType type;
  static const char *tags[] = { "memory", NULL };      /* don't know what to set here */

  if (g_once_init_enter (&type)) {
    GType _type = meta_api_type_register ("MpegVideoMetaAPI", tags);
    DEBUG_CATEGORY_INIT (mpegv_meta_debug, "mpegvideometa", 0,
        "MPEG-1/2 video Meta");

    g_once_init_leave (&type, _type);
  }
  return type;
}

const MetaInfo *
mpeg_video_meta_get_info (void)
{
  static const MetaInfo *mpeg_video_meta_info = NULL;

  if (g_once_init_enter (&mpeg_video_meta_info)) {
    const MetaInfo *meta = meta_register (MPEG_VIDEO_META_API_TYPE,
        "MpegVideoMeta", sizeof (MpegVideoMeta),
        (MetaInitFunction) mpeg_video_meta_init,
        (MetaFreeFunction) mpeg_video_meta_free,
        (MetaTransformFunction) NULL);
    g_once_init_leave (&mpeg_video_meta_info, meta);
  }

  return mpeg_video_meta_info;
}

/**
 * buffer_add_mpeg_video_meta:
 * @buffer: a #Buffer
 *
 * Creates and adds a #MpegVideoMeta to a @buffer.
 *
 * Provided structures must either be %NULL or GSlice-allocated.
 *
 * Returns: (transfer full): a newly created #MpegVideoMeta
 *
 * Since: 1.2
 */
MpegVideoMeta *
buffer_add_mpeg_video_meta (Buffer * buffer,
    const MpegVideoSequenceHdr * seq_hdr,
    const MpegVideoSequenceExt * seq_ext,
    const MpegVideoSequenceDisplayExt * disp_ext,
    const MpegVideoPictureHdr * pic_hdr,
    const MpegVideoPictureExt * pic_ext,
    const MpegVideoQuantMatrixExt * quant_ext)
{
  MpegVideoMeta *mpeg_video_meta;

  mpeg_video_meta =
      (MpegVideoMeta *) buffer_add_meta (buffer,
      MPEG_VIDEO_META_INFO, NULL);

  DEBUG
      ("seq_hdr:%p, seq_ext:%p, disp_ext:%p, pic_hdr:%p, pic_ext:%p, quant_ext:%p",
      seq_hdr, seq_ext, disp_ext, pic_hdr, pic_ext, quant_ext);

  if (seq_hdr)
    mpeg_video_meta->sequencehdr =
        g_slice_dup (MpegVideoSequenceHdr, seq_hdr);
  if (seq_ext)
    mpeg_video_meta->sequenceext =
        g_slice_dup (MpegVideoSequenceExt, seq_ext);
  if (disp_ext)
    mpeg_video_meta->sequencedispext =
        g_slice_dup (MpegVideoSequenceDisplayExt, disp_ext);
  mpeg_video_meta->pichdr = g_slice_dup (MpegVideoPictureHdr, pic_hdr);
  if (pic_ext)
    mpeg_video_meta->picext = g_slice_dup (MpegVideoPictureExt, pic_ext);
  if (quant_ext)
    mpeg_video_meta->quantext =
        g_slice_dup (MpegVideoQuantMatrixExt, quant_ext);

  return mpeg_video_meta;
}
