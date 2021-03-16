#include <Arduino.h>

#include "GrblSettings.h"


GrblSettings::GrblSettings()
{
    m_Mutex = xSemaphoreCreateMutex();
}


GrblSettings::~GrblSettings()
{
    vSemaphoreDelete( m_Mutex );
    m_Mutex=NULL;
}

bool GrblSettings::getSetting( int field, float &value ) const
{
    bool ok = true;

    // Only try lock if we're not already the holder.
    if (xSemaphoreGetMutexHolder(m_Mutex) != xTaskGetCurrentTaskHandle())
    {
        xSemaphoreTake(m_Mutex,portMAX_DELAY);

        auto it = m_settings.find( field );
        ok = it!=m_settings.end();
        if (ok)
        {
            value = it->second;
        }
        xSemaphoreGive(m_Mutex);
    }
    return ok ;
}


void GrblSettings::setSetting( int field, float value )
{
    if (xSemaphoreGetMutexHolder(m_Mutex) != xTaskGetCurrentTaskHandle())
    {
        xSemaphoreTake(m_Mutex,portMAX_DELAY);

        auto it = m_settings.find( field );
        if (it!=m_settings.end())
        {
            m_settings.erase(it);
        }

        m_settings.insert(std::pair<int,float>(field,value));
        xSemaphoreGive(m_Mutex);

        //Serial.printf("Setting %d is %F\n",field,value);
        // Last setting.
        if (field == 135 && m_onInitialised != nullptr)
        {
            m_onInitialised();
            m_bInitialised = true;
        }
    }
    else
    {
         Serial.printf("Already have sema, alegedly\n");
       /* code */
    }
    

}

// Parse a settings string, for example:
bool GrblSettings::parse( std::string s )
{
    if (s[0] != '$')
        return false;

        // Tail the string.
    while (isspace(s.back()) )
        s.pop_back();

    if (s.find('=') == std::string::npos)
        return false;

    std::vector<std::string> fields = split(s.substr(1),"=");

    if (fields.size() != 2)
        return false;

    int field = atoi(fields[0].c_str());
    float value = atof( fields[1].c_str());

    setSetting(field,value);

    return true;
}

float GrblSettings::getRate_mmsec(char axis) const 
{ 
    float res = 0.0;

    if (axis=='X')
        getSetting( 110, res);
    else if (axis=='Y')
        getSetting( 111, res);
    else if (axis=='Z')
        getSetting( 112, res);

    return res / 60.0;
}

float GrblSettings::getAccel_mmsec2(char axis) const
{
    float res = 0.0;

    if (axis=='X')
        getSetting( 120, res);
    else if (axis=='Y')
        getSetting( 121, res);
    else if (axis=='Z')
        getSetting( 122, res);

    return res;
}
float GrblSettings::getTravel(char axis) const
{
   float res = 0.0;

    if (axis=='X')
        getSetting( 130, res);
    else if (axis=='Y')
        getSetting( 131, res);
    else if (axis=='Z')
        getSetting( 132, res);

    return res;
}
