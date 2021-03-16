#pragma once

#include <Arduino.h>

#include <FreeRTOS.h>
#include <freertos/semphr.h>
#include <algorithm>
#include <stdexcept>
#include <vector>

#include "State.h"
#include "Position.h"

#    include <stdint.h>


enum GrblStateEnum_t
{
    IDLE,
    RUN,
    HOLD,
    JOG,
    ALARM,
    DOOR,
    CHECK,
    HOME,
    SLEEP
};


enum WorkSpace_t
{
    WORKSPACE_G54,
    WORKSPACE_G55,
    WORKSPACE_G56,
    WORKSPACE_G57,
    WORKSPACE_G58,
    WORKSPACE_G59
};

static const int WORKSPACE_COUNT = 6;

enum SpindleState_t
{
    SPINDLE_M3,
    SPINDLE_M4,
    SPINDLE_M5
};

enum class ProbeMode_t
{
    NONE,
    WORKPIECE_PROBE,
    TOOL_REF_PROBE,
    TOOL_LEN_PROBE
};


// 
class GrblState
{
    public:
        GrblState()
        {
            Mutex = xSemaphoreCreateMutex();
        }
        ~GrblState()
        {
            vSemaphoreDelete( Mutex );
            Mutex=NULL;
        }
        GrblState( const GrblState & ) = delete; 	

        // Parse a line from GRBL status indication
        bool parseGrblLine(std::string s);

        void SetWorkspaceNotify( void (*fn)(State*,void*)  )
        {
            m_notifyWorkspaceFn = fn;
        }
        void SetSpindleNotify( void (*fn)(State*,void*)  )
        {
            m_notifySpindleFn = fn;
        }

        void setNotifyProbeComplete( void (*fn)() )
        {
            m_notifyProbeComplete = fn;
        }

        void UpdateA( float delta );

        bool IsIdleState();

        SemaphoreHandle_t Mutex = NULL;

        TState<std::string> parserState = TState<std::string>("GrblState");

        Position workspace[ WORKSPACE_COUNT ] = 
            { Position("G54"), Position("G55"), Position("G56"), Position("G57"), Position("G58"), Position("G59")};

        WorkSpace_t currentWorkspace = WORKSPACE_G54;

        TState<double> TLO = TState<double>("TLO");                     // Tool Length Offset

        Position G28 = Position("G28");     // Reference Position
        Position G30 = Position("G30");     // Second Reference Position
        Position G92 = Position("G31");     // Temporary Work Offset.
        
        TState<std::string> spindleState = TState<std::string>("Spindle");
        TState<int> spindleSpeed = TState<int>("SpindleSpeed");

        Position PRB = Position("PRB"); 
        TState<bool> probeSuccess = TState<bool>("ProbeSuccess", false );

        Position WCO = Position("WCO");
        Position MPos = Position("MPOS");
        Position WPos = Position("WPOS");
        TState<float> A = TState<float>("APOS", 0.0 );

// Work coordinate offset = SUM(workspace[currentWorkspace],G92,TLO)

//      Machine position and work position are related by this simple equation per axis: WPos = MPos - WCO
//        Position *pCurrentDisplay = &WPos;

        TState<int> feedRate = TState<int>("FeedRate");
        int lineNumber = 0;

        TState<int> overrideFeedPercent = TState<int>("OverrideFeedPercent", 100);
        TState<int> overrideRapidPercent = TState<int>("OverrideRapidPercent", 100);
        TState<int> overrideSpindlePercent = TState<int>("OverrideSpindlePercent", 100);

        bool door = false;
        bool hold = false;
        bool reset = false;
        bool start = false;
        TState<bool> limitX = TState<bool>("LimitX", false );
        TState<bool> limitY = TState<bool>("LimitY", false );
        TState<bool> limitZ = TState<bool>("LimitZ", false );
        TState<bool> probe = TState<bool>("Probe", false );

        TState<bool> resetRequired = TState<bool>("ResetRequired", false );
        TState<bool> unlockRequired = TState<bool>("UnlockRequired", false );
        TState<bool> homingRequired = TState<bool>("HomingRequired", false );

        TState<ProbeMode_t> motionMode = TState<ProbeMode_t>("ProbeMode",ProbeMode_t::NONE);

    protected:

        // Parse a field from a ? return
        void parseStateField(std::string s);

        // Parse a GCODE Parser 
        void parseGcodeParserState(std::string s);

        // Update WCO after change to dependent (coordinate system, G92 or TLO)
        void updateWCO();
        
        /// Clear the state of the Pins (limit pins, etc)
        void resetPinState();
        void notifyPinState();

        void (*m_notifyWorkspaceFn)(State*,void*) = nullptr;
        void (*m_notifySpindleFn)(State*,void*) = nullptr;
        void (*m_notifyProbeComplete)() = nullptr;

};

/// Grab mutex for the GRBL state within the lifetime of this object
class GrblStateMutexHelper
{
    public:
        GrblStateMutexHelper(GrblState &state ) : m_state(state)
        {
            // Only try lock if we're not already the holder.
            if (xSemaphoreGetMutexHolder(m_state.Mutex) != xTaskGetCurrentTaskHandle())
            {
                xSemaphoreTake(m_state.Mutex,portMAX_DELAY);
                m_release=true;
            }
        }
        ~GrblStateMutexHelper()
        {
            if (m_release)
                xSemaphoreGive(m_state.Mutex);
        }
        GrblStateMutexHelper( const GrblStateMutexHelper & ) = delete; 	
    private:
        GrblState &m_state;
        bool m_release = false;
};


