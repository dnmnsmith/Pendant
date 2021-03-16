// Values preserved to eeprom.
#pragma once

#include "Lockable.h"

class EepromValues : public Lockable
{
    static const int slots = 4;

    struct EepromPos
    {
        float x;
        float y;
        float z;
    };

    struct EepromValueStruct
    {
        EepromPos pos[slots];

        EepromPos toolChangePos;
        EepromPos probePos;

        EepromPos tloRefPos;

        float probeHeight;
        float probeDistance;

    };

    public:
        EepromValues(SemaphoreHandle_t& mutex);
        ~EepromValues();

        void init();
        
        int getSlots() { return slots; }
        
        void set( int index, float x, float y, float z);
        void get( int index, float &x, float &y, float &z);

        // Height of probe switch assembly doodat
        void setProbeHeight( float probeHeight );
        float getProbeHeight();

        // Distance to probe.
        void setProbeDistance( float value );
        float getProbeDistance();

        void setToolChangePos( float x, float y, float z);
        void getToolChangePos( float &x, float &y, float &z);

        void setProbePos( float x, float y, float z);
        void getProbePos( float &x, float &y, float &z);

        void setTloRefPos( float x, float y, float z);
        void getTloRefPos( float &x, float &y, float &z);
        
    private:
        void setPos( EepromPos &pos, float x, float y, float z);
        void getPos( const EepromPos &pos, float &x, float &y, float &z);

        void load();
        void save();

        EepromValueStruct m_values;

};