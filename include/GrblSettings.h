#pragma once

#include <Arduino.h>
#include <map>

#include <Util.h>


class GrblSettings
{

    public:

        GrblSettings();
        ~GrblSettings();
        GrblSettings(const GrblSettings & obj) = delete;
        GrblSettings(const GrblSettings && obj) = delete;
        GrblSettings & operator = (const GrblSettings & obj) = delete;

        bool getSetting( int field, float &value ) const;
        void setSetting( int field, float value );
        bool parse( std::string s );

        float getRate_mmsec(char axis) const;
        float getAccel_mmsec2(char axis) const;
        float getTravel(char axis) const;

        typedef void (*NotifyFn)();

        void setOnInitialised( NotifyFn notifyFn ) { m_onInitialised = notifyFn; }

        bool initialised() { return m_bInitialised; }
    private:
        bool m_bInitialised = false;
        
        std::map<int,float> m_settings;
        SemaphoreHandle_t m_Mutex = NULL;

        NotifyFn m_onInitialised = nullptr;

};

