#ifndef EEPROM_H
#define EEPROM_H

#include <Wire.h>
#include <Arduino.h>

namespace Murli
{
    const unsigned short ModMemorySize = 0xFA0; // 4000 Byte

    class Rom24LC32A
    {
        public:
            Rom24LC32A(const uint8_t deviceAddress);
            int clear();
            int read(uint8_t* buffer, unsigned short length);
            int write(const uint8_t* buffer, unsigned short length);

        private:
            void waitReady() const;

        private:
            uint8_t _pageSize = 32;
            uint8_t _deviceAddress;
    };
}

#endif // EEPROM_H