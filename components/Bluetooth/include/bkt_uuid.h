//
// SPDX-License-Identifier: GPL-2.0-only
//


#pragma once

#define APP_UUID(A,B) 0xF2, 0xB4, 0x98, 0xA8, 0xD2, 0x15, 0xE2, 0xAF, 0x03, 0x4B, B, A, 0x75, 0x17, 0xAF, 0x30

//30AF1775-xxxx-4B03-AFE2-15D2A898B4F2

#define APP_COMM_SERVICE_UUID                  APP_UUID(0x01, 0x00)

#define APP_COMM_READ_CHARACTERISTIC_UUID      APP_UUID(0x01, 0x01)
#define APP_COMM_WRITE_CHARACTERISTIC_UUID     APP_UUID(0x01, 0x02)
