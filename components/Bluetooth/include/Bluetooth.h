#pragma once

#include <cstdint>
#include <string>

class BLE
{
  public:
    BLE();

    void begin();

    void setDeviceName(const std::string deviceName);


    ////////////////////////////////////////
    /// application service interface

    void sendData(const uint8_t* data, const size_t length);

    ////////////////////////////////////////
    /// device information service interface

    ////////////////////////////////////////
    /// battery service interface

    // battery level
    void setBatteryLevel(const uint8_t percentage);  // range from 0..100


};

extern BLE ble;
