/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vaapiencpicture.h"
#include "vaapicodedbuffer.h"

#include "common/log.h"
#ifdef __BUILD_GET_MV__
#include <va/va_intel_fei.h>
#endif

namespace YamiMediaCodec{
VaapiEncPicture::VaapiEncPicture(const ContextPtr& context,
                                 const SurfacePtr & surface,
                                 int64_t timeStamp)
: VaapiPicture(context, surface, timeStamp)
, m_temporalID(0)
{
}

bool VaapiEncPicture::encode()
{
    return render();
}

bool VaapiEncPicture::doRender()
{
    RENDER_OBJECT(m_sequence);
    RENDER_OBJECT(m_packedHeaders);
    RENDER_OBJECT(m_miscParams);
    RENDER_OBJECT(m_picture);
    RENDER_OBJECT(m_qMatrix);
    RENDER_OBJECT(m_huffTable);
    RENDER_OBJECT(m_slices);
#ifdef __BUILD_GET_MV__
    RENDER_OBJECT(m_FEIBuffer);
#endif
    return true;
}

bool VaapiEncPicture::
addPackedHeader(VAEncPackedHeaderType packedHeaderType, const void *header,
                uint32_t headerBitSize)
{
    VAEncPackedHeaderParameterBuffer *packedHeader;
    BufObjectPtr param =
        createBufferObject(VAEncPackedHeaderParameterBufferType,
                           packedHeader);
    BufObjectPtr data =
        createBufferObject(VAEncPackedHeaderDataBufferType,
                           (headerBitSize + 7) / 8, header, NULL);
    bool ret = addObject(m_packedHeaders, param, data);
    if (ret && packedHeader) {
        packedHeader->type = packedHeaderType;
        packedHeader->bit_length = headerBitSize;
        packedHeader->has_emulation_bytes = 0;
        return true;
    }
    return false;
}

YamiStatus VaapiEncPicture::getOutput(VideoEncOutputBuffer* outBuffer)
{
    ASSERT(outBuffer);
    uint32_t size = m_codedBuffer->size();
    if (size > outBuffer->bufferSize) {
        outBuffer->dataSize = 0;
        return YAMI_ENCODE_BUFFER_TOO_SMALL;
    }
    if (size > 0) {
        m_codedBuffer->copyInto(outBuffer->data);
        outBuffer->flag |= m_codedBuffer->getFlags();
    }
    outBuffer->dataSize = size;
    return YAMI_SUCCESS;
}

#ifdef __BUILD_GET_MV__
bool VaapiEncPicture::editMVBuffer(void*& buffer, uint32_t *size)
{
    VABufferID bufID;
    VAEncMiscParameterFEIFrameControlH264Intel   fei_pic_param;
    VAEncMiscParameterFEIFrameControlH264Intel *misc_fei_pic_control_param;

    if (!m_MVBuffer) {
        m_MVBuffer = createBufferObject(VAEncFEIMVBufferTypeIntel, *size, NULL, (void**)&buffer);
        bufID = m_MVBuffer->getID();
        fei_pic_param.mv_data = bufID;
        fei_pic_param.function = VA_ENC_FUNCTION_ENC_PAK_INTEL;
        fei_pic_param.num_mv_predictors   = 1;

        if (!newMisc(VAEncMiscParameterTypeFEIFrameControlIntel, misc_fei_pic_control_param))
            return false;
        *misc_fei_pic_control_param = fei_pic_param;
    } else {
        buffer = m_MVBuffer->map();
        *size = m_MVBuffer->getSize();
    }
    return true;
}

#endif

}
