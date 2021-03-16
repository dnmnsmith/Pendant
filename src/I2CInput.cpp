#include <Arduino.h>

#include "I2CInput.h"


I2CInput::I2CInput( TwoWire *pTwoWire, SemaphoreHandle_t& mutex, int addr ) :
    Lockable(mutex),
    m_pTwoWire(pTwoWire),
    m_addr( addr )
{
}

void I2CInput::init()
{
    MutexHolder mutexHolder(this);

    m_pTwoWire->beginTransmission(m_addr);
    m_pTwoWire->write(0xFF);                   // Configure all pins inputs
    m_pTwoWire->endTransmission();  
}

int  I2CInput::read()
{
    int result = 0;
    MutexHolder mutexHolder(this);

    if (m_pTwoWire->requestFrom(m_addr, 1))
    {
        result = (~m_pTwoWire->read()) & 0xFF;
        m_pTwoWire->endTransmission();  
    }

    return result;
}

bool I2CInput::getTopSwitchUp()
{
    return (read() & 0x20) != 0;
}

bool I2CInput::getToggleUp()
{
    return (read() & 0x80) != 0;
}

bool I2CInput::getToggleDown()
{
    return (read() & 0x40) != 0;
}
