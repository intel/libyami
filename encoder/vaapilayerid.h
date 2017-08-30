/*
 * Copyright (C) 2014-2016 Intel Corporation. All rights reserved.
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

#ifndef vaapilayerid_h
#define vaapilayerid_h

#include <vector>
#include "VideoEncoderDefs.h"

namespace YamiMediaCodec {

#define VP8_MAX_TEMPORAL_LAYER_NUM 3
#define VP8_MIN_TEMPORAL_GOP 4
#define H264_MIN_TEMPORAL_GOP 8
#define H264_MAX_TEMPORAL_LAYER_NUM 4

class TemporalLayerID;

typedef std::vector<uint8_t> LayerIDs;
typedef std::vector<VideoFrameRate> LayerFrameRates;
typedef SharedPtr<TemporalLayerID> TemporalLayerIDPtr;

class TemporalLayerID {
public:
    TemporalLayerID(const VideoFrameRate& frameRate, const VideoTemporalLayerIDs& layerIDs, const uint8_t* defaultIDs, uint8_t defaultIDsLen);
    virtual void getLayerIds(LayerIDs& ids) const;
    virtual uint8_t getTemporalLayer(uint32_t frameNum) const;
    virtual void getLayerFrameRates(LayerFrameRates& frameRates) const;
    virtual uint8_t getLayerNum() const { return m_layerLen; }
    virtual uint8_t getMiniRefFrameNum() const;

    void checkLayerIDs(uint8_t maxLayerLength = 0) const;

private:
    void calculateFramerate(const VideoFrameRate& frameRate);

protected:
    uint8_t m_layerLen;
    LayerIDs m_ids;
    LayerFrameRates m_frameRates;
    uint8_t m_idPeriod;
    uint8_t m_miniRefFrameNum;
};

class Vp8LayerID : public TemporalLayerID {
public:
    Vp8LayerID(const VideoFrameRate& frameRate, const VideoTemporalLayerIDs& layerIDs, uint8_t layerIndex);

public:
    static const uint8_t m_vp8TempIds[VP8_MAX_TEMPORAL_LAYER_NUM][VP8_MIN_TEMPORAL_GOP];
};

class AvcLayerID : public TemporalLayerID {
public:
    AvcLayerID(const VideoFrameRate& frameRate, const VideoTemporalLayerIDs& layerIDs, uint8_t layerIndex);

private:
    void calculateMiniRefNum();

public:
    static const uint8_t m_avcTempIds[H264_MAX_TEMPORAL_LAYER_NUM][H264_MIN_TEMPORAL_GOP];
};
}

#endif
