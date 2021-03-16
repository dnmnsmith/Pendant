#include <Arduino.h>
#include <Wire.h>

#include "Util.h"
#include "BluetoothSerial.h"

#include "esp_log.h"


extern BluetoothSerial SerialBT;


void applyWs();
void applySpindle();

static const char * alarmTable[] = 
{
   //01234567890123456789
    "Invalid alarm code",   // 0
 	"Hard limit triggered", // 1
 	"Soft limit triggered", // 2
 	"Reset during motion",  // 3
    "Probe fail",
    "Probe fail",
    "Homing fail",
    "Homing fail",
    "Homing fail",
    "Homing fail"
};

std::vector<std::string> split(std::string str, std::string token){
    std::vector<std::string>result;
    while(str.size()){
        int index = str.find(token);
        if(index!=std::string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}

void reportStats()
{
    printf("RAM left %d\n", esp_get_free_heap_size());
    printf("Task stack: %d\n", uxTaskGetStackHighWaterMark(NULL));
}


const char *getAlarmString( int alarmId )
{
    if (alarmId <= 0 || alarmId > 9)
    {
        return alarmTable[ 0 ];
    }
    else
    {
        return alarmTable[ alarmId ];
    }
    
}

void safeMoveMpos( float x, float y, float z)
{
    SerialBT.printf("G53G90G0Z0\n");                // Set safe Z
    SerialBT.printf("G53G90G0X%.3fY%.3f\n", x, y);  // Move to toolchange machine pos.
    SerialBT.printf("G53G90G0Z%.3f\n",z);           // Drop Z to required.
}