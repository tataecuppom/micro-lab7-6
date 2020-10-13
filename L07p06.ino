#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <TaskScheduler.h>
#include "xbWeb.h"
#include "xbUCI.h"
#include "xbRTCC.h"
#include <Wire.h>

#define BH1750address  0x23
#define RTCC_ALARM_PIN 16
#define LED_MODE 2
xbUCI uci("check_get_set_run_stop_");
xbRTCC rtcc;
byte mode;
byte ctrlstate;
bool bRun;
String vLight;
int vvLight;
//###############################################
void service(void);
void serveAlarm(void);
void readBH1750(void);

xbWeb myw("@JumboPlusIoT","ilovecmu","www.minionsworker.com", "2cf43217a89a", "youwillneverwalkalone");
// IP :  2C:F4:32:17:A8:9A
void autoConnect(void);
Task t1(5000, TASK_FOREVER, &autoConnect);
Scheduler runner;
int myState;
int user_request;
int dummy_data;
//###############################################
void service(void);
void GetUserRequest(void);

void setup()
{
    delay(3000);
    // H/W initialize ---------------------------
    Serial.begin(115200);
    // Variables and state initialize -----------
    Serial.print("\n\r\n\r# wifi begin...");
    myw.reconnect();
    runner.init();
    runner.addTask(t1);
    delay(500);
    t1.enable();
    myState = 0;
    user_request = 0;
    dummy_data = 0;
    vvLight =0 ;

    Wire.begin();
    pinMode(RTCC_ALARM_PIN, INPUT);
    Wire.beginTransmission(BH1750address);
    Wire.write(0x10);
    Wire.endTransmission();
    // Variables and state initialize -----------
    uci.initialize();
    bRun = 0;
    rtcc.Check();
    // End of setup -----------------------------

}

void loop()
{
   
    runner.execute();
    
    //if (Serial.available())
       // uci.run((char)Serial.read());
    
    if (Serial.available()) GetUserRequest();
}

void autoConnect(void) {
    int res;
    Serial.print("* State -> ");
    Serial.println(myState);
    switch (myState)
    {
        case 0:
            if (myw.status() == 3) { //connect to wifi
                Serial.println("WiFi connected, move to State-1");
                myState = 1;
            }
            break;
        case 1:
            Serial.println("? Test connection to Web server");
            res = myw.web_connect("cmd=connect");
            if (res != 0) {
                Serial.println("! connection failed");
            }
            else {
                if (myw.response == "#OK") {
                    Serial.println("  Web connection OK, move to State-2");
                    myState = 2;
                    user_request = 0;
                }
                else {
                    Serial.println("! Invalid respone from Web server");
                }
            }
            break;
        case 2:
            if (user_request > 0) {
                Serial.print("# service for user request of ");
                Serial.println(user_request);
                service();
                user_request = 0;
            }
            else {
                Serial.println("# none");
            }
            break;
    }
}

void GetUserRequest(void) {
    char tm ;
    while(Serial.available()) {
        tm = Serial.read();
    }
    if (myState != 2) return;

    if (tm < 0x31) return;
    if (tm > 0x33) return;
    user_request = tm - 0x30;
    Serial.print("\n\r# Machine accepts requst of ");
    Serial.println(user_request);
}



void service(void) {
    String dd;
    int res;
    if (user_request == 1) { //data count
        Serial.println("request No. of data");
        res = myw.web_connect("cmd=data_count");
    }
    else if (user_request == 2) { // insert data
        readBH1750();
        rtcc.GetTime();
        rtcc.TimeToString();
        Serial.println("request to insert data");
        //dummy data and time stamp of '2020-03-17 10:11:00'
        
        dd = "cmd=data_insert&ndata=" + String(vvLight)  ;
        dd.concat("&ntstamp=" + String(rtcc.sTime));
        //dd.concat("&ndata=" + String(vLight));
        res = myw.web_connect(dd);
        //dummy_data++;
    }
    else if (user_request == 3) { // data clear
        Serial.println("request to clear all data");
        res = myw.web_connect("cmd=data_clear");
    }
    
    if (res == 0) {
        Serial.println(myw.response);
    }    
    else {
        Serial.println("! Invalid response from server");
    }
}


void readBH1750(void) {
    unsigned long ro;
    byte  buff[2];
    char rotext[7];
    Wire.beginTransmission(BH1750address);
    Wire.requestFrom(BH1750address, 2);

    buff[0] = Wire.read();
    buff[1] = Wire.read();
    Wire.endTransmission();
    ro = buff[0]*256 + buff[1];
    ro = ro*6;
    ro = ro/5;
    vvLight = ro;
    Serial.println(ro);
    sprintf(rotext, "%6d", ro);
    vLight = String(rotext);
}



