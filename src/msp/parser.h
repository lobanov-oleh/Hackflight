/*
   Copyright (c) 2022 Simon D. Levy

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   Hackflight is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along with
   Hackflight. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>

class MspParser {

    private:

        typedef enum {
            IDLE,
            GOT_START,
            GOT_M,
            GOT_ARROW,
            GOT_SIZE,
            IN_PAYLOAD,
            GOT_CRC
        } parserState_t; 

        parserState_t m_parserState;

        uint8_t m_payload[128] = {};

    public:

        parserState_t getParserState(void)
        {
            return m_parserState;
        }

        /**
          * Returns message type or 0 for not  ready
          */
        uint8_t parse(const uint8_t c)
        {
            uint8_t messageType = 0;

            static uint8_t _type;
            static uint8_t _crc;
            static uint8_t _size;
            static uint8_t _index;

            // Payload functions
            _size = m_parserState == GOT_ARROW ? c : _size;
            _index = m_parserState == IN_PAYLOAD ? _index + 1 : 0;
            const bool incoming = _type >= 200;
            const bool in_payload = incoming && m_parserState == IN_PAYLOAD;
            // Command acquisition function
            _type = m_parserState == GOT_SIZE ? c : _type;

            // Parser state transition function (final transition below)
            m_parserState
                = m_parserState == IDLE && c == '$' ? GOT_START
                : m_parserState == GOT_START && c == 'M' ? GOT_M
                : m_parserState == GOT_M && (c == '<' || c == '>') ? GOT_ARROW
                : m_parserState == GOT_ARROW ? GOT_SIZE
                : m_parserState == GOT_SIZE ? IN_PAYLOAD
                : m_parserState == IN_PAYLOAD && _index <= _size ? IN_PAYLOAD
                : m_parserState == IN_PAYLOAD ? GOT_CRC
                : m_parserState;

            // Checksum transition function
            _crc 
                = m_parserState == GOT_SIZE ?  c
                : m_parserState == IN_PAYLOAD ? _crc ^ c
                : m_parserState == GOT_CRC ? _crc 
                : 0;

            // Payload accumulation
            if (in_payload) {
                m_payload[_index-1] = c;
            }

            if (m_parserState == GOT_CRC) {

                // Message dispatch
                if (_crc == c) {
                    messageType = _type;
                }

                m_parserState = IDLE;
            }

            return messageType;

        } // parse

        bool isIdle(void)
        {
            return m_parserState == IDLE;
        }

        int16_t parseShort(uint8_t index)
        {
            int16_t s = 0;
            memcpy(&s,  &m_payload[2*index], sizeof(int16_t));
            return s;

        }

}; // class MspParser