/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#define LOG_TAG "MediaBufferPool"
#include <utils/Log.h>

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include "MediaBufferPool.h"

#define DEFAULT_PAGE_SIZE 4096

namespace android {

MediaBufferPool::MediaBufferPool()
    : mMaxBufferSize(0),
      mFirstBuffer(NULL),
      mLastBuffer(NULL) {
}

MediaBufferPool::~MediaBufferPool() {
    MediaBuffer *next;
    for (MediaBuffer *buffer = mFirstBuffer; buffer != NULL;
         buffer = next) {
        next = buffer->nextBuffer();

        CHECK_EQ(buffer->refcount(), 0);

        buffer->setObserver(NULL);
        buffer->release();
    }
}

status_t MediaBufferPool::acquire_buffer(int size, MediaBuffer **out) {
    Mutex::Autolock autoLock(mLock);

    MediaBuffer *next = NULL;
    while (mFirstBuffer) {
        if ((int)mFirstBuffer->size() >= size) {
            next = mFirstBuffer->nextBuffer();

            // pop first buffer out of list
            *out = mFirstBuffer;
            mFirstBuffer->add_ref();
            mFirstBuffer->reset();

            mFirstBuffer = next;
            if (mFirstBuffer == NULL) {
                mLastBuffer = NULL;
            }
            return OK;
        } else {
            // delete the first buffer from the list
            next = mFirstBuffer->nextBuffer();
            mFirstBuffer->setObserver(NULL);
            mFirstBuffer->release();
            mFirstBuffer = next;
        }
    }

    // not a single buffer matches the requirement. Allocating a new buffer.

    mFirstBuffer = NULL;
    mLastBuffer = NULL;

    size = ((size + DEFAULT_PAGE_SIZE - 1)/DEFAULT_PAGE_SIZE) * DEFAULT_PAGE_SIZE;
    if (size < mMaxBufferSize) {
        size = mMaxBufferSize;
    } else {
        mMaxBufferSize = size;
    }
    MediaBuffer *p = new MediaBuffer(size);
    *out = p;
    return (p != NULL) ? OK : NO_MEMORY;
}

void MediaBufferPool::signalBufferReturned(MediaBuffer *buffer) {
    Mutex::Autolock autoLock(mLock);

    buffer->setObserver(this);

    if (mLastBuffer) {
        mLastBuffer->setNextBuffer(buffer);
    } else {
        mFirstBuffer = buffer;
    }

    mLastBuffer = buffer;
}

}  // namespace android
