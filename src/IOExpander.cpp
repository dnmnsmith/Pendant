
#include "IOExpander.h"

IOExpander::IOExpander( TwoWire * wire, int addr) : m_wire(wire), m_addr(addr)
{
    if ((addr >= 0x20) && (addr <= 0x27)) {
        m_addr = addr;
    } else if (addr <= 0x07) {
        m_addr = 0x20 + addr;
    } else {
        m_addr = 0x27;
    }
}

