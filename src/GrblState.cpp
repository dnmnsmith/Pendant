

#include "GrblState.h"
#include "Util.h"

#include "LcdDisplay.h"

GrblStateEnum_t strToState( const std::string&s)
{
    if (s.compare("Idle") == 0)
        return IDLE;
    else if (s.compare("Run") == 0)
        return RUN;
    else if (s.compare("Hold") == 0)
        return HOLD;
    else if (s.compare("Jog") == 0)
        return JOG;
    else if (s.compare("Alarm") == 0)
        return ALARM;
    else if (s.compare("Door") == 0)
        return DOOR;
    else if (s.compare("Check") == 0)
        return CHECK;
    else if (s.compare("Home") == 0)
        return HOME;
    else if (s.compare("Sleep") == 0)
        return SLEEP;

    throw std::invalid_argument ("Failed to parse state " + s);
}

bool strToWorkspace( const std::string &s, WorkSpace_t &workspace)
{
    bool ok = false;

    if (s.compare("G54") == 0)
        {
            ok = true; 
            workspace = WORKSPACE_G54;
        }
    else if (s.compare("G55") == 0)
        {
            ok = true; 
            workspace = WORKSPACE_G55;
        }
    else if (s.compare("G56") == 0)
        {
            ok = true; 
            workspace = WORKSPACE_G56;
        }
    else if (s.compare("G57") == 0)
        {
            ok = true; 
            workspace = WORKSPACE_G57;
        }
    else if (s.compare("G58") == 0)
        {
            ok = true; 
            workspace = WORKSPACE_G58;
        }
    else if (s.compare("G59") == 0)
        {
            ok = true; 
            workspace = WORKSPACE_G59;
        }

    return ok;
}





bool GrblState::parseGrblLine(std::string s)
{
    bool ok = false;
    WorkSpace_t ws;
    Position pos("Scratch");

    try
    {
    
        // Skip trailing WS
        while ( isspace(s.back()) )
            s.pop_back();

        // Parse returns from $#
        // Workspace [G54:0.000,0.000,0.000]
        if (s[0] == '[' && s.back() == ']')
        {
            //Serial.printf("Parsing : %s\n",s.c_str());

            s.pop_back();   // Trailing bracket.
            std::vector<std::string> fields = split(s.substr(1),":");

            // Serial.printf("Found %d fields in %s\n",fields.size(),s.substr(1).c_str());
            // Serial.printf("First field = %s\n",fields[0].c_str());            



            if (fields[0].compare("MSG") == 0)
            {
//                Serial.printf("MSG : %s\n",s.substr(1).c_str() );
                
                lcdDisplay.display( fields[1] );
                ok = true;
            }
            else if (strToWorkspace( fields[0], ws)) // Parse G54, G55, G56, G57, G58, G59
            {
                pos.parse(fields[1]);
                workspace[ws] = pos;

                if (currentWorkspace == ws)
                    updateWCO();

                ok = true;
            }
            else if (fields[0].compare("G28") == 0)
            {
                pos.parse(fields[1]);
                G28 = pos;
                ok = true;
            }
            else if (fields[0].compare("G30") == 0)
            {
                pos.parse(fields[1]);
                G30 = pos;
                ok = true;
            }
            else if (fields[0].compare("G92") == 0)
            {
                pos.parse(fields[1]);
                G92 = pos;
                updateWCO();
                ok = true;
            }
            else if (fields[0].compare("TLO") == 0)
            {
                TLO.set( (double)atof(fields[1].c_str()) );
                updateWCO();        
                ok = true;
            }

            else if (fields[0].compare("GC") == 0)
            {
                parseGcodeParserState( fields[1]);
                ok = true;
            }
            else if (fields[0].compare("PRB") == 0)
            {
                pos.parse(fields[1]);
                PRB = pos;
                bool oldProbeSuccess = probeSuccess.get();
                probeSuccess.set( fields[2].compare("1") == 0);
                if (probeSuccess.get() && !oldProbeSuccess && m_notifyProbeComplete != nullptr)
                {
                    m_notifyProbeComplete();
                }
                ok = true;
            }
            else
            {
                Serial.printf("SKIPPED parsing %s\n",s.c_str());
            }
            
        }

        // <Idle|MPos:-52.000,0.000,0.000|FS:0,0|Pn:XYZ>
        else if (s[0] == '<' && s.back() == '>')
        {
            //Serial.printf("Parsing : %s\n",s.c_str());

            s.pop_back();   // Remove trailing delimiter.
            std::vector<std::string> fields = split(s.substr(1),"|");

            if (xSemaphoreTake( Mutex, 1000))
            {
                try
                {
                    // Only pins set are in this status message.
                    resetPinState();

                    //Serial.printf("Set parser state to %s\n", fields[0].c_str());
                    // First is the state
                    parserState.set( fields[0] );
                    if (parserState.get().compare("Idle") == 0)
                    {
                        resetRequired.set( false );
                        unlockRequired.set( false );
                        homingRequired.set( false );
                    }

                    for (const std::string &f : fields)
                    {
                        parseStateField( f );
                    }
                    assert( ! (MPos.getResync() & WPos.getResync()));

                    if (MPos.getResync())
                    {
                        //Serial.println("MPos dirty, recalculating");
                        MPos.setDirty(false);
                        MPos.Set( WPos.x + WCO.x, WPos.y + WCO.y, WPos.z + WCO.z );
                        MPos.setResync(false);
                    }
                    else if (WPos.getResync())
                    {
                        //Serial.println("WPos dirty, recalculating");
                        WPos.setDirty(false);
                        WPos.Set( MPos.x - WCO.x, MPos.y - WCO.y, MPos.z - WCO.z );
                        WPos.setResync(false);
                    }

                    // Flush any dirty pin states.
                    notifyPinState();

                    ok = true;
                }
                catch(const std::exception& e)
                {
                    ok = false;
                }
                
                xSemaphoreGive(Mutex);
            }
            
        }
        /* code */
    }
    catch(const std::exception& e)
    {
        Serial.printf("Parsing : %s failed with %s\n",s.c_str(),e.what());
        ok = false;
    }
    return ok;
}

void  GrblState::parseStateField(std::string s)
{

    std::vector<std::string> fields = split(s,":");
    std::string name = fields[0];
    for (auto & c: name) c = toupper(c);

    //Serial.printf("Parse state fields %s %s\n",s.c_str(),fields[0].c_str());

    if (fields[0].compare("MPos") == 0)
    {
//        Serial.printf("Found MPos\n");
        MPos.parse(fields[1]);
        MPos.setResync(false);        
        WPos.setResync(true);
    }
    else if (fields[0].compare("WPos") == 0)
    {
        WPos.parse(fields[1]);
        WPos.setResync(false);        
        MPos.setResync(true);
    }
    else if (fields[0].compare("WCO") == 0)
    {
        WCO.parse(fields[1]);
    }

    // Ov:96,100,100
    else if (fields[0].compare("Ov") == 0)
    {
        std::vector<std::string> values = split(fields[1],",");

        overrideFeedPercent.set( atoi(values[0].c_str()) );

        overrideRapidPercent.set( atoi(values[1].c_str()) );
        overrideSpindlePercent.set( atoi(values[2].c_str()) );
    }
    // FS:0,0       Feed Rate, Spindle Speed
    else if (fields[0].compare("FS") == 0)
    {
        std::vector<std::string> values = split(fields[1],",");

        feedRate.set( atoi(values[0].c_str()) );
    }
    // F:0          Feed Rate
    else if (fields[0].compare("F") == 0)
    {
        feedRate.set( atoi(fields[1].c_str()) );
    }
    // Line Ln:nnnn
    else if (fields[0].compare("Ln") == 0)
    {
        lineNumber = atoi(fields[1].c_str());
    }
    // Pin State
    else if (fields[0].compare("Pn") == 0)
    {
        door = fields[1].find('D') != std::string::npos;
        hold = fields[1].find('H') != std::string::npos;
        reset = fields[1].find('R') != std::string::npos;
        start = fields[1].find('S') != std::string::npos;
        limitX.set( fields[1].find('X') != std::string::npos );
        limitY.set( fields[1].find('Y') != std::string::npos );
        limitZ.set( fields[1].find('Z') != std::string::npos );
        probe.set( fields[1].find('P') != std::string::npos );
    }
}

//GC:G0 G54 G17 G21 G90 G94 M5 M9 T0 F0 S0
void GrblState::parseGcodeParserState(std::string s)
{
    std::vector<std::string> values = split(s," ");
    //Serial.printf("Parsing GC, %d fields, WS=%s\n",values.size(),values[1].c_str());
    
    for (int i = 0; i < values.size(); i++)
    {
        if (strToWorkspace(values[i], currentWorkspace) )
        {
            updateWCO();

            WPos.Set( MPos.x - WCO.x, MPos.y - WCO.y, MPos.z - WCO.z );
            WPos.setResync(false);

            if (m_notifyWorkspaceFn != nullptr)
                m_notifyWorkspaceFn( &workspace[ currentWorkspace], (void *)currentWorkspace );
        }
        else if (values[i].substr(0,1).compare("S") == 0)
        {
            spindleSpeed.set(  atoi(values[i].substr(1).c_str()) );
        }
        else if (values[i].compare("M3") == 0 || values[i].compare("M4") == 0 || values[i].compare("M5") == 0 )
        {
            spindleState.set( values[i] );
        }
    }

    if (m_notifySpindleFn != nullptr)
    {
        int state = (int)spindleState.get()[1] - (int)'3';
//        Serial.printf("Spindle state =%s (%d)\n",spindleState.get().c_str(), state );
        m_notifySpindleFn( nullptr, (void*)state );
    }
}

// WCO:0.000,1.551,5.664 is the current work coordinate offset of the g-code parser,
// which is the sum of the current work coordinate system, G92 offsets, and G43.1 tool length offset.
void GrblState::updateWCO()
{
    Position oldWCO = Position(WCO);
    WCO = workspace[currentWorkspace];
    WCO.z = WCO.z + TLO.get();
    WCO = WCO + G92;
    WCO.OnChange();

    if (WCO != oldWCO)
        WPos.setResync(true);
}

void GrblState::resetPinState()
{
    door = false;
    hold = false;
    reset = false;
    start = false;
    limitX.set( false, false );
    limitY.set( false, false );
    limitZ.set( false, false );
    probe.set( false, false );
}

void GrblState::notifyPinState()
{
    limitX.NotifyIfDirty(  );
    limitY.NotifyIfDirty(  );
    limitZ.NotifyIfDirty(  );
    probe.NotifyIfDirty(  );
}

void GrblState::UpdateA( float delta )
{
    {
        GrblStateMutexHelper holder( *this );

        float newVal = A.get() + delta;

        A.set( newVal,false);
    }

    A.NotifyIfDirty();
}

bool GrblState::IsIdleState()
{
 {
    GrblStateMutexHelper holder( *this );
    return (parserState.get().compare("Idle") == 0);
  }


}
