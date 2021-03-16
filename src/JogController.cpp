#include <Arduino.h>

#include <sstream>
#include <iomanip>

#include "Util.h"
#include "JogController.h"



JogController::JogController(GrblSettings &settings, GrblState &state, long dt) :    
    m_settings(settings),
    m_state(state),
    m_jogSentTime(xTaskGetTickCount() ),
    m_jogRequest("JOG"),
    m_dt(dt)
{



}


long JogController::timeSinceJogMs()
{
    return ((xTaskGetTickCount() - m_jogSentTime) * ( TickType_t ) 1000) / configTICK_RATE_HZ;
}

long JogController::getJogSentTime()
{
    return m_jogSentTime;
}


void JogController::setJogSentTime()
{
    m_jogSentTime = xTaskGetTickCount();
}


bool JogController::parse( std::string s )
{
    bool ok = false;

    if (!m_enabled)
        return false;

    try
    {
    
        // Skip trailing WS
        while ( isspace(s.back()) )
            s.pop_back();

        //Serial.printf("Validating %s\n",s.c_str());

        if (
            s.length() > 4 && 
            s[0]=='<' && 
            (s[1] == 'X' || s[1] == 'Y' || s[1] == 'Z' || s[1] == 'A' ) && s[2] == ':' && 
            s.back() == '>' &&
            std::count(s.begin(), s.end(), ':') == 1)
        {
            s.pop_back();
            std::vector<std::string> fields = split(s.substr(1),":");
            char axis = s[1];
            float delta = atof(fields[1].c_str());

            //Serial.printf("Axis %c delta %f\n", axis, delta );

            if (axis == 'X')
            {
                 m_jogRequest.x += delta;                
            }
            else if (axis == 'Y')
            {
                 m_jogRequest.y += delta;                
            }
            else if (axis == 'Z')
            {
                 m_jogRequest.z += delta;                
            }
            else if (axis == 'A' && m_APosUpdateFn != nullptr)
            {
                m_APosUpdateFn( delta );
            }
        }

    }
    catch(const std::exception& e)
    {
        Serial.printf("Parsing : %s failed with %s\n",s.c_str(),e.what());
        ok = false;
    }
    return ok;
}

bool JogController::poll()
{
    if (m_jogRequest.isZero())
        return false;
    
    if (timeSinceJogMs() < m_dt)
        return false;

    float dts = (float)m_dt / 1000.0;

    // At this point, we have non-zero jog awaiting, and at least dt time (ms) since last.
    float feedx = m_state.feedRate.get() / 60;
    float feedy = m_state.feedRate.get() / 60;
    float feedz = m_state.feedRate.get() / 60;

    // TODO - select feed rate.
    float maxFeedx = m_settings.getRate_mmsec('X');
    float maxFeedy = m_settings.getRate_mmsec('Y');
    float maxFeedz = m_settings.getRate_mmsec('Z');

    float accelx = m_settings.getAccel_mmsec2('X');
    float accely = m_settings.getAccel_mmsec2('Y');
    float accelz = m_settings.getAccel_mmsec2('Z');

    feedx += (accelx/2) * dts;
    feedy += (accely/2) * dts;
    feedz += (accelz/2) * dts;
    feedx = std::min( feedx, maxFeedx);
    feedy = std::min( feedy, maxFeedy);
    feedz = std::min( feedz, maxFeedz);

    int xSign = (m_jogRequest.x > 0) - (m_jogRequest.x < 0);
    int ySign = (m_jogRequest.y > 0) - (m_jogRequest.y < 0);
    int zSign = (m_jogRequest.z > 0) - (m_jogRequest.z < 0);

    //Serial.printf("Signs = %d %d %d\n", xSign, ySign, zSign );

    float xDelta = std::min( feedx * dts, abs(m_jogRequest.x) );
    float yDelta = std::min( feedy * dts, abs(m_jogRequest.y) );
    float zDelta = std::min( feedz * dts, abs(m_jogRequest.z) );

    //Serial.printf("Deltas = %f %f %f\n", xDelta, yDelta, zDelta );

    m_jogRequest.x -= (xDelta * xSign);
    m_jogRequest.y -= (yDelta * ySign);
    m_jogRequest.z -= (zDelta * zSign);

    std::ostringstream oss;
    oss << "$J=G21G91" << 
            "F" << std::fixed << std::setprecision( 3 ) <<(maxFeedx * 60) << 
            "X" << std::fixed << std::setprecision( 3 ) <<(xDelta * xSign) << 
            "Y"<< std::fixed << std::setprecision( 3 ) <<(yDelta * ySign) << 
            "Z" << std::fixed << std::setprecision( 3 ) <<(zDelta * zSign);    
    m_command = oss.str();

    setJogSentTime();

    return true;
}

void JogController::clear()
{
    setJogSentTime();
    m_jogRequest.clear();
    m_command.clear();
}

std::string JogController::getCommand()
{
    std::string res = m_command;
    m_command.clear();
    return res;
}
