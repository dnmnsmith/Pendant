#include <Arduino.h>

#include <sstream>
#include <iomanip>

#include "Position.h"
#include "Util.h"

void Position::parse( std::string s)
{
    std::size_t start_pos = 0;

    // Tail the string.
    while (isspace(s.back()) || s.back() == ']' )
        s.pop_back();

    // Expect two commas in list of 3 floating point numbers.
    if (std::count(s.begin(), s.end(), ',') != 2)
    {
        throw std::invalid_argument ("Position::parse Failed to parse list in " + s);
    }

    start_pos = s.find_first_of(':');
    if (start_pos == std::string::npos)
        start_pos = 0;
    
    std::vector<std::string> fields = split(s.substr(start_pos),",");
    if (fields.size() != 3)
        throw std::invalid_argument ("Position::parse Failed to parse list in " + s);

    Set( atof(fields[0].c_str()), atof(fields[1].c_str()), atof(fields[2].c_str()), true );
}

std::string Position::ToString() const
{
    std::ostringstream oss;
    oss << name() << ":"; 
    oss << std::fixed << std::setw( 8 ) << std::setprecision( 3 ) << x << ",";
    oss << std::fixed << std::setw( 8 ) << std::setprecision( 3 ) << y << ",";
    oss << std::fixed << std::setw( 8 ) << std::setprecision( 3 ) << z;
    return oss.str();
}

void Position::ToSerial( HardwareSerial &serial ) const
{
    serial.printf("<%s:%.3f,%.3f,%.3f>\n", name().c_str(),x, y, z );
    serial.flush();
}

Position operator-(Position& lhs, const Position& rhs)
{
    Position pos(lhs.name());

    pos.Set( lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, false );

    return pos;
}

Position operator+(Position& lhs, const Position& rhs)
{
    Position pos(lhs.name());

    pos.Set( lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z, false );

    return pos;
}

bool Position::isZero()
{
    bool zero = ( abs(x) <= __FLT_EPSILON__ && abs(y) <= __FLT_EPSILON__ && abs(z) <= __FLT_EPSILON__);
    return zero;
}

void Position::clear(bool notifyIfChanged)
{
    Set( 0.0, 0.0, 0.0, notifyIfChanged );
}

void Position::Set( float newx, float newy, float newz, bool notifyIfChanged )
{
    bool changed = (newx != x) || (newy != y) || (newz != z);

    if (changed)
    {
        x = newx;
        y = newy;
        z = newz;

        if (notifyIfChanged)
        {
            OnChange();
        }
        else
        {
            setDirty(true);
        }
        
    }
}
