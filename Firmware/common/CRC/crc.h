/* Derived from:
 * SD/MMC File System Library
 * Copyright (c) 2016 Neil Thiessen
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

#ifndef SD_CRC_H
#define SD_CRC_H

#include <stddef.h>
#include <stdint.h>
    
uint8_t crc7(const uint8_t* data, int length);
uint16_t crc16(const uint8_t* data, int length);
void update_crc16(uint16_t *pCrc16, const uint8_t data[], size_t length);

// define CRC_NOTABLE to 1 to use table-less implementation of crc16
#if !defined CRC_NOTABLE
    #define CRC_NOTABLE 0
#endif

#endif

/* [] END OF FILE */
