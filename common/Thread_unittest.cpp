/*
 * Copyright 2016 Intel Corporation
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

// primary header
#include "Thread.h"

#include "common/unittest.h"
#include "common/log.h"

namespace YamiMediaCodec {

using namespace std;

#define THREAD_TEST(name) \
    TEST(ThreadTest, name)

void add(int& v)
{
    v++;
}

void sleepAndAdd(Thread& t, int seconds, int& v)
{
    sleep(seconds);
    EXPECT_FALSE(t.isCurrent());
    t.post(bind(add, ref(v)));
}

void checkSum(int& v, int expected)
{
    EXPECT_EQ(expected, v);
}

void isCurrent(Thread& t)
{
    EXPECT_TRUE(t.isCurrent());
}

void emptyJob()
{
    /* nothing */
}

void sendJob(Thread& t, Job job)
{
    t.send(job);
}

THREAD_TEST(FullTest)
{
    std::cout << "this may take some time" << std::endl;

    Thread t[3];

    //check start
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(t[i].start());
    }

    //post
    int v = 0;
    for (int i = 0; i < 5; i++) {
        t[0].post(bind(sleepAndAdd, ref(t[2]), 1, ref(v)));
    }
    for (int i = 0; i < 5; i++) {
        t[1].post(bind(sleepAndAdd, ref(t[2]), 2, ref(v)));
    }

    //make sure we can send message from diffrent thread
    t[0].send(bind(sendJob, ref(t[2]), emptyJob));
    t[1].send(bind(sendJob, ref(t[2]), emptyJob));

    //is current
    t[2].send(bind(isCurrent, ref(t[2])));

    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(t[i].send(bind(isCurrent, ref(t[i]))));
        t[i].stop();
    }
    EXPECT_EQ(10, v);

    //send after stop
    EXPECT_FALSE(t[2].send(bind(checkSum, ref(v), 11)));
}
}
