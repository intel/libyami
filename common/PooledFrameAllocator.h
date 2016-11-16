/*
 * Copyright (C) 2016 Intel Corporation. All rights reserved.
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

#ifndef PooledFrameAllocator_h 
#define PooledFrameAllocator_h

#include <vector>
#include <va/va.h>
#include <VideoCommonDefs.h>
#include "common/videopool.h"


namespace YamiMediaCodec {

class FrameAllocator
{
public:
    virtual bool setFormat(uint32_t fourcc, int width, int height) = 0;
    virtual SharedPtr<VideoFrame> alloc() = 0;
    virtual ~FrameAllocator() {}
};

class PooledFrameAllocator : public FrameAllocator
{
public:
    PooledFrameAllocator(const SharedPtr<VADisplay>& display, int poolsize);
    bool setFormat(uint32_t fourcc, int width, int height);
    SharedPtr<VideoFrame> alloc();

private:
    SharedPtr<VADisplay> m_display;
    SharedPtr<VideoPool<VideoFrame> > m_pool;
    int m_poolsize;
};
};

#endif
