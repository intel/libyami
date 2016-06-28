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

#ifndef vaapipostprocess_scaler_h
#define vaapipostprocess_scaler_h

#include "vaapipostprocess_base.h"
#include <vector>
#include <set>

namespace YamiMediaCodec{

/* class for video scale and color space conversion */
class VaapiPostProcessScaler : public VaapiPostProcessBase {
public:
    VaapiPostProcessScaler();
    virtual YamiStatus process(const SharedPtr<VideoFrame>& src,
                               const SharedPtr<VideoFrame>& dest);

    virtual YamiStatus setParameters(VppParamType type, void* vppParam);

private:
    friend class FactoryTest<IVideoPostProcess, VaapiPostProcessScaler>;
    friend class VaapiPostProcessScalerTest;

    struct ProcParams {
        int32_t level; //set by user
        SharedPtr<VAProcFilterCap> caps; //query from va;
        BufObjectPtr filter; //send to va;
    };

    struct DeinterlaceParams {
        VppDeinterlaceMode mode; //set by user
        std::set<VppDeinterlaceMode> supportedModes; //query from va
        BufObjectPtr filter; //send to va;
    };

    bool mapToRange(float& value, float min, float max,
        int32_t level, int32_t minLevel, int32_t maxLevel);

    //map to driver's range
    bool mapToRange(float& value, int32_t level, int32_t minLevel, int32_t maxLevel,
        VAProcFilterType filterType, SharedPtr<VAProcFilterCap>& caps);

    YamiStatus setParamToNone(ProcParams& params, int32_t none);

    bool getFilters(std::vector<VABufferID>& filters);
    YamiStatus createFilter(BufObjectPtr& filter, VAProcFilterType, float value);

    YamiStatus setProcParams(ProcParams& params, int32_t level,
        int32_t min, int32_t max, int32_t none, VAProcFilterType type);

    YamiStatus setDeinterlaceParam(const VPPDeinterlaceParameters&);
    YamiStatus createDeinterlaceFilter(const VPPDeinterlaceParameters&);

    ProcParams m_denoise;
    ProcParams m_sharpening;
    DeinterlaceParams m_deinterlace;

    static const bool s_registered; // VaapiPostProcessFactory registration result
};
}
#endif                          /* vaapipostprocess_scaler_h */
