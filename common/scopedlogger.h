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

#ifndef scopedlogger_h
#define scopedlogger_h

/*it's expensive, disable it by default*/
#define DISABLE_SCOPED_LOGGER
#ifndef DISABLE_SCOPED_LOGGER
#include "log.h"

class ScopedLogger {
  public:
    ScopedLogger(const char *str)
    {
        m_str = str;
        INFO("+%s", m_str);
    }
    ~ScopedLogger()
    {
        INFO("-%s", m_str);
    }

  private:
    const char *m_str;
};

#define FUNC_ENTER() ScopedLogger __func_loggger__(__func__)
#else
#define FUNC_ENTER()
#endif

#endif  //scopedlogger_h
