#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <sstream>
#include <iomanip>
#include <functional>

#include "GrblState.h"
#include "GrblSettings.h"
#include "JogController.h"
#include "KeyHandler.h"

#include "BluetoothSerial.h"
#include "I2CScan.h"
#include "SpecialKeys.h"


#include <Adafruit_MCP23008.h>

#include "LcdDisplay.h"
#include "EepromValues.h"
#include "I2CInput.h"

#include "OLED.h"

#include "LEDs.h"

static const uint8_t SCREEN_WIDTH  = 128; // OLED display width, in pixels
static const uint8_t SCREEN_HEIGHT = 32; // OLED display height, in pixels

SemaphoreHandle_t mutexI2C = nullptr;

LcdDisplay lcdDisplay( mutexI2C );
EepromValues eepromValues( mutexI2C );
LEDs leds( &Wire, mutexI2C);
OLED stateOled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, mutexI2C, 0);
OLED tloOled(SCREEN_WIDTH, 2 * SCREEN_HEIGHT, &Wire,  mutexI2C, 1);
OLED feedOled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,  mutexI2C, 2);
OLED feedPercentOled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,  mutexI2C, 3);
I2CInput i2cinput(&Wire, mutexI2C );

int jogDT = 100;  // Millisecs


BluetoothSerial SerialBT;
Adafruit_MCP23008 mcp;

// Define CS pin for the SD card module
#define SD_CS 5



//String MACadd = "AA:BB:CC:11:22:33";
//uint8_t address[6]  = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
//uint8_t address[6]  = {0x00, 0x1D, 0xA5, 0x02, 0xC3, 0x22};
String name = "btgrblesp";
//char *pin = "1234"; //<- standard pin would be provided by default
bool connected = false;


static TaskHandle_t bluetoothTaskHandle = 0;
static TaskHandle_t displayTaskHandle = 0;

int intCount = 0;

GrblState grblState;
GrblSettings grblSettings;
JogController jogController = JogController( grblSettings, grblState, 100 );
KeyHandler keyHandler = KeyHandler();

QueueHandle_t xStateChangeNotifyQueue = nullptr;

void CommsLoop(void* pvParameters);
bool ProcessGrblNotification(const std::string&line);



void WorkspaceUpdate(State* state, void* param ) 
{
  int ws = (int)param;
  //Serial.printf("Workspace notify : %s %d\n", state->name().c_str(), ws );
  leds.setWsLed( ws );
}

void PrioritySendFn(const char *s)
{
   //Serial.printf("Send : 0x%02X\n", int(s[0]));

  // Special case soft-reset
  if (s[0] == (char)0x85)  // Jog Cancel
  {
    jogController.clear();  // Also clear outstanding jog move.
  }

  SerialBT.println(s);
}

void NormalSendFn(const char *s)
{
  std::string parserState;
  {
    GrblStateMutexHelper lock(grblState);
    parserState = grblState.parserState.get();
  }

  if (parserState == "Idle")
  {
    Serial.println(s);
    SerialBT.println(s);
  }
  else
  {
    lcdDisplay.printf("Normal State Only");
    Serial.printf("Command %s not sent, state is %s\n",s,parserState.c_str());
  }
}

void SpindleUpdate(State*state, void* param)
{
  int mode = (int)param;
  //Serial.printf("Spindle notify : %d\n", mode );
  leds.setSpindleMode( mode );
}


void RapidOverrideUpdate(State* state, void* param ) 
{
  int rapid = 0;

  {
      GrblStateMutexHelper lock(grblState);
      rapid = grblState.overrideRapidPercent.get();
  }

  //Serial.printf("Rapid Over-ride set to %d\n", rapid);

  if (rapid == 25)
    leds.setRapidLed( 0 );
  else if (rapid == 50)
    leds.setRapidLed( 1 );
  else if (rapid == 100)
    leds.setRapidLed( 2 );
  else
  {
      Serial.printf("ERROR = Unexpected RAPID over-ride %d\n", rapid );
  }
  
}

void StatusBitUpdate(State* state, void* param ) 
{
  if (state == &grblState.limitX)
    leds.setXlimit( grblState.limitX.get());
  else if (state == &grblState.limitY)
    leds.setYlimit( grblState.limitY.get());
  else if (state == &grblState.limitZ)
    leds.setZlimit( grblState.limitZ.get());
  else if (state == &grblState.probe)
    leds.setProbe( grblState.probe.get());
}

void ResetUnlockUpdate(State* state, void* param ) 
{
  if (state == &grblState.resetRequired)
  {
    leds.setResetLed( grblState.resetRequired.get() );
  }
  else if (state == &grblState.unlockRequired)
  {
    leds.setUnlockLed( grblState.unlockRequired.get() );

  }
  else if (state ==  & grblState.homingRequired)
  {
    leds.setHomeLed( grblState.homingRequired.get() );
  }

}

void GrblWposUpdate(State* state, void* param ) 
{
  // If currently displayed position notifies, then
  // update the display.
  //Serial.println("WPOS updated");
  Position *pos =(Position*)(state);
  pos->ToSerial( Serial2 );
  //pos->ToSerial( Serial );
}

void AUpdate(State* state, void* param ) 
{
  float value;
  {
    GrblStateMutexHelper lock(grblState);
    value = grblState.A.get();
  }

  std::ostringstream oss;
  oss << "<A" << ":" << std::fixed << std::setw( 3 ) << std::setprecision( 3 ) << value << ">";

  Serial2.println(oss.str().c_str());
}

void GrblTloUpdate(State* state, void* param ) 
{
  tloOled.printf( "TLO:\n%0.3f",grblState.TLO.get() );
}


void GrblStateUpdate(State* state, void* param ) 
{
  //Serial.printf("Update recieved for %s\n", state->name().c_str() );

  if (state->name().compare("GrblState") == 0)
  {
    std::string parserState;
    {
      GrblStateMutexHelper lock(grblState);
      parserState = grblState.parserState.get();
    }
    //Serial.printf("Grbl Parser State is now %s\n",parserState.c_str() );

    leds.setJogLed( parserState.compare("Jog") == 0 );

    stateOled.display( parserState.c_str() );
  }
  else if (state->name().compare("WCO") == 0)
  {
    //Serial.printf("WCO Updated\n");
  }
  else if (state == &grblState.overrideFeedPercent)
  {
    int feed = 100;

    {
      GrblStateMutexHelper lock(grblState);
      feed = grblState.overrideFeedPercent.get();
    }

    leds.setFeed100Led( feed == 100 );
    feedPercentOled.printf("%d%%", feed);
  }
  else if (state == &grblState.feedRate)
  {
    int feed = 100;

    {
      GrblStateMutexHelper lock(grblState);
      feed = grblState.feedRate.get();
    }

    feedOled.printf( "%d", feed);
  }


} 

void notifyStateFn(State *state,void *param)
{ 
    StateNotifyStruct stateNotifyStruct = { GrblStateUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 1000);
};

void notifyWposFn(State *state,void *param)
{ 
    StateNotifyStruct stateNotifyStruct = { GrblWposUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 1000);
};

void notifyAFn(State *state,void *param)
{ 
    StateNotifyStruct stateNotifyStruct = { AUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 1000);
};


void notifyTloFn(State *state,void *param)
{ 
    //Serial.printf("notifyTloFn. Messages waiting = %d\n", uxQueueMessagesWaiting(xStateChangeNotifyQueue));

    StateNotifyStruct stateNotifyStruct = { GrblTloUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 0 );
};


void notifyWorkspaceFn( State *state, void *param)
{
    StateNotifyStruct stateNotifyStruct = { WorkspaceUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 0);
}

void notifySpindleFn( State *state, void *param)
{
    StateNotifyStruct stateNotifyStruct = { SpindleUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 0);
}

void notifyRapidFn( State *state, void *param)
{
    StateNotifyStruct stateNotifyStruct = { RapidOverrideUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 0);
}

void notifyStatusBitFn( State *state, void *param)
{
    StateNotifyStruct stateNotifyStruct = { StatusBitUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 0);
}

void notifyResetUnlockStatus( State *state, void *param)
{
    StateNotifyStruct stateNotifyStruct = { ResetUnlockUpdate, state, param };
    xQueueSendToBack(xStateChangeNotifyQueue, &stateNotifyStruct, 0);
}

void SDSetup()
{
 // Initialize SD card
  SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/keys.txt");
  if(!file) {
    Serial.println("File doesn't exist");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

}

// Display thread loop, mostly Wire accessors needing mutexing. Get the display serial handling away from the
// main parsing/update loop.
void DisplayLoop(void*)
{
    while (1)
    {
        lcdDisplay.poll();
        leds.poll();
        OLED::poll();

        vTaskDelay(1);

        // if ( (xTaskGetTickCount() - lastPrint) > pdMS_TO_TICKS( 1000))
        // {
        //     lastPrint = xTaskGetTickCount();
        //     Serial.printf("lcdHandled=%d, ledHandled=%d, oledHandled=%d\n", lcdHandled, ledHandled, oledHandled );
        //     lcdHandled = 0;
        //     ledHandled = 0;
        //     oledHandled = 0;
        // }
    }
}

void notifyProbeComplete()
{
  float x,y,z;
  float xr, yr, zr;
  grblState.PRB.Get(x,y,z);
  lcdDisplay.displayPos("Probe Complete", x, y, z);

  ProbeMode_t probeMode = grblState.motionMode.get();
  grblState.motionMode.set( ProbeMode_t::NONE );

  switch (probeMode)
  {
      case ProbeMode_t::NONE:
        // Huh? May have been activated by UI independently. Ignore.
        break;

      case ProbeMode_t::WORKPIECE_PROBE:
        // Up to the user to do the necessary on workpiece probe.
        break;

      case ProbeMode_t::TOOL_REF_PROBE:
        // Set the tool length reference
        eepromValues.setTloRefPos( x, y, z );
        SerialBT.printf("G43.1 Z0.0\n");
        break;

      case ProbeMode_t::TOOL_LEN_PROBE:
        eepromValues.getTloRefPos( xr, yr, zr );

        float tlo = x - xr;
        SerialBT.printf("G43.1 Z%.03f\n", tlo);

        Serial.printf("TLO calculated as %.03f\n", tlo );
        lcdDisplay.printf("TLO calculated as:\n%.03f",tlo);
        break;
  }
}



void setup() 
{
  Serial.begin(115200);
  Serial2.begin(115200);

  mutexI2C = xSemaphoreCreateRecursiveMutex();

  OLED::initQueue();

  eepromValues.init();
  
  Serial.printf("Init Wire1...\n");
  if ( !Wire.begin(21,22,100000) && ! !Wire.begin(21,22,100000))
  {
      Serial.printf("Faied to init Wire1\n");
  }

  Serial.printf("Init Wire2...\n");
  if (!Wire1.begin(25,26,100000) && !Wire1.begin(25,26,100000))
  {
      Serial.printf("Faied to init Wire2\n");
  }

  i2cinput.init();
 
  SPI.begin();
  
  lcdDisplay.init();

  lcdDisplay.display("Hello");

  i2cScan( "Wire 1", &Wire );
  i2cScan( "Wire 2", &Wire1 );

  mcp.begin( 20, &Wire1 );
  for (int i = 0; i < 8; i++)
  {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);  // turn on a 100K pullup internally
  }


  pinMode(36, INPUT_PULLUP);
 
  stateOled.init(OLED::FontSize_t::LARGE);
  tloOled.init(OLED::FontSize_t::MEDIUM);
  feedOled.init(OLED::FontSize_t::LARGE);
  feedPercentOled.init(OLED::FontSize_t::LARGE);

  stateOled.printf("Disco!");

  SDSetup();


  xTaskCreatePinnedToCore(DisplayLoop,    // task
                          "displayTask",       // name for task
                          4096,                // size of task stack
                          nullptr,             // parameters
                          10,                  // priority
                          &displayTaskHandle,
                          1 ); // core


  //SerialBT.setPin(pin);
  SerialBT.begin("GrblCtl", true); 
  //SerialBT.setPin(pin);
  //Serial.println("The device started in master mode, make sure remote BT device is on!");
  
  // connect(address) is fast (upto 10 secs max), connect(name) is slow (upto 30 secs max) as it needs
  // to resolve name to address first, but it allows to connect to different devices with the same name.
  // Set CoreDebugLevel to Info to view devices bluetooth address and device names
  lcdDisplay.display("Connecting Bluetooth");
  Serial.println("Connecting...");

  do
  {
    connected = SerialBT.connect(name);
  } 
  while (!SerialBT.hasClient());


  //esp_log_level_set(const char *tag, esp_log_level_t level);

  //connected = SerialBT.connect(address);
  
  if(connected) {
    lcdDisplay.display("Connected");

    Serial.println("Connected Successfully!");
    
  } else {
      lcdDisplay.display("Connect failed");
    while(!SerialBT.connected(10000)) {
      Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app."); 
    }
  }

  // disconnect() may take upto 10 secs max
  //if (SerialBT.disconnect()) {
  //  Serial.println("Disconnected Succesfully!");
  //}
  // this would reconnect to the name(will use address, if resolved) or address used with connect(name/address).
  //SerialBT.connect();

  keyHandler.setNormalSendFunction(NormalSendFn);
  keyHandler.setPrioritySendFunction(PrioritySendFn);


    xStateChangeNotifyQueue = xQueueCreate( 64, sizeof(StateNotifyStruct) );

    grblState.parserState.SetNotify( notifyStateFn );

    for (int i = 0; i < WORKSPACE_COUNT; i++ )
      grblState.workspace[i].SetNotify( notifyStateFn );

    grblState.SetWorkspaceNotify( notifyWorkspaceFn );
    grblState.SetSpindleNotify( notifySpindleFn );
    grblState.overrideFeedPercent.SetNotify( notifyStateFn );
    grblState.feedRate.SetNotify( notifyStateFn );
    grblState.WPos.SetNotify( notifyWposFn );
    grblState.TLO.SetNotify( notifyTloFn );
    grblState.overrideRapidPercent.SetNotify( notifyRapidFn );    
    
    grblState.limitX.SetNotify(notifyStatusBitFn );
    grblState.limitY.SetNotify(notifyStatusBitFn );
    grblState.limitZ.SetNotify(notifyStatusBitFn );
    grblState.probe.SetNotify(notifyStatusBitFn );

    grblState.resetRequired.SetNotify(notifyResetUnlockStatus);
    grblState.unlockRequired.SetNotify(notifyResetUnlockStatus);
    grblState.homingRequired.SetNotify(notifyResetUnlockStatus);

    jogController.setAposUpdateFn( [](float delta ) { grblState.UpdateA(delta); } );
    
    grblState.A.SetNotify( notifyAFn );

    grblState.setNotifyProbeComplete( notifyProbeComplete );

    SetupCustomKeys();

    leds.init();

    grblSettings.setOnInitialised( [](){ 
      //Serial.println("Settings receieved");
      jogController.enable(); 
      leds.setStatusLed( 0, -1, -1); } );

    // Set red until settings received back.
    leds.setStatusLed( LEDs::MAX, -1, -1);

    xTaskCreatePinnedToCore(CommsLoop,    // task
                            "bluetoothTask",  // name for task
                            16384,               // size of task stack
                            nullptr,               // parameters
                            18,                  // priority
                            &bluetoothTaskHandle,
                            0  // core
    );

}


bool ProcessGrblNotification(const std::string&line)
{
  bool ok = false;
  size_t term = line.find_last_of(']');
  
  //Serial.println(line.c_str());

  if (line.compare(0, 5, "[MSG:") == 0 && term != std::string::npos)
  {
    std::string msg = line.substr(5,term-5);
    ok = true;    

    if (msg.compare(0,16,"BT Connected with") == 0)
      msg[17]='\n';

    lcdDisplay.printf( "%s", msg.c_str() );
    
    Serial.printf("MSG : %s\n", msg.c_str() );

    if (msg.compare("'$H'|'$X' to unlock") == 0)
    {
      grblState.resetRequired.set(false);
      grblState.unlockRequired.set( true );
      grblState.homingRequired.set( true );

    }
    else if (msg.compare("Caution: Unlocked") == 0)
    {
      grblState.resetRequired.set(false);
      grblState.unlockRequired.set(false);
      grblState.homingRequired.set( true );
    }
    else if (msg.compare("Reset to continue") == 0)
    {
      grblState.resetRequired.set(true);
      grblState.unlockRequired.set(false);
      grblState.homingRequired.set( false );
    }
  }
  else if (line.compare(0, 6, "ALARM:") == 0)
  {
    int alarmcode = atoi( &line.c_str()[6] );    
    ok = true;

    lcdDisplay.display( line );
    lcdDisplay.display( getAlarmString( alarmcode ) );

    Serial.printf("%s<<<\nALARM : %s\n", line.c_str(), getAlarmString( alarmcode ));

    grblState.unlockRequired.set( true ); // Homing fail.
  }
  else if (line.compare(0,6, "error:") == 0)
  {
    lcdDisplay.display( line );
    Serial.println( line.c_str() );
    ok = true;
  }
  else if (line.compare(0,2,"ok") == 0)
  {
    ok = true;
  }
  return ok;
}


bool  ProcessGrblLine(std::string line)
{
  bool ok = false;

  trim( line );

  //Serial.printf("Processing : %s\n",line.c_str());
  ok |= ProcessGrblNotification(line);
  ok |= grblState.parseGrblLine(line);
  ok |= grblSettings.parse(line);

  return ok;
}

bool ParseMegaConnection(const std::string&line)
{
  bool ok = (line.compare(0, 11, "<MegaHello>") == 0);

  if (ok)
  {
    Serial.println("Mega connected.");

    // Mega turn on. Send all positions.
    GrblWposUpdate( &grblState.WPos, nullptr );
    AUpdate( &grblState.A, nullptr );
  }

  return ok;
}

bool ProcessSerialLine(const std::string&line)
{
  //Serial.printf("Processing : %s\n",line.c_str());

  bool ok = false;
  bool jogWasZero = jogController.pos().isZero();

  ok |= ParseMegaConnection( line );
  ok |= jogController.parse( line );

  // If there is an active probe mode, the a keypress will cancel.
  ProbeMode_t prevProbeMode = grblState.motionMode.get();
  if (!ok && keyHandler.parse(line))
  {
    ok = true;
    if ( prevProbeMode != ProbeMode_t::NONE )
      grblState.motionMode.set( ProbeMode_t::NONE );
  }
  
  bool jogZero = jogController.pos().isZero();

  // Begin a jog, after DT, 
  if (jogWasZero && !jogZero)
  {
    //Serial.printf("New Jog Scheduled\n");

    jogController.setJogSentTime();
  }

  return ok;
}

char btBuf[128];
int btpos = 0;
char prevBtCh = 0;


char serBuf[128];
int serPos = 0;

bool settingsInitialised = false;

int lastBtMessageSeen = 0;

void CommsLoop(void* pvParameters)
{
  bool ok = true;
  long timePollPosition = xTaskGetTickCount();
  long timePollState = xTaskGetTickCount();

  SerialBT.println("$$");
  SerialBT.flush();

  lastBtMessageSeen = xTaskGetTickCount();

  while (ok)
  {
    if (SerialBT.available()) 
    {
        lastBtMessageSeen = xTaskGetTickCount();
        leds.setStatusLed( 0, LEDs::MAX, -1);

        char ch = SerialBT.read();
        btBuf[btpos] = ch;
        btpos++;

        if (ch==0xA && prevBtCh == 0xD)
        {
            btBuf[btpos] = 0;
            ProcessGrblLine(btBuf);
            btpos = 0;
            ch=0;
        }

        // Over-run. Discard.
        // N.B. Allow extra char for terminator.
        // TODO - restart bluetooth?
        if (btpos == sizeof(btBuf) - 1 )
        {
          btpos = 0;
          lcdDisplay.display("Over-run (BT)");
        }

        prevBtCh = ch;

        if (!settingsInitialised && grblSettings.initialised())
        {
            settingsInitialised= true;
            SerialBT.println("$#");
            SerialBT.flush();
        }

    }
    else if (Serial2.available()) {      // If anything comes in Serial from co-processor
        leds.setStatusLed( -1, -1, LEDs::MAX );

        char ch = Serial2.read();
        serBuf[serPos] = ch;
        serPos++;

        if (ch==0xA)
        {
            serBuf[serPos] = 0;
            ProcessSerialLine(serBuf);
            serPos = 0;
            ch=0;
        }

        // Over-run. Discard.
        // N.B. Allow extra char for terminator.
        if (serPos == sizeof(serBuf) - 1 )
        {
          serPos = 0;
          lcdDisplay.display("Over-run (MEGA)");
        }
    }
    else 
    {
      leds.setStatusLed( -1, 0, 0);

      // Nothing availabe on Bluetooth.
      vTaskDelay(1);

      long now = xTaskGetTickCount();

      int pollInterval = 50UL;
      int pollStateInterval = 20 * pollInterval;

      if (grblSettings.initialised())
      {
        if ( (grblState.parserState.get().compare("Idle") == 0) && 
             (now - timePollState) > pdMS_TO_TICKS( pollStateInterval) )
        {
            SerialBT.println("$G");
            SerialBT.println("$#");
            SerialBT.flush();
            timePollState = now;        
            timePollPosition = now;        
        }
        else if ( (now - timePollPosition) > pdMS_TO_TICKS( pollInterval))
          {
            SerialBT.write('?');
            SerialBT.flush();
            timePollPosition = now;        
          }
      }

    }
  
    if ( !SerialBT.hasClient() || 
        (xTaskGetTickCount() - lastBtMessageSeen) > pdMS_TO_TICKS( 10000))
    {
        reportStats();

        grblState.parserState.set("Disco!");
        leds.ledsOff();
        leds.setStatusLed( LEDs::MAX, 0, -1);

        Serial.printf("Bluetooth timeout. hasClient=%d\n",SerialBT.hasClient());

        if (SerialBT.hasClient())
          SerialBT.disconnect();
        
        Serial.println("Bluetooth begun.");

        do
        {
          Serial.println("Bluetooth timeout. Re-connect.");
        
          connected = SerialBT.connect(name);
        } while (!connected);

        Serial.println("Bluetooth re-connected.");

        SerialBT.println("$$");
        SerialBT.flush();
        lastBtMessageSeen = xTaskGetTickCount();
    }

    if (grblSettings.initialised() && jogController.poll())
    {
      std::string cmd = jogController.getCommand(); 
      SerialBT.println(cmd.c_str());
      //Serial.println(cmd.c_str());
      timePollState = xTaskGetTickCount();    // Holdoff state poll on jog command    
    }
  }

}

void loop() 
{
  StateNotifyStruct stateNotifyStruct;

  SPISettings spiSettings;

  Serial2.println("Hello me!");
  Serial2.flush();

  while (1)
  {
    if (xQueueReceive(xStateChangeNotifyQueue, (void *const)&stateNotifyStruct, 1))
    {
      stateNotifyStruct.fn( stateNotifyStruct.state, stateNotifyStruct.param);
    }

    delay(1);
  }
}