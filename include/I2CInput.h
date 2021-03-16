#pragma once
#include <Arduino.h>

#include <Wire.h>

#include "Lockable.h"


class I2CInput : public Lockable
{
    public:
        static const int MAX=4096;

        I2CInput( TwoWire *pTwoWire, SemaphoreHandle_t& mutex, int addr = 0x23 );
        ~I2CInput() = default;

        void init();

        int read();

        bool getTopSwitchUp();
        bool getToggleUp();
        bool getToggleDown();

    private:
        TwoWire *m_pTwoWire;
        int m_addr;
};
