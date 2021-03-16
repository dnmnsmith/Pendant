#include "LEDs.h"

#include <Arduino.h>

static const uint8_t ws_bit[] = { 0xFD, 0xFE, 0x7F, 0xBF, 0xDF, 0xEF };
static const uint8_t spindle_bit [] = { 0xBF, 0xDF, 0xEF };
static const uint8_t rapid_bit[] = { 0x7F, 0xBF, 0xDF };

enum LedId
{
    LEDS_OFF,
    WS_G54,
    WS_G55,
    WS_G56,
    WS_G57,
    WS_G58,
    WS_G59,

    SPINDLE_M3,
    SPINDLE_M4,
    SPINDLE_M5,

    LED_RESET,
    LED_UNLOCK,
    LED_HOME,
    LED_HOLD,
    LED_RESUME,
    LED_FEED100,
    LED_RAPID25,
    LED_RAPID50,
    LED_RAPID100,
    
    LED_XLIM,
    LED_YLIM,
    LED_ZLIM,
    LED_PROBE,
    LED_JOG,

    LED_STATUS

};

struct LEDs::LedNotify_t 
    {
        LedId ledId;
        union 
        {
            bool        state;
            LEDs::PwmLedState ppmLedState;
        } ledInfo;        
    };

struct LEDs::LedUpdate_t
    {
        int i2cAddress;
        union 
        {
            int        state;
            LEDs::PwmLedState ppmLedState;
        } ledInfo;        
    };


LEDs::LEDs( TwoWire *pTwoWire, SemaphoreHandle_t& mutex ) :
    Lockable(mutex),
    m_pTwoWire(pTwoWire),
    m_pwm((uint8_t)0x40,*pTwoWire)
{
    m_i2cStateMap.insert( std::pair<uint8_t,uint8_t>(0x20, 0xFF));
    m_i2cStateMap.insert( std::pair<uint8_t,uint8_t>(0x21, 0xFF));
    m_i2cStateMap.insert( std::pair<uint8_t,uint8_t>(0x22, 0xFF));

    m_stateMutex = xSemaphoreCreateRecursiveMutex();
    m_LedQueue = xQueueCreate( 10, sizeof(LedUpdate_t) );
}

void LEDs::init()
{
    MutexHolder mutexHolder(this);

    m_pwm.begin();
    m_pwm.setPWMFreq(1600);  // Set to whatever you like, we don't use it in this demo!

    // Drive each pin in a 'wave'
    // for (uint8_t pin=0; pin<3; pin++) {
    //     m_pwm.setPWM(pin, 4096, 0);       // turns pin fully on
    //     delay(100);
    //     m_pwm.setPWM(pin, 0, 4096);       // turns pin fully off
    // }

    applyWs();
    applySpindle();
    applyLimts();
}


void LEDs::applyLedStruct(const LedNotify_t &ledNotifyStruct)
{
    if (!xSemaphoreTakeRecursive( m_stateMutex ,portMAX_DELAY))
    {
        assert(false);
    }

    switch( ledNotifyStruct.ledId )
    {
    case WS_G54: wsIdx = 0; applyWs(); break;
    case WS_G55: wsIdx = 1; applyWs(); break;
    case WS_G56: wsIdx = 2; applyWs(); break;
    case WS_G57: wsIdx = 3; applyWs(); break;
    case WS_G58: wsIdx = 4; applyWs(); break;
    case WS_G59: wsIdx = 5; applyWs(); break;

    case SPINDLE_M3: spindleIdx = 0; applySpindle(); break;
    case SPINDLE_M4: spindleIdx = 1; applySpindle(); break;
    case SPINDLE_M5: spindleIdx = 2; applySpindle(); break;

    case LED_RESET: resetLedSet = ledNotifyStruct.ledInfo.state;  applySpindle(); break;
    case LED_UNLOCK: unlockBitSet = ledNotifyStruct.ledInfo.state;  applyWs(); break;
    case LED_HOME: homeBitSet = ledNotifyStruct.ledInfo.state;  applyWs(); break;
    case LED_HOLD: holdLedSet = ledNotifyStruct.ledInfo.state;  applySpindle(); break;
    case LED_RESUME: resumeLedSet = ledNotifyStruct.ledInfo.state;  applySpindle(); break;
    case LED_FEED100: feed100LedSet = ledNotifyStruct.ledInfo.state;  applySpindle(); break;
    
    case LED_RAPID25: rapidIdx = 0; applyLimts(); break;
    case LED_RAPID50: rapidIdx = 1; applyLimts(); break;
    case LED_RAPID100: rapidIdx = 2; applyLimts(); break;
    
    case LED_XLIM: xlimit = ledNotifyStruct.ledInfo.state; applyLimts(); break;
    case LED_YLIM: ylimit = ledNotifyStruct.ledInfo.state; applyLimts(); break;
    case LED_ZLIM: zlimit = ledNotifyStruct.ledInfo.state; applyLimts(); break;
    case LED_PROBE: probe = ledNotifyStruct.ledInfo.state; applyLimts(); break;
    case LED_JOG: jogCancel = ledNotifyStruct.ledInfo.state; applyLimts(); break;

    case LED_STATUS: setStatusLed( ledNotifyStruct.ledInfo.ppmLedState ); break;

    case LEDS_OFF: 
        rapidIdx = -1; 
        wsIdx =-1; 
        spindleIdx =-1; 
        resetLedSet = false;
        unlockBitSet = false;
        homeBitSet = false;
        holdLedSet = false;
        resumeLedSet = false;
        feed100LedSet = false;
        xlimit = false;
        ylimit = false;
        zlimit = false;
        probe = false;
        jogCancel = false;
        applyAll(); 
        break;
    }

    xSemaphoreGiveRecursive(m_stateMutex);
}

void LEDs::applyAll()
{
    applySpindle();
    applyWs(); 
    applyLimts();
}


bool LEDs::poll()
{
    LedUpdate_t update;

    if (xQueueReceive(m_LedQueue, (void *const)&update, 0))
    {
        MutexHolder mutexHolder(this);

        if ( update.i2cAddress == 0x20 || update.i2cAddress == 0x21 || update.i2cAddress == 0x22)
        {
            Wire.beginTransmission(update.i2cAddress);
            Wire.write(update.ledInfo.state);
            Wire.endTransmission();  
        }
        else if ( update.i2cAddress == 0x40 )
        {
            applyRGB( update.ledInfo.ppmLedState );
        }
        else
        {
            assert(false);
        }
        
        return true;
    }
    return false;
}

void LEDs::ledsOff()
{
    LedNotify_t notify = { LEDS_OFF, .ledInfo = { .state = 0 } };
    applyLedStruct( notify );
}

void LEDs::ledSet( uint8_t addr, uint8_t data)
{
    LedUpdate_t update = { .i2cAddress = addr, .ledInfo = { .state = data } };
    xQueueSendToBack( m_LedQueue, &update, 0 );
}


static const LedId wsLedMap[] = { WS_G54, WS_G55, WS_G56, WS_G57, WS_G58, WS_G59 };

void LEDs::setWsLed( int idx )
{
    LedNotify_t notify = { wsLedMap[ idx ], .ledInfo = { .state = true } };
    applyLedStruct( notify );
}

void LEDs::applyWs()
{
    uint8_t state = 0xFF;

    if (wsIdx != -1)
        state &=  ws_bit[wsIdx];


    if (homeBitSet)
        state &= 0xF7;

    if (unlockBitSet)
        state &= 0xFB;

    if (state != m_i2cStateMap[ 0x20 ])
    {
        m_i2cStateMap[ 0x20 ] = state;
        ledSet( 0x20, state );
    }
}

static const LedId spindleModeMap[] = { SPINDLE_M3, SPINDLE_M4, SPINDLE_M5 };

// 0 for M3, 1 for M4, 2 for M5
void LEDs::setSpindleMode( int idx ) 
{
    LedNotify_t notify = { spindleModeMap[ idx ], .ledInfo = { .state = true } };
    applyLedStruct( notify );
}

void LEDs::setResetLed( bool value )
{
    LedNotify_t notify = { LED_RESET, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setUnlockLed( bool value )
{
    LedNotify_t notify = { LED_UNLOCK, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setHomeLed( bool value )
{
    LedNotify_t notify = { LED_HOME, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setHoldLed( bool value )
{
    LedNotify_t notify = { LED_HOLD, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setResumeLed( bool value )
{
    LedNotify_t notify = { LED_RESUME, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setFeed100Led( bool value )
{
    LedNotify_t notify = { LED_FEED100, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

// Apply the state of bits relating to spindle, and others.
void LEDs::applySpindle()
{
    uint8_t state = 0xFF;

    if (spindleIdx != -1)
        state &= spindle_bit[spindleIdx];

    if (resetLedSet)
        state &= 0xF7;

    if (resumeLedSet)
        state &= 0xFD;

    if (feed100LedSet)
        state &= 0xFB;

    // xxxxxxx1 = Row 5 Col 2, the Pause button.
    // xxxxxx1x = Row 5 Col 1, The Start/Resume
    // xxxxx1xx = Feed 100%
    // xxxx1xxx = Reset button
    // xxx1xxxx = M5
    // xx1xxxxx = M4
    // x1xxxxxx = M3
    // 1xxxxxxx = Row 5 Col 3. Likely soft reset?

    if (state != m_i2cStateMap[ 0x21 ])
    {
        m_i2cStateMap[ 0x21 ] = state;
        ledSet( 0x21, state );
    }

}


void LEDs::applyLimts()
{
    uint8_t state = 0xFF;
    
    if (rapidIdx != -1)
         state &= rapid_bit[rapidIdx];
 
    if (xlimit)
        state &= 0xFE;
    
    if (ylimit)
        state &= 0xFD;

    if (zlimit)
        state &= 0xFB;

    if (probe)
        state &= 0xF7;

    if (jogCancel)
        state &=0xEF;

    // xxxxxxx1 = X Limit
    // xxxxxx1x = Y Limit
    // xxxxx1xx = Z Limit
    // xxxx1xxx = Probe
    // xxx1xxxx = Row 6 Col 4  EF
    // xx1xxxxx = Row 6 Col 3  DF
    // x1xxxxxx = Row 6 Col 2  BF
    // 1xxxxxxx = Row 6        7F

    if (state != m_i2cStateMap[ 0x22 ])
    {
        m_i2cStateMap[ 0x22 ] = state;
        ledSet( 0x22, state );
    }
}

static LedId rapidMap[] = { LED_RAPID25, LED_RAPID50, LED_RAPID100 };

void LEDs::setRapidLed( int idx)
{
    if (idx < 3)
    {
        LedNotify_t notify = { rapidMap[idx], .ledInfo = { .state = true } };
        applyLedStruct( notify );
    }
}

void LEDs::setXlimit( bool value)
{
    LedNotify_t notify = { LED_XLIM, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setYlimit( bool value)
{
    LedNotify_t notify = { LED_YLIM, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setZlimit( bool value)
{
    LedNotify_t notify = { LED_ZLIM, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setProbe( bool value)
{
    LedNotify_t notify = { LED_PROBE, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setJogLed(bool value)
{
    LedNotify_t notify = { LED_JOG, .ledInfo = { .state = value } };
    applyLedStruct( notify );
}

void LEDs::setStatusLed( const PwmLedState &state)
{
    const int &r = state.r;
    const int &g = state.g;
    const int &b = state.b;

    bool changed = false;
    if (r !=-1 && statusR != r)
    {
        statusR = r;
        changed=true;
    }

    if (g!=-1 && statusG != g)
    {
        statusG = g;
        changed=true;
    }

    if (b!=-1 && statusB != b)
    {  
         statusB = b;
         changed=true;
    }

    if (changed)
    {
        LedUpdate_t update = { .i2cAddress = 0x40, .ledInfo = {  .ppmLedState = { .r = statusR, .g = statusG, .b = statusB  } } };
        xQueueSendToBack( m_LedQueue, &update, 0 );
    }
}

void LEDs::setStatusLed( int r, int g, int b)
{
    LedNotify_t notify = { LED_STATUS, .ledInfo = { .ppmLedState = { .r = r, .g = g, .b = b  } } };
    applyLedStruct( notify );
}

void LEDs::applyRGB( const PwmLedState &state )
{
    //printf("Set pwm rgb: %d %d %d\n", statusR, statusG, statusB );

    m_pwm.setPWM(2,state.r, MAX-state.r);
    m_pwm.setPWM(0,state.g, MAX-state.g);
    m_pwm.setPWM(1,state.b, MAX-state.b);    
}

