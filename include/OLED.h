#pragma once 

#include <Arduino.h>
#include <Wire.h>


#include <Adafruit_SSD1306.h>

#include "Lockable.h"


class OLED : public Lockable
{
    public:
        static const int MAX_BUF = 64;
    private:
        struct OledMessage
        {
            OLED *instance;
            char buf[ MAX_BUF ];
        };

    public:
        enum FontSize_t
        {
            MEDIUM,
            LARGE
        };
        OLED( uint8_t w, uint8_t h, TwoWire *pTwoWire, SemaphoreHandle_t& mutex, int muxIdx );
        ~OLED() = default;

        static void initQueue();
        static bool poll();

        void init( FontSize_t fontSize = LARGE );

        void printf( const char *format, ...);
        void display(const std::string &str ) { display( str.c_str()); };
        void display(const char *str );
        
    protected:
        void tcaselect(uint8_t i);
        void displayInternal( const char *str );

    private:
        Adafruit_SSD1306 m_display;
        int m_muxIdx;

        static QueueHandle_t m_queue;
};
