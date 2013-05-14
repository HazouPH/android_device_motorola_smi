/* ATParser.cpp
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
#include "ATParser.h"
#include <ctype.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

CATParser::CATParser() : _uiCurrentBufferIndex(0), _uiStartSentenceIndex(0), _bWaitForSentenceBegin(true)
{
}

// Clear
void CATParser::clear()
{
    _uiCurrentBufferIndex = 0;
    _uiStartSentenceIndex = 0;
    _bWaitForSentenceBegin = true;
}

// Sentence
bool CATParser::receive(int iFd)
{
    // Make sure there's room for reading
    if (_uiCurrentBufferIndex >= sizeof(_acReceptionBuffer)) {

        // Ignore whatever we received already
        clear();
    }

    // Read all available bytes
    uint32_t uiPreviousBufferIndex = _uiCurrentBufferIndex;

    while (_uiCurrentBufferIndex < sizeof(_acReceptionBuffer)) {

        int iNbReadChars = read(iFd, &_acReceptionBuffer[_uiCurrentBufferIndex], sizeof(_acReceptionBuffer) - _uiCurrentBufferIndex);

        if (iNbReadChars < 0) {

            break;
        }

        _uiCurrentBufferIndex += iNbReadChars;
    }
    // We should have read something
    if (uiPreviousBufferIndex == _uiCurrentBufferIndex) {

        // Caught a signal or io error?
        return false;
    }

    // Process data
    return processReceivedData(uiPreviousBufferIndex);
}

// Resulting transaction
bool CATParser::extractReceivedSentence(string& strSentence)
{
    // AT Sentence
    if (_sentenceList.empty()) {

        return false;
    }

    // Extract
    strSentence = _sentenceList.front();

    // Pop
    _sentenceList.pop_front();

    return true;
}

// Received data processing
bool CATParser::processReceivedData(uint32_t uiStartIndex)
{
    bool bSentencesAdded = false;

    uint32_t uiIndex = uiStartIndex;

    while (true) {
        // Find all present sentences in buffer
        if (_bWaitForSentenceBegin) {

            // Skip ctrl chars
            for (; uiIndex < _uiCurrentBufferIndex; uiIndex++) {

                if (!iscntrl(_acReceptionBuffer[uiIndex])) {

                    break;
                }
            }
            // Found anything else than ctrl chars?
            if (uiIndex == _uiCurrentBufferIndex) {

                // Only ctrl chars

                // Skip them
                _uiCurrentBufferIndex = 0;

                assert(!_uiStartSentenceIndex);

                // Done
                break;
            }
            // Sentence start
            _uiStartSentenceIndex = uiIndex;

            // Found chars other than ctrls
            _bWaitForSentenceBegin = false;
        }

        // Skip displayable ctrl chars
        for (; uiIndex < _uiCurrentBufferIndex; uiIndex++) {

            if (iscntrl(_acReceptionBuffer[uiIndex])) {

                break;
            }
        }
        // Found?
        if (uiIndex == _uiCurrentBufferIndex) {

            // Unfinished receiving answer

            // Bring available data back to the beginning of the buffer
            memmove(_acReceptionBuffer, &_acReceptionBuffer[_uiStartSentenceIndex], _uiCurrentBufferIndex - _uiStartSentenceIndex);

            // Set current index accordingly
            _uiCurrentBufferIndex -= _uiStartSentenceIndex;

            // Reset start index
            _uiStartSentenceIndex = 0;

            // Done
            break;
        }
        // Complete answer (ignoring trailing ctrl chars)
        _acReceptionBuffer[uiIndex] = '\0';

        // Record new sentence
        _sentenceList.push_back(&_acReceptionBuffer[_uiStartSentenceIndex]);

        // Reset start sentence index
        _uiStartSentenceIndex = 0;

        // Remember some sentences added
        bSentencesAdded = true;

        // Sentence complete
        _bWaitForSentenceBegin = true;
    }

    return bSentencesAdded;
}
