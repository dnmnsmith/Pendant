#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>

#include "OLED.h"
#include "Util.h"

#define TCAADDR 0x70

QueueHandle_t OLED::m_queue = nullptr;

void OLED::initQueue()
{
    m_queue = xQueueCreate( 10, sizeof(OledMessage) );    
};

bool OLED::poll()
{
    OledMessage msg;

    if (xQueueReceive(m_queue, (void *const)&msg, 0))
    {
        msg.instance->displayInternal( msg.buf );
        return true;
    }
    return false;
}

OLED::OLED(uint8_t w, uint8_t h, TwoWire *pTwoWire, SemaphoreHandle_t& mutex, int muxIdx)
:  Lockable(mutex),
    m_display(w, h, pTwoWire, -1),
    m_muxIdx( muxIdx )
{

}

  
void OLED::tcaselect(uint8_t i) 
{
    MutexHolder mutexHolder(this);

    if (i > 7) return;

    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();  
}


void OLED::init(FontSize_t fontSize)
{
    MutexHolder mutexHolder(this);

    tcaselect( m_muxIdx );

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!m_display.begin(SSD1306_SWITCHCAPVCC, 0x3C,false,false)) { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    m_display.clearDisplay();

    m_display.setTextSize(1);      // Normal 1:1 pixel scale
    m_display.setTextColor(SSD1306_WHITE); // Draw white text
    m_display.setCursor(0, 24);     // Start at top-left corner
    m_display.cp437(true);         // Use full 256 char 'Code Page 437' font

    switch (fontSize)
    {
        case MEDIUM:
            m_display.setFont(&FreeSans12pt7b);
            break;
        case LARGE:
            m_display.setFont(&FreeSans18pt7b);
            break;
    }
    
    m_display.write("Hello");

    m_display.display();
}

// Forward the display request to the handler.
void OLED::display(const char *str )
{
    OledMessage msg;
    msg.instance = this;
    strncpy( &msg.buf[ 0 ], str, MAX_BUF - 1 );

    xQueueSendToBack(m_queue, &msg, 0);
}


//parserState.c_str()
void OLED::displayInternal(const char *str )
{
    MutexHolder mutexHolder(this);

    tcaselect( m_muxIdx );
    
    m_display.clearDisplay();
    m_display.setCursor(0, 24);     // Start at top-left corner
    m_display.write( str );
    m_display.display();
}


void OLED::printf( const char *format, ...)
{
    char buffer[ 65 ];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 64, format, args);
    display( buffer );
    va_end (args);
}
