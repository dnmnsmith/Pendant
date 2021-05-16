#include <Arduino.h>
#include <EEPROM.h>

#include "LcdDisplay.h"
#include "EepromValues.h"

#define EEPROM_SIZE sizeof(EepromValueStruct)


EepromValues::EepromValues(SemaphoreHandle_t& mutex) : Lockable( mutex )
{
}

EepromValues::~EepromValues()
{

}

void EepromValues::init()
{
    MutexHolder holder( this );
    
    EEPROM.begin(EEPROM_SIZE);
    load();
}

void EepromValues::load()
{
    MutexHolder holder( this );
    EEPROM.get(0, m_values );
}

void EepromValues::save()
{
    {
        MutexHolder holder( this );
        EEPROM.put(0, m_values );
        EEPROM.commit();
    }
}

void EepromValues::set( int index, float x, float y, float z)
{
    setPos( m_values.pos[index], x, y, z );
}
void EepromValues::get( int index, float &x, float &y, float &z)
{
    getPos( m_values.pos[index], x, y, z );
}

void EepromValues::setPos( EepromPos &pos, float x, float y, float z)
{
    MutexHolder holder( this );
    pos = { x, y, z };
    save();
}

void EepromValues::getPos( const EepromPos &pos, float &x, float &y, float &z)
{
    MutexHolder holder( this );

    x = pos.x;
    y = pos.y;
    z = pos.z; 
}


void EepromValues::setToolChangePos( float x, float y, float z)
{
    setPos( m_values.toolChangePos, x, y, z );
}
void EepromValues::getToolChangePos( float &x, float &y, float &z)
{
    getPos( m_values.toolChangePos, x, y, z );
}

void EepromValues::setProbePos( float x, float y, float z)
{
     setPos( m_values.probePos, x, y, z );
   
}
void EepromValues::getProbePos( float &x, float &y, float &z)
{
    getPos( m_values.probePos, x, y, z );
}

void EepromValues::setTloRefPos( float x, float y, float z)
{
    setPos( m_values.tloRefPos, x, y, z );
}
void EepromValues::getTloRefPos( float &x, float &y, float &z)
{
     getPos( m_values.tloRefPos, x, y, z );
}

void EepromValues::setProbeHeight( float probeHeight )
{
    if (probeHeight >= 1.0)
    {
        MutexHolder holder( this );
        m_values.probeHeight  = probeHeight;
        save();
        Serial.printf("Probe height set to: %.03f\n", probeHeight);
    }
    else
    {
        lcdDisplay.printf("Min probe height 1mm");
    }
    
}

float EepromValues::getProbeHeight()
{
    MutexHolder holder( this );
    return m_values.probeHeight;
}


void EepromValues::setProbeDistance( float value )
{
    if (value >= 0.1)
    {    
        MutexHolder holder( this );
        m_values.probeDistance  = value;
        save();
        Serial.printf("Probe distance set to: %.03f\n", value);
    }
    else
    {
        lcdDisplay.printf("Min dist height 0.1mm");
    }
}

float EepromValues::getProbeDistance()
{
    MutexHolder holder( this );
    return m_values.probeDistance;

}
