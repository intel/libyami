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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <algorithm>
#include "vaapilayerid.h"
#include "common/log.h"

namespace YamiMediaCodec {

const uint8_t Vp8LayerID::m_vp8TempIds[VP8_MAX_TEMPORAL_LAYER_NUM][VP8_MIN_TEMPORAL_GOP]
    = { { 0, 0, 0, 0 },
        { 0, 1, 0, 1 },
        { 0, 2, 1, 2 } };

const uint8_t AvcLayerID::m_avcTempIds[H264_MAX_TEMPORAL_LAYER_NUM][H264_MIN_TEMPORAL_GOP]
    = { { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 0, 1, 0, 1, 0, 1 },
        { 0, 2, 1, 2, 0, 2, 1, 2 },
        { 0, 3, 2, 3, 1, 3, 2, 3 } };

TemporalLayerID::TemporalLayerID(const VideoFrameRate& frameRate, const VideoTemporalLayerIDs& layerIDs, const uint8_t* defaultIDs, uint8_t defaultIDsLen)
{
    if (layerIDs.numIDs) {
        m_idPeriod = layerIDs.numIDs;
        for (uint32_t i = 0; i < layerIDs.numIDs; i++)
            m_ids.push_back(layerIDs.ids[i]);
    }
    else { //use the default temporal IDs
        assert(defaultIDs && defaultIDsLen > 0);
        m_idPeriod = defaultIDsLen;
        for (uint32_t i = 0; i < m_idPeriod; i++)
            m_ids.push_back(defaultIDs[i]);
    }

    calculateFramerate(frameRate);
}

void TemporalLayerID::getLayerIds(LayerIDs& ids) const
{
    ids = m_ids;
}

uint8_t TemporalLayerID::getTemporalLayer(uint32_t frameNumInGOP) const
{
    return m_ids[frameNumInGOP % m_idPeriod];
}

void TemporalLayerID::getLayerFrameRates(LayerFrameRates& frameRates) const
{
    frameRates = m_frameRates;
    return;
}

void TemporalLayerID::calculateFramerate(const VideoFrameRate& frameRate)
{
    uint8_t numberOfLayerIDs[TEMPORAL_LAYERIDS_LENGTH_MAX];
    LayerIDs tempIDs = m_ids;
    uint8_t i;

    std::sort(tempIDs.begin(), tempIDs.end());
    memset(numberOfLayerIDs, 0, sizeof(numberOfLayerIDs));
    for (i = 0; i < tempIDs.size(); i++) {
        numberOfLayerIDs[tempIDs[i]]++;
    }
    m_layerLen = tempIDs[i - 1] + 1;
    assert(m_layerLen < TEMPORAL_LAYERIDS_LENGTH_MAX);

    VideoFrameRate frameRateTemp;
    uint32_t denom = m_ids.size();
    uint32_t num = 0;
    assert(frameRate.frameRateNum && frameRate.frameRateDenom);
    frameRateTemp.frameRateDenom = frameRate.frameRateDenom * denom;
    for (i = 0; i < m_layerLen; i++) {
        num += numberOfLayerIDs[i];
        frameRateTemp.frameRateNum = num * frameRate.frameRateNum;
        m_frameRates.push_back(frameRateTemp);
    }

    return;
}

void TemporalLayerID::checkLayerIDs(uint8_t maxLayerLength) const
{
    LayerIDs tempIDs = m_ids;
    const uint8_t LAYERID0 = 0;
    assert(LAYERID0 == tempIDs[0]);
    if (m_idPeriod > TEMPORAL_LAYERIDS_LENGTH_MAX) {
        ERROR("m_idPeriod(%d) should be in (0, %d]", m_idPeriod, TEMPORAL_LAYERIDS_LENGTH_MAX);
        assert(false);
    }
    //check if the layerID is complete
    std::sort(tempIDs.begin(), tempIDs.end());
    for (uint8_t i = 1; i < m_idPeriod; i++) {
        if (tempIDs[i] - tempIDs[i - 1] > 1) {
            ERROR("layer IDs illegal, no layer: %d.\n", (tempIDs[i - 1] + tempIDs[i]) / 2);
            assert(false);
        }
    }
    if ((m_layerLen > maxLayerLength) || (m_layerLen < 2)) {
        ERROR("m_layerLen(%d) should be in [2, %d]", m_layerLen, maxLayerLength);
        assert(false);
    }
    return;
}

Vp8LayerID::Vp8LayerID(const VideoFrameRate& frameRate, const VideoTemporalLayerIDs& layerIDs, uint8_t layerIndex)
    : TemporalLayerID(frameRate, layerIDs, m_vp8TempIds[layerIndex], VP8_MIN_TEMPORAL_GOP)
{
    checkLayerIDs(VP8_MAX_TEMPORAL_LAYER_NUM);
}

AvcLayerID::AvcLayerID(const VideoFrameRate& frameRate, const VideoTemporalLayerIDs& layerIDs, uint8_t layerIndex)
    : TemporalLayerID(frameRate, layerIDs, m_avcTempIds[layerIndex], H264_MIN_TEMPORAL_GOP)
{
    checkLayerIDs(H264_MAX_TEMPORAL_LAYER_NUM);
}
}
