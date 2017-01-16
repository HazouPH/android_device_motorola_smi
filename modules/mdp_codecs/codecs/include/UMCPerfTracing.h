/*
Portions Copyright (c) 2011 Intel Corporation.
*/

/*
* Copyright (C) 2009 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef PERF_GATHER_H_

#define PERF_GATHER_H_

#include <sys/time.h>
//#include <time.h>
#include <string.h>
#include <stdio.h>

#define USE_RDTSC // enable RDTSC counter
#define COLLECT_PERFORMANCE

static long long getNowUs1()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_usec + tv.tv_sec * 1000000ll;
}

static unsigned long long int rdtsc1(void)
{
   unsigned long long int x;
   unsigned a, d;

   __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

   return ((unsigned long long)a) | (((unsigned long long)d) << 32);;
}

#ifdef USE_RDTSC
    #define GET_TICKS()  rdtsc1()
#else
    #define GET_TICKS()  getNowUs1()
#endif

class GlobalTimer
{
public:
    GlobalTimer(const char *name)
    {
        number_calls = 0;
        ticks = 0;
        strncpy(m_name, name, sizeof(m_name)/sizeof(m_name[0])-1);
    }
    void Add(long long val)
    {
        ticks += val;
        number_calls++;
    }
    void Close()
    {
    float averageTicks = float(ticks) / number_calls;

    printf("%s; %d; %lld; %.6f;", m_name, number_calls, ticks, averageTicks);

    }
    ~GlobalTimer()
    {
        Close();
    }
protected:
    int number_calls;
    long long ticks;
    char m_name[1024];
};

class LocalTimer
{
public:
    LocalTimer(GlobalTimer &timer)
    :m_timer(timer)
    {
        start = GET_TICKS();
    }
    ~LocalTimer()
    {
        end = GET_TICKS();
        m_timer.Add(end - start);
    }
protected:
    GlobalTimer &m_timer;
    long long start;
    long long end;
};

#ifdef COLLECT_PERFORMANCE
#define AUTO_TIMER(NAME)                            \
    static GlobalTimer global_timer(NAME);          \
    LocalTimer local_timer(global_timer);
#else
#define AUTO_TIMER(NAME)
#endif

#endif  //PERF_GATHER_H_
