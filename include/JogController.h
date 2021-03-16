#pragma once



#include <Arduino.h>

#include "Position.h"
#include "GrblSettings.h"
#include "GrblState.h"




class JogController
{
    public:
        JogController(GrblSettings &settings, GrblState &state, long dt);
        ~JogController() = default;

        bool parse( std::string line );

        Position &pos() { return m_jogRequest; }

        long getJogSentTime();
        void setJogSentTime();
        long timeSinceJogMs();

        bool poll();

        std::string getCommand();

        void clear();

        void setAposUpdateFn(void (*APosUpdateFn)( float ) ) { m_APosUpdateFn = APosUpdateFn; }

        void enable() { m_enabled = true; }

    private:

        bool m_enabled = false;
        GrblSettings &m_settings;
        GrblState &m_state;
        long m_jogSentTime;
        Position m_jogRequest;
        long m_dt;

        std::string m_command;

        void (*m_APosUpdateFn)( float ) = nullptr;
};