#pragma once

#include <deque>
#include <Arduino.h>

#include <LiquidCrystal_I2C.h>

#include "Lockable.h"

class LcdDisplay : public Lockable
{
    public:
        static const int ROWS=4;
        static const int COLS=20;

        LcdDisplay(SemaphoreHandle_t& mutex) : Lockable(mutex), m_lcd(0x27,COLS,ROWS) { }
        ~LcdDisplay() = default;

        void init();

        void display(const std::string &s, bool redraw = true );
        void displayPos(const char *s, float x, float y, float z );
        void display(const char *s, bool redraw = true);
        void printf( const char *format, ...);

        bool poll();

        void clear();

    private:
        void clearBuffer();
        
        void redrawScreen(); 

        void displayInternal(const char *s, bool redraw );

        LiquidCrystal_I2C m_lcd; 
        
        char m_buf[ROWS][COLS+1] ;
        int m_bufTop = 0;

        QueueHandle_t m_DisplayQueue = nullptr;
};

extern LcdDisplay lcdDisplay;
