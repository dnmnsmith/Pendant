#include <Arduino.h>
#include <memory>

#include "KeyHandler.h"
#include "Util.h"

KeyHandler::KeyHandler()
{
    //m_keyMap.insert<std::pair<int,std::string> >(  );
    //    std::map< int, std::function<void(int)> > m_keyMap;
    //    std::function< void(const char*) > m_displayFn;
    m_keyMap[0x32] = { { 0x18, 0x00 }, KEY_PRIORITY };// Soft-Reset
    m_keyMap[0x33] = { "$X", KEY_PRIORITY };            // Unlock
    m_keyMap[0x34] = { "$H", KEY_NORMAL };            // Home

    m_keyMap[0x42] = { "G54", KEY_NORMAL };
    m_keyMap[0x43] = { "G55", KEY_NORMAL };
    m_keyMap[0x44] = { "G56", KEY_NORMAL };
    m_keyMap[0x40] = { "G57", KEY_NORMAL };
    m_keyMap[0x41] = { "G58", KEY_NORMAL };
    m_keyMap[0x45] = { "G59", KEY_NORMAL };

    m_keyMap[0x52] = { "~", KEY_PRIORITY }; // Cycle Start/Resume
    m_keyMap[0x53] = { "!", KEY_PRIORITY }; // Feed Hold
    m_keyMap[0x54] = { { (char)0x9E, 0x00 }, KEY_PRIORITY };   // Toggle Spindle Stop
    m_keyMap[0x55] = { "M5", KEY_NORMAL };

    m_keyMap[0x62] = { { (char)0x97, 0x00 }, KEY_PRIORITY };    // Rapid 25%
    m_keyMap[0x63] = { { (char)0x96, 0x00 }, KEY_PRIORITY };    // Rapid 50%
    m_keyMap[0x64] = { { (char)0x95, 0x00 }, KEY_PRIORITY };    // Rapid 100%
    m_keyMap[0x60] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel

    m_keyMap[0x72] = { { (char)0x92, 0x00 }, KEY_PRIORITY };    // Feed --
    m_keyMap[0x73] = { { (char)0x94, 0x00 }, KEY_PRIORITY };    // Feed -
    m_keyMap[0x74] = { { (char)0x90, 0x00 }, KEY_PRIORITY };    // Feed 100%
    m_keyMap[0x70] = { { (char)0x93, 0x00 }, KEY_PRIORITY };    // Feed +
    m_keyMap[0x71] = { { (char)0x91, 0x00 }, KEY_PRIORITY };    // Feed ++

    // Fine jog buttons, all hooked up to jog cancel. Jog cancel everywhere.
    m_keyMap[0x4f] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel
    m_keyMap[0x8f] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel
    m_keyMap[0x5f] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel
    m_keyMap[0x9f] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel
    m_keyMap[0x6f] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel
    m_keyMap[0xaf] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel
    m_keyMap[0xbf] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel
    m_keyMap[0x7f] = { { (char)0x85, 0x00 }, KEY_PRIORITY };    // Jog Cancel

    m_keyMap[0x46] = { { "G53G90G0Z0" }, KEY_NORMAL };        // Safe Z
    


}


KeyHandler::~KeyHandler()
{

}

void KeyHandler::setNormalSendFunction( std::function< void(const char *) > fn )
{
    m_normalSendFn = fn;
}

void KeyHandler::setPrioritySendFunction( std::function< void(const char *) > fn )
{
    m_prioritySendFn = fn;
}

void KeyHandler::setCustomKeyHandler( int keyCode, KeyNotifyFn keyNotifyFn, const char *s )
{
    void *pVoidArg = static_cast<void*>(const_cast<char *>(s));
    setCustomKeyHandler( keyCode, keyNotifyFn, pVoidArg );
}

void KeyHandler::setCustomKeyHandler( int keyCode, KeyNotifyFn keyNotifyFn, void*pArg)
{
   m_customKeyMap.insert( std::pair<int,CustomKeyDef_t>( keyCode, { keyCode, keyNotifyFn, pArg } ) );
}

bool KeyHandler::parse( std::string s  )
{

    // Skip trailing WS
    while ( isspace(s.back()) )
        s.pop_back();

    if (s[0] != '<' || s.back() != '>')
    {
        return false;
    }

    s.pop_back();   // Trailing bracket.
    std::vector<std::string> fields = split(s.substr(1),":");
    if (fields[0].compare("K") != 0 || fields.size() != 2)
        return false;

    int k = strtol( fields[1].c_str(), nullptr, 16 );


    if (m_keyMap.count(k) != 0 && m_normalSendFn != nullptr )
    {
         auto keydef = m_keyMap[ k ];

        std::string command = m_keyMap[ k ].keys;
        if (keydef.priority == KEY_NORMAL && m_normalSendFn != nullptr)
        {
            m_normalSendFn( keydef.keys.c_str() );
        }
        else if (keydef.priority == KEY_PRIORITY && m_prioritySendFn != nullptr)
        {
            m_prioritySendFn( keydef.keys.c_str() );
        }
    }
    else if (m_customKeyMap.count(k) != 0)
    {
        auto customKeyDef = m_customKeyMap[k];
        customKeyDef.keyNotifyFn( k, customKeyDef.pArg );
    }

    return true;
}
