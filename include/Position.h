#pragma once

#include <Arduino.h>
#include "State.h"

class Position : public State
{ 
    public:
        Position(std::string name ) : State(name) { };
//        Position( float xInit, float yInit, float zInit ) : x(xInit),y(yInit),z(zInit) {}
        ~Position() = default;

        Position( std::string name, float x1, float y1, float z1 ) : 
            State(name), x(x1) , y(y1), z(z1) 
            {
                
            }

        Position( const Position & other) : State ( other )
        {
            Set( other.x, other.y, other.z, false );
            m_resync = false;
        }
        Position & operator= ( const Position & other ) 
        {
//          By default, don't over-write name and notify callback on assignment.
//            State::operator=(other);            
            Set( other.x, other.y, other.z, false );
            m_resync = false;

            return *this;
        }
        bool operator ==(const Position &b) const
        {
            return x == b.x && y == b.y && z == b.z;
        }
        bool operator !=(const Position &b) const
        {
            return x != b.x || y != b.y || z != b.z;
        }

        void Sum( const Position &other)
        {
            Set( x + other.x, y + other.y,  z + other.z, false );
        }

        void Set( float newx, float newy, float newz, bool notifyIfChanged = true );
        void Get( float &xout, float &yout, float &zout ) { xout =x; yout = y; zout = z; }
        
                ///
        /// Parse something like:
        ///  "[G54:0.000,0.000,0.000]
        /// 
        /// Or just the number part:
        ///     "0.000,0.000,0.000"
        ///
        void parse( std::string s);

        bool isZero();

        void clear( bool notifyIfChanged = true );

        bool getResync() { return m_resync; }
        void setResync( bool resync = true ) { m_resync = resync; }

        float x = 0.0;
        float y = 0.0;
        float z = 0.0;

        bool m_resync = false;

        std::string ToString() const;
        void ToSerial( HardwareSerial &serial ) const;

        friend Position operator-(Position& lhs, const Position& rhs);
        friend Position operator+(Position& lhs, const Position& rhs);
};

