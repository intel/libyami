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

#ifndef vaapivpppicture_h
#define vaapivpppicture_h

#include "vaapi/vaapipicture.h"
#include <va/va_vpp.h>

namespace YamiMediaCodec{

class VaapiVppPicture:public VaapiPicture {
public:
    VaapiVppPicture(const ContextPtr& context,
                    const SurfacePtr & surface);
    virtual ~VaapiVppPicture() { }

    bool editVppParam(VAProcPipelineParameterBuffer*&);

    bool process();

protected:
    bool doRender();
private:
    BufObjectPtr m_vppParam;
    DISALLOW_COPY_AND_ASSIGN(VaapiVppPicture);
};

}

#endif                          /* vaapivpppicture_h */
