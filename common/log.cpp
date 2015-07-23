/*
 *  log.cpp - enable the log system
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Xin Tang <xin.t.tang@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */
 
/*  log.cpp is to enable debug log with LIBYAMI_LOG_LEVEL and LIBYAMI_TRACE:
 *  LIBYAMI_LOG_LEVEL=log_level: the record will be printed if the log_level larger than (Error, 0), (Warning, 1), (Info, 2)(Debug & Debug_, 3).
 *  LIBYAMI_LOG=log_file: the libyami log saved into log_file;
 */
  
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
                	yamiMessage(stdout, "Libyami_Trace is on, save log into %s.\n", filename);
            	} else {
                	yamiMessage(stderr, "Open file %s failed.\n", filename);
            	}
            }
        }
        else {
            yamiLogFlag = YAMI_LOG_ERROR;
        }
        isInit = 1;
    }
}
