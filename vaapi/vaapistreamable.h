/*
 * Copyright (C) 2017 Intel Corporation. All rights reserved.
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

#ifndef vaapistreamable_h
#define vaapistreamable_h
#include <va/va.h>
#include <sstream>

template <typename T>
const std::string toString(const T& t)
{
    std::ostringstream os;
    os << t;
    return os.str();
}


inline std::ostream&
operator<<(std::ostream& os, const VAProfile& profile)
{
    switch(profile) {
    case VAProfileNone:
        return os << "VAProfileNone";
    case VAProfileMPEG2Simple:
        return os << "VAProfileMPEG2Simple";
    case VAProfileMPEG2Main:
        return os << "VAProfileMPEG2Main";
    case VAProfileMPEG4Simple:
        return os << "VAProfileMPEG4Simple";
    case VAProfileMPEG4AdvancedSimple:
        return os << "VAProfileMPEG4AdvancedSimple";
    case VAProfileMPEG4Main:
        return os << "VAProfileMPEG4Main";
    case VAProfileVC1Simple:
        return os << "VAProfileVC1Simple";
    case VAProfileVC1Main:
        return os << "VAProfileVC1Main";
    case VAProfileVC1Advanced:
        return os << "VAProfileVC1Advanced";
    case VAProfileH263Baseline:
        return os << "VAProfileH263Baseline";
    case VAProfileJPEGBaseline:
        return os << "VAProfileJPEGBaseline";
    case VAProfileVP8Version0_3:
        return os << "VAProfileVP8Version0_3";
    case VAProfileHEVCMain:
        return os << "VAProfileHEVCMain";
    case VAProfileHEVCMain10:
        return os << "VAProfileHEVCMain10";
    case VAProfileVP9Profile0:
        return os << "VAProfileVP9Profile0";
    case VAProfileVP9Profile1:
        return os << "VAProfileVP9Profile1";
    case VAProfileVP9Profile2:
        return os << "VAProfileVP9Profile2";
    case VAProfileVP9Profile3:
        return os << "VAProfileVP9Profile3";
    case VAProfileH264ConstrainedBaseline:
        return os << "VAProfileH264ConstrainedBaseline";
    case VAProfileH264High:
        return os << "VAProfileH264High";
    case VAProfileH264Main:
        return os << "VAProfileH264Main";
    case VAProfileH264MultiviewHigh:
        return os << "VAProfileH264MultiviewHigh";
    case VAProfileH264StereoHigh:
        return os << "VAProfileH264StereoHigh";
    default:
        return os << "Unknown VAProfile: " << static_cast<int>(profile);
    }
}

inline std::ostream&
operator<<(std::ostream& os, const VAEntrypoint& entrypoint)
{
    switch(entrypoint) {
    case VAEntrypointVLD:
        return os << "VAEntrypointVLD";
    case VAEntrypointIZZ:
        return os << "VAEntrypointIZZ";
    case VAEntrypointIDCT:
        return os << "VAEntrypointIDCT";
    case VAEntrypointMoComp:
        return os << "VAEntrypointMoComp";
    case VAEntrypointDeblocking:
        return os << "VAEntrypointDeblocking";
    case VAEntrypointVideoProc:
        return os << "VAEntrypointVideoProc";
    case VAEntrypointEncSlice:
        return os << "VAEntrypointEncSlice";
    case VAEntrypointEncSliceLP:
        return os << "VAEntrypointEncSliceLP";
    case VAEntrypointEncPicture:
        return os << "VAEntrypointEncPicture";
#ifndef __ENABLE_STUDIO_VA__
    case VAEntrypointFEI:
        return os << "VAEntrypointFEI";
#endif
    default:
        return os << "Unknown VAEntrypoint: " << static_cast<int>(entrypoint);
    }
}

#endif // vaapistreamable_h
