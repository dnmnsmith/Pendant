#include <Arduino.h>
#include <sstream>

#include "Globals.h"


void SetAZero(int keycode, void *)
{
  grblState.A.set(0.0);
}

void SendIdleOnlyString( int keycode, void*arg)
{
    const char *command = static_cast<const char *>(arg);

    if (grblState.parserState.get().compare("Idle") ==0)
    {
      SerialBT.println(command);
    }
}

void SendToA( int keycode, void *arg)
{
  char axis = (char)((uintptr_t) arg);

  if (axis == 'X') // X
    grblState.A.set(grblState.WPos.x);
  else if (axis == 'Y') // Y
    grblState.A.set(grblState.WPos.y);
  else if (axis == 'Z') // Z
    grblState.A.set(grblState.WPos.z);
}

void GetFromA( int keycode, void *arg)
{
  char buffer[32];
  
  sprintf(buffer, static_cast<const char *>(arg), grblState.A.get() );

  Serial.println(buffer);
  SerialBT.println( buffer );
}


static const char *moveToPosString[] =
 { 
   "Move to position 1", 
   "Move to position 2", 
   "Move to position 3", 
   "Move to position 4" };

static const char *saveToPosString[] =
  {
    "Saved position 1",
    "Saved position 2",
    "Saved position 3",
    "Saved position 4" };

static const char *dispToPosString[] =
  {
    "Position 1",
    "Position 2",
    "Position 3",
    "Position 4" };


void SavedPosition( int keycode, void *arg)
{
  float x,y,z;
  int idx = (int)arg;

  if (i2cinput.getToggleUp()) // Go position
  {
    eepromValues.get( idx, x, y, z  );
    lcdDisplay.displayPos(moveToPosString[ idx ], x, y, z);
    safeMoveMpos( x, y, z);
  }
  else if (i2cinput.getToggleDown()) // Save position
  {
    grblState.MPos.Get( x, y, z );
    lcdDisplay.displayPos(saveToPosString[ idx ], x, y, z);
    eepromValues.set( idx, x, y, z  );
  }
  else
  {
    eepromValues.get( idx, x, y, z  );
    lcdDisplay.displayPos(dispToPosString[ idx ], x, y, z);
  }
  
}

void TouchProbeHeightHandler( int keycode, void *arg)
{
  if (i2cinput.getToggleDown()) // Save position
  {
    float h = grblState.A.get( );
    eepromValues.setProbeHeight(h);
    lcdDisplay.printf("Probe Height set to:\n%.03f", h);
  }
  else
  {
    lcdDisplay.clear();
    lcdDisplay.printf("Current Probe Height:\n%.03f",eepromValues.getProbeHeight( ));
    grblState.A.set( eepromValues.getProbeHeight());
  }

}

void TouchProbeDistanceHandler( int keycode, void *arg)
{
    if (i2cinput.getToggleDown()) // Save position
    {
      float h = grblState.A.get( );
      eepromValues.setProbeDistance(h);
      lcdDisplay.printf("Probe Dist set to:\n%.03f", h);
    }
    else
    {
      lcdDisplay.clear();
      lcdDisplay.printf("Current Probe Dist:\n%.03f",eepromValues.getProbeDistance( ));
      grblState.A.set( eepromValues.getProbeDistance());
  }

}




void ProbeHandler( char axis, float amount, ProbeMode_t probeMode )
{
  std::ostringstream oss;
  oss << "G38.2" << axis << amount << "F10";

  if (i2cinput.getToggleUp()) // Go position
  {
    grblState.probeSuccess.set(false);
    grblState.motionMode.set( probeMode );

    SerialBT.println( "G91G21" );  // Relative MM
    SerialBT.println( oss.str().c_str() );
    SerialBT.println( "G90G21" );  // Absolute MM
  }
  else
  {
      lcdDisplay.printf("Homing command:\n%s\nKey UP to execute.",oss.str().c_str());
  }
}

void ProbeNegativeKeyHandler( int keycode, void *axis )
{
  ProbeHandler( (char)((int)axis&0xFF), -1.0 * eepromValues.getProbeDistance(), ProbeMode_t::WORKPIECE_PROBE );
}

void ProbePositiveKeyHandler( int keycode, void *axis )
{
  ProbeHandler( (char)((int)axis&0xFF), eepromValues.getProbeDistance(), ProbeMode_t::WORKPIECE_PROBE );
}

void ToolChangePos( int keycode, void *arg )
{
   float x,y,z;
 
  if (i2cinput.getToggleUp()) // Go position
  {
      if (!grblState.IsIdleState())
      {
        lcdDisplay.printf("Normal State Only");
        Serial.printf("ToolChangePos Command not sent, state must be Idle");
      }
      else
      {
        eepromValues.getToolChangePos( x, y, z);
        safeMoveMpos( x, y, z);
      }
  }
  else if (i2cinput.getToggleDown()) // Save position
  {
    if (!grblState.IsIdleState())
      {
        lcdDisplay.printf("Normal State Only");
        Serial.printf("Tool Change Pos not set, state must be Idle");
      }
      else
      {
        grblState.MPos.Get(x, y, z);
        eepromValues.setToolChangePos(x, y, z);
      lcdDisplay.displayPos("Tool Change Pos:", x, y, z);
      }
  }
  else
  {
      eepromValues.getToolChangePos( x, y, z);
      lcdDisplay.displayPos("Tool Change Pos:", x, y, z);
  }
}


void ToolLengthProbePos( int keycode, void *arg )
{
   float x,y,z;
 
  if (i2cinput.getToggleUp()) // Go position
  {
      if (!grblState.IsIdleState())
      {
        lcdDisplay.printf("Normal State Only");
        Serial.printf("ToolLengthProbePos Command not sent, state must be Idle");
      }
      else
      {
        eepromValues.getProbePos( x, y, z);
        safeMoveMpos( x, y, z);
      }
  }
  else if (i2cinput.getToggleDown()) // Save position
  {
    if (!grblState.IsIdleState())
      {
        lcdDisplay.printf("Normal State Only");
        Serial.printf("Tool Length Probe Pos not set, state must be Idle");
      }
      else
      {
        grblState.MPos.Get(x, y, z);
        eepromValues.setProbePos(x, y, z);
        //                 01234567890123456789
        lcdDisplay.displayPos("Tool Len Probe Pos:", x, y, z);
      }
  }
  else
  {
      eepromValues.getProbePos( x, y, z);
      lcdDisplay.displayPos("Tool Len Probe Pos:", x, y, z);
  }
}



void showMPos( int keycode, void *arg )
{
  float x,y,z;
  grblState.MPos.Get(x,y,z);
  lcdDisplay.displayPos("Machine Position:", x, y, z);
}

static const char *goodProbeString = "Probed Position OK";
static const char *badProbeString = "Probed Position NFG";

void showPRB( int keycode, void *arg )
{
  float x,y,z;
  grblState.PRB.Get(x,y,z);
  bool pok = grblState.probeSuccess.get();

  if (i2cinput.getToggleUp()) // Go position
  {
      if (!pok)
      {
          lcdDisplay.clear();
          lcdDisplay.printf("Probe position is NFG");
      }
      else
      {
        safeMoveMpos( x, y, z);
      }
  }
  else
  {
    //                 01234567890123456789
    lcdDisplay.displayPos(pok ? goodProbeString: badProbeString, x, y, z);
  }
      
}

void workOrigin( int keycode, void *arg )
{
  if (i2cinput.getToggleUp()) // Go position
  {
    SerialBT.println( "G53G90G0Z0" );  // Safe Z
    SerialBT.println( "G53G90G0X0Y0" );  // Work XY origin.
  }
  else if (i2cinput.getToggleUp()) // Set XY
  {
     SerialBT.println( "G10L20P1X0Y0" );  // Zero X and Y
  }
  else
  {
    lcdDisplay.printf("Move to WPOS Origin.\nKey UP to execute.");
  }
}

void DoReferenceProbe(int keycode, void *arg)
{
  if (i2cinput.getToggleUp()) // Go position
  {
    ProbeHandler( 'Z', -1.0 * eepromValues.getProbeDistance(), ProbeMode_t::TOOL_REF_PROBE );
  }
  else
  {
    lcdDisplay.printf("Probe Tool Length\nReference\n\nKey UP to execute.");
  }
}

void DoOffsetProbe(int keycode, void *arg)
{
  if (i2cinput.getToggleUp()) // Go position
  {
    ProbeHandler( 'Z', -1.0 * eepromValues.getProbeDistance(), ProbeMode_t::TOOL_LEN_PROBE );
  }
  else
  {
    lcdDisplay.printf("Probe Tool Length\nCalculate Offset\n\nKey UP to execute.");
  }
}

void DoSpindle(int keycode, void *args)
{
  if (i2cinput.getToggleUp()) // Go position
  {
    SendIdleOnlyString(keycode, args);
  }
  else
  {
    lcdDisplay.printf("Spindle Activate\nKey UP to execute.");
  }
}
// G92 sets position. G10L20P1 though?

void SetupCustomKeys()
{
  keyHandler.setCustomKeyHandler( 0x69, SetAZero );

  // Zero out axis GCODE commands.
  keyHandler.setCustomKeyHandler( 0x08, SendIdleOnlyString, "G10L20P1X0" );  // X
  keyHandler.setCustomKeyHandler( 0x67, SendIdleOnlyString, "G10L20P1Y0" );  // Y
  keyHandler.setCustomKeyHandler( 0x66, SendIdleOnlyString, "G10L20P1Z0" );  // Z

  keyHandler.setCustomKeyHandler( 0x78, SendToA, (void*)'X');
  keyHandler.setCustomKeyHandler( 0x07, SendToA, (void*)'Y');
  keyHandler.setCustomKeyHandler( 0x06, SendToA, (void*)'Z');

  keyHandler.setCustomKeyHandler( 0x68, GetFromA, "G10L20P1X%.3f"  );  // X
  keyHandler.setCustomKeyHandler( 0x17, GetFromA, "G10L20P1Y%.3f"  );  // X
  keyHandler.setCustomKeyHandler( 0x16, GetFromA, "G10L20P1Z%.3f"  );  // X

  keyHandler.setCustomKeyHandler( 0x7B, SavedPosition, (void*)0 );
  keyHandler.setCustomKeyHandler( 0x4B, SavedPosition, (void*)1 ); 
  keyHandler.setCustomKeyHandler( 0x2B, SavedPosition, (void*)2 );
  keyHandler.setCustomKeyHandler( 0x0B, SavedPosition, (void*)3 );

  keyHandler.setCustomKeyHandler( 0x6B, TouchProbeHeightHandler );
  keyHandler.setCustomKeyHandler( 0x5B, TouchProbeDistanceHandler );
  keyHandler.setCustomKeyHandler( 0x18, ProbeNegativeKeyHandler, (void*)'X');
  keyHandler.setCustomKeyHandler( 0x57, ProbeNegativeKeyHandler, (void*)'Y');
  keyHandler.setCustomKeyHandler( 0x76, ProbeNegativeKeyHandler, (void*)'Z');

  keyHandler.setCustomKeyHandler( 0x38, ProbePositiveKeyHandler, (void*)'X');
  keyHandler.setCustomKeyHandler( 0x47, ProbePositiveKeyHandler, (void*)'Y');

  keyHandler.setCustomKeyHandler( 0x3B, ToolLengthProbePos );
  keyHandler.setCustomKeyHandler( 0x1B, ToolChangePos );

  keyHandler.setCustomKeyHandler( 0x4A, showMPos );
  keyHandler.setCustomKeyHandler( 0x6A, showPRB );

  keyHandler.setCustomKeyHandler( 0x0A, workOrigin );
  
  keyHandler.setCustomKeyHandler( 0x39, DoReferenceProbe );
  keyHandler.setCustomKeyHandler( 0x19, DoOffsetProbe );

  keyHandler.setCustomKeyHandler( 0x19, DoOffsetProbe );

  keyHandler.setCustomKeyHandler( 0x50, DoSpindle, (void*)"M3 S1000");
  keyHandler.setCustomKeyHandler( 0x51, DoSpindle, (void*)"M4 S1000");
}
