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

#ifndef Thread_h
#define Thread_h

#include "condition.h"
#include "lock.h"
#include "Functional.h"

#include <string>
#include <deque>
#include <pthread.h>

namespace YamiMediaCodec {

typedef std::function<void(void)> Job;

class Thread {
public:
    explicit Thread(const char* name = "");
    ~Thread();
    bool start();
    //stop thread, this will waiting all post/sent job done
    void stop();
    //post job to this thread
    void post(const Job&);
    //send job and wait it done
    bool send(const Job&);
    bool isCurrent();

private:
    //thread loop
    static void* init(void*);
    void loop();
    void enqueue(const Job& r);
    void sendJob(const Job& r, bool& flag);

    std::string m_name;
    bool m_started;
    pthread_t m_thread;

    Lock m_lock;
    Condition m_cond;
    Condition m_sent;
    std::deque<Job> m_queue;

    static const pthread_t INVALID_ID = (pthread_t)-1;

    DISALLOW_COPY_AND_ASSIGN(Thread);
};
};

#endif
