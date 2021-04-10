#pragma once
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

#include <Wire.h>
#include <map>

#include "Lockable.h"


class LEDs : public Lockable
{
    protected:
        struct PwmLedState
            {
                int r;
                int g;
                int b;
            };

        struct LedNotify_t;
        struct LedUpdate_t;

    public:
        static const int MAX=4096;
        static const int NORMAL=1024;

        LEDs( TwoWire *pTwoWire, SemaphoreHandle_t& mutex );
        ~LEDs() = default;

        void init();
        bool poll();

        void ledsOff();
        
        void ledSet( uint8_t addr, uint8_t data);

        void setWsLed( int idx );
        void setSpindleMode( int idx );
        void setResetLed( bool value );
        void setUnlockLed( bool value );
        void setHomeLed( bool value );
        
        void setHoldLed( bool value );
        void setResumeLed( bool value );
        void setFeed100Led( bool value );

        void setRapidLed( int idx); // 0 = 25% 1 = 50% 2 = 100%
        void setXlimit( bool value);
        void setYlimit( bool value);
        void setZlimit( bool value);
        void setProbe( bool value);

        void setJogLed(bool value);

        void setStatusLed( int r, int g, int b);

    protected:
        // Primary mutex we're initialised with handles I2C.
        SemaphoreHandle_t m_stateMutex = nullptr;

        void setStatusLed( const PwmLedState &state);

        void applyLedStruct(const LedNotify_t &ledNotifyStruct);


        void applySpindle();
        void applyWs();
        void applyLimts();
        void applyRGB( const PwmLedState &state );

        void applyAll();

    private:
        TwoWire *m_pTwoWire;

        Adafruit_PWMServoDriver m_pwm;

        bool resetBitSet = false;
        int spindleIdx = 0;

        bool unlockBitSet = false;
        bool homeBitSet = false;
        int wsIdx = 0;

        bool resetLedSet = false;
        bool holdLedSet = false;
        bool resumeLedSet = false;
        bool feed100LedSet = false;

        bool xlimit=false;
        bool ylimit=false;
        bool zlimit=false;
        bool probe=false;

        int rapidIdx=2;
        bool jogCancel =false;

        int statusR = 0;
        int statusG = 0;
        int statusB = 0;

        // Map of I2C addr to preserved state.
        //
        // 0x20 = workspace LEDs
        // 0x21 = spindle LEDs
        // 0x22 = limits LEDs
        std::map< uint8_t, uint8_t > m_i2cStateMap;

        friend class MutexHolder;
        QueueHandle_t m_LedQueue = nullptr;

        friend struct LedNotify_t;
};


