/*
 * Copyright (C) 2011-2014 Intel Corporation. All rights reserved.
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
 
/*  log.cpp is to enable debug log with LIBYAMI_LOG_LEVEL and LIBYAMI_TRACE:
 *  LIBYAMI_LOG_LEVEL=log_level: the record will be printed if the log_level larger than (Error, 0), (Warning, 1), (Info, 2)(Debug & Debug_, 3).
 *  LIBYAMI_LOG=log_file: the libyami log saved into log_file;
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
  
#include "log.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include "common/lock.h"

int yamiLogFlag;
FILE* yamiLogFn;
int isInit = 0;
static YamiMediaCodec::Lock g_traceLock;

void yamiTraceInit()
{
    YamiMediaCodec::AutoLock locker(g_traceLock);
    if(!isInit){
        char* libyamiLogLevel = getenv("LIBYAMI_LOG_LEVEL");
        char* libyamLog = getenv("LIBYAMI_LOG");
        FILE* tmp;

        yamiLogFn = stderr;
        if (libyamiLogLevel){
            yamiLogFlag = atoi(libyamiLogLevel);
            if (libyamLog){
                time_t now;
                struct tm* curtime;
                char filename[80];
                
                time(&now);

                if ((curtime = localtime(&now))) {
                    snprintf(filename, sizeof(filename), "%s_%2d_%02d_%02d_%02d_%02d", libyamLog,
                    	curtime->tm_year + 1900, curtime->tm_mon + 1, curtime->tm_mday,
                    	curtime->tm_hour, curtime->tm_sec);
                } else {
                    snprintf(filename, sizeof(filename), "%s", libyamLog);
                }

            	if ((tmp = fopen(filename, "w"))){
                	yamiLogFn = tmp;
                        ERROR("Libyami_Trace is on, save log into %s.\n", filename);
            	} else {
                        ERROR("Open file %s failed.\n", filename);
            	}
            }
        }
        else {
            yamiLogFlag = YAMI_LOG_ERROR;
        }
        isInit = 1;
    }
#ifndef __ENABLE_DEBUG__
     if (yamiLogFlag > YAMI_LOG_ERROR)
         fprintf(stderr, "yami log isn't enabled (--enable-debug)\n");
 #endif
}
