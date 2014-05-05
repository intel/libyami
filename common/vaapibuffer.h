/*
 *  vaapibuffer.h - Basic object used for decode/encode
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li<xiaowei.li@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef VAAPIBUFFER_H
#define VAAPIBUFFER_H

#include "vaapitypes.h"
#include "vaapiptrs.h"
#include <stdint.h>
#include <va/va.h>

class VaapiBufObject {
  private:
    DISALLOW_COPY_AND_ASSIGN(VaapiBufObject);
  public:
    VaapiBufObject(VADisplay display,
                   VAContextID context,
                   uint32_t bufType, const void *param, uint32_t size);
    ~VaapiBufObject();
    VABufferID getID() const;
    uint32_t getSize();
    void *map();
    void unmap();
    bool isMapped() const;
    static BufObjectPtr create(VADisplay display,
                               VAContextID context,
                               VABufferType bufType,
                               uint32_t size,
                               const void *data = 0,
                               void **mapped_data = 0);

  private:
    VaapiBufObject(VADisplay, VABufferID, void *buf, uint32_t size);
    VADisplay m_display;
    VABufferID m_bufID;
    void *m_buf;
    uint32_t m_size;
};

#endif                          /* VAAPIBUFFER_H */
