#include "LcdDisplay.h"
#include <algorithm>
#include <stdarg.h>

#include "Util.h"


static const char * spaces = "                    ";

// At the moment, if clear is true it over-rides the other fields, and they are ignored.
struct DisplayNotifyStruct { char s[21]; bool redraw; bool clear; };

void LcdDisplay::init()
{
    m_DisplayQueue = xQueueCreate( 10, sizeof(DisplayNotifyStruct) );

    {
        MutexHolder holder(this);
        m_lcd.init();
        m_lcd.backlight();
    }

    clearBuffer();
    redrawScreen();
}


void LcdDisplay::clearBuffer()
{
    for (int i = 0; i < ROWS; i++)
    {
        memcpy( &m_buf[i][0], spaces, COLS + 1 );
    }  
    m_bufTop = 0;
}

void LcdDisplay::display( const std::string &s, bool redraw )
{
    display( s.c_str(), redraw );
}

void LcdDisplay::printf( const char *format, ...)
{
    char buffer[ 128 ];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 127, format, args);

    char *p = buffer;
    char *q = strchr( p, '\n');

    while (q != nullptr)
    {
        *q++ = 0;
        const char *r = p;
        p = q;
        q = strchr( p, '\n');

        display( r, (q == nullptr) && (p[0] == 0) );
    }

    if (p[0] != 0)
        display( p, true );

    va_end (args);
}

void  LcdDisplay::clear()
{
    // Empty the queue, these messages are just going to get blanked.
    while (uxQueueMessagesWaiting( m_DisplayQueue ) > 0)
    {
        DisplayNotifyStruct discard;
        xQueueReceive( m_DisplayQueue, &discard, 0 );
    }

    DisplayNotifyStruct displayNotifyStruct;
    displayNotifyStruct.clear = true;
    xQueueSendToBack(m_DisplayQueue, &displayNotifyStruct, 0);
}


void LcdDisplay::display(const char *s, bool redraw )
{
    int discarded = 0;

    DisplayNotifyStruct displayNotifyStruct;
    displayNotifyStruct.redraw = redraw;
    displayNotifyStruct.clear = false;

    int len = (int)strlen(s);
    if (len > COLS )
        len = COLS;

    strncpy( displayNotifyStruct.s, s, len );

    if (len < COLS)
    {
        strncpy( &displayNotifyStruct.s[len], spaces, COLS-len);
    }
    
    displayNotifyStruct.s[COLS]=0;

    assert( strlen(displayNotifyStruct.s) == COLS );

    // Display can only display so many rows at a time. 
    // If there are more messages queued up than this, the older ones 
    // simply won't be seen as they'll get immediately scrolled off the top, so trim the oldest ones off
    // the queue before adding the new one.
    while (uxQueueMessagesWaiting( m_DisplayQueue ) >= ROWS)
    {
        DisplayNotifyStruct discard;
        xQueueReceive( m_DisplayQueue, &discard, 0 );
        discarded++;
    }

    xQueueSendToBack(m_DisplayQueue, &displayNotifyStruct, 0);

    if (discarded != 0)
    {
        Serial.printf( "Discarded %d messages\n", discarded );
    }
    //Serial.println("Queued display message");
}




void LcdDisplay::displayPos(const char *s, float x, float y, float z )
{
    printf("%s\nX=%.03f\nY=%.03f\nZ=%.03f", s, x, y, z);
}

bool LcdDisplay::poll()
{
    DisplayNotifyStruct displayNotifyStruct;

    if (xQueueReceive(m_DisplayQueue, (void *const)&displayNotifyStruct, 0))
    {
        //Serial.println("Got message to display");

        if (displayNotifyStruct.clear)
        {
            clearBuffer();
            redrawScreen();
        }
        else
        {
         displayInternal( &displayNotifyStruct.s[ 0 ], displayNotifyStruct.redraw );
        }
        return true;
    }
    return false;
}


void LcdDisplay::displayInternal(const char *s, bool redraw)
{    
    //reportStats();

    assert( strlen(s) == COLS );

    memcpy( &m_buf[m_bufTop][0], s, COLS+1 );

    m_bufTop = (m_bufTop + 1) % ROWS;

    if (redraw)
        redrawScreen();
}

void LcdDisplay::redrawScreen()
{
    MutexHolder holder( this );

    m_lcd.clear();

    for (int i = 0; i < ROWS; i++)
    {
        m_lcd.setCursor(0,i);
        m_lcd.print( m_buf[ (i + m_bufTop) % ROWS] );
    }
}
