#pragma once

#include <Wire.h>

#include "Config.h"

#    include <stdint.h>


class IOExpander 
{

    public:
        IOExpander( TwoWire * wire, int addr );
        ~IOExpander() = default;

        virtual void configure(uint8_t inputs) = 0; // Write 1s for inputs.

        virtual void write(uint8_t data) = 0;
        virtual void read(uint8_t *data) = 0;

    protected:
        TwoWire *m_wire;
        int m_addr;
};