#pragma once

#include <functional>
#include <map>
#include <Arduino.h>



class KeyHandler
{

    enum KeyPriority
    {
        KEY_PRIORITY,       // Send any time.
        KEY_NORMAL,         // Send only Idle.
        KEY_INTERLOCK,      // Send only when unlock engaged.
    };

    struct KeyDef_t
    {
        std::string keys;
        KeyPriority priority;
    };

    public:
        KeyHandler();
        ~KeyHandler();

        typedef void (*KeyNotifyFn)(int, void*);

        struct CustomKeyDef_t
        {
            int keyCode;
            KeyNotifyFn keyNotifyFn;
            void *pArg;
        };

        bool parse( std::string s );

        void setNormalSendFunction( std::function< void(const char *) > fn );
        void setPrioritySendFunction( std::function< void(const char *) > fn );

        void setCustomKeyHandler( int keyCode, KeyNotifyFn keyNotifyFn, const char *s );
        void setCustomKeyHandler( int keyCode, KeyNotifyFn keyNotifyFn, void* pArg = nullptr);

    private:
        std::map< int, KeyDef_t> m_keyMap;
        std::map< int, CustomKeyDef_t> m_customKeyMap;

        std::function< void(const char*) > m_displayFn = nullptr;
        std::function< void(const char*) > m_normalSendFn = nullptr;
        std::function< void(const char*) > m_prioritySendFn = nullptr;

};

