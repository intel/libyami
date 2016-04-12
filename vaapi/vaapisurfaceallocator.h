/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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

#ifndef vaapisurfaceallocator_h
#define vaapisurfaceallocator_h
#include "common/basesurfaceallocator.h"
#include "common/NonCopyable.h"
#include <va/va.h>

namespace YamiMediaCodec{

class VaapiSurfaceAllocator : public BaseSurfaceAllocator
{
    //extra buffer size for performance
    static const uint32_t EXTRA_BUFFER_SIZE = 5;
public:
    //we do not use DisplayPtr here since we may give this to user someday.
    //@extraSize[in] extra size add to SurfaceAllocParams.size
    VaapiSurfaceAllocator(VADisplay display, uint32_t extraSize = EXTRA_BUFFER_SIZE);
protected:
    virtual YamiStatus doAlloc(SurfaceAllocParams* params);
    virtual YamiStatus doFree(SurfaceAllocParams* params);
    virtual void doUnref();
private:
    VADisplay m_display;
    uint32_t  m_extraSize;
    DISALLOW_COPY_AND_ASSIGN(VaapiSurfaceAllocator);
};

}
#endif //vaapisurfaceallocator_h
