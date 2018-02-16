
/*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*/

#define MY_RADIO_NRF24
#define CHILD_ID_HVAC 0

#include <MySensors.h>

// Uncomment your heatpump model
//#include <FujitsuHeatpumpIR.h>
//#include <PanasonicCKPHeatpumpIR.h>
#include <PanasonicHeatpumpIR.h>
//#include <CarrierHeatpumpIR.h>
//#include <MideaHeatpumpIR.h>
//#include <MitsubishiHeatpumpIR.h>
//#include <SamsungHeatpumpIR.h>
//#include <SharpHeatpumpIR.h>
//#include <DaikinHeatpumpIR.h>

unsigned long previousMillis = 0;

//Some global variables to hold the states
int POWER_STATE = 0;
int TEMP_STATE = 0;
int FAN_STATE = 0;
int MODE_STATE = 0;
int VDIR_STATE = VDIR_AUTO;
int HDIR_STATE = HDIR_AUTO;

int CURRENT_POWER_STATE = 0;
int CURRENT_TEMP_STATE = 0;
int CURRENT_FAN_STATE = 0;
int CURRENT_MODE_STATE = 0;

IRSenderPWM irSender(3);       // IR led on Arduino digital pin 3, using Arduino PWM

//Change to your Heatpump
HeatpumpIR *heatpumpIR = new PanasonicJKEHeatpumpIR();

/*
new PanasonicDKEHeatpumpIR()
new PanasonicJKEHeatpumpIR()
new PanasonicNKEHeatpumpIR()
new CarrierHeatpumpIR()
new MideaHeatpumpIR()
new FujitsuHeatpumpIR()
new MitsubishiFDHeatpumpIR()
new MitsubishiFEHeatpumpIR()
new SamsungHeatpumpIR()
new SharpHeatpumpIR()
new DaikinHeatpumpIR()
*/

MyMessage msgHVACSetPointC(CHILD_ID_HVAC, V_HVAC_SETPOINT_COOL);
MyMessage msgHVACSpeed(CHILD_ID_HVAC, V_HVAC_SPEED);
MyMessage msgHVACFlowState(CHILD_ID_HVAC, V_HVAC_FLOW_STATE);

void presentation() {
  sendSketchInfo("Heatpump", "2.1");
  present(CHILD_ID_HVAC, S_HVAC, "Thermostat");
}

void setup() {
  
}

void loop() {
  //Do not launch IR commands to fast
  unsigned long currentMillis = millis();
  previousMillis = min(currentMillis, previousMillis); 
  unsigned long duration = currentMillis-previousMillis;
  if ((duration<60000 && previousMillis==0) || (duration<2000 && previousMillis>0))
    return;
  previousMillis=currentMillis;
 
  //Initialize STATES to valid values 
  if (POWER_STATE!=POWER_ON && POWER_STATE!=POWER_OFF)
    POWER_STATE=POWER_ON;
  //if (MODE_STATE!=MODE_MAINT && MODE_STATE!=MODE_COOL && MODE_STATE!=MODE_HEAT && MODE_STATE!=MODE_AUTO && MODE_STATE!=MODE_FAN) {
  if (MODE_STATE!=MODE_MAINT && MODE_STATE!=MODE_HEAT) {
    MODE_STATE=MODE_AUTO;
    POWER_STATE=POWER_ON;
  }
  if (FAN_STATE!=FAN_AUTO && FAN_STATE!=FAN_1 && FAN_STATE!=FAN_2 && FAN_STATE!=FAN_3 && FAN_STATE!=FAN_4 && FAN_STATE!=FAN_5 && FAN_STATE!=FAN_QUIET && FAN_STATE!=FAN_POWERFUL)
    FAN_STATE=FAN_AUTO;
  if (TEMP_STATE!=8 && TEMP_STATE!=10 && (TEMP_STATE<16 || TEMP_STATE>30))
    TEMP_STATE=21;  

    
   if (CURRENT_FAN_STATE!=FAN_STATE || CURRENT_TEMP_STATE!=TEMP_STATE || CURRENT_POWER_STATE!=POWER_STATE || CURRENT_MODE_STATE!=MODE_STATE) {
    CURRENT_FAN_STATE=FAN_STATE;
    CURRENT_TEMP_STATE=TEMP_STATE;
    CURRENT_POWER_STATE=POWER_STATE;
    CURRENT_MODE_STATE=MODE_STATE;

    sendHeatpumpCommand();
    delayMicroseconds(1000);
    sendNewStateToGateway();    
   }
}

void receive(const MyMessage &message) {
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
     return;
  }

  Serial.print("Incoming message for: ");
  Serial.print(message.sensor);

  String recvData = message.data;
  recvData.trim();

  Serial.print(", New status: ");
  Serial.println(recvData);
  switch (message.type) {
    case V_HVAC_SPEED:
      Serial.println("V_HVAC_SPEED");

      if(recvData.equalsIgnoreCase("auto")) FAN_STATE = FAN_AUTO;
      else if(recvData.equalsIgnoreCase("min")) FAN_STATE = FAN_1;
      else if(recvData.equalsIgnoreCase("normalmin")) FAN_STATE = FAN_2;
      else if(recvData.equalsIgnoreCase("normal")) FAN_STATE = FAN_3;
      else if(recvData.equalsIgnoreCase("normalmax")) FAN_STATE = FAN_4;
      else if(recvData.equalsIgnoreCase("max")) FAN_STATE = FAN_5;
      else if(recvData.equalsIgnoreCase("quiet")) FAN_STATE = FAN_QUIET;
      else if(recvData.equalsIgnoreCase("powerful")) FAN_STATE = FAN_POWERFUL;
    break;

    case V_HVAC_SETPOINT_COOL:
      Serial.println("V_HVAC_SETPOINT_COOL");
      TEMP_STATE = message.getFloat();
    break;

    case V_HVAC_FLOW_STATE:
      Serial.println("V_HVAC_FLOW_STATE");
      if (recvData.equalsIgnoreCase("mainton")) {
        POWER_STATE = POWER_ON;
        MODE_STATE = MODE_MAINT;
      }
      if (recvData.equalsIgnoreCase("fanon")) {
        POWER_STATE = POWER_ON;
        MODE_STATE = MODE_FAN;
      }
      else if (recvData.equalsIgnoreCase("coolon")) {
        POWER_STATE = POWER_ON;
        MODE_STATE = MODE_COOL;
      }
      else if (recvData.equalsIgnoreCase("heaton")) {
        POWER_STATE = POWER_ON;
        MODE_STATE = MODE_HEAT;
      }
      else if (recvData.equalsIgnoreCase("autochangeover")) {
        POWER_STATE = POWER_ON;
        MODE_STATE = MODE_AUTO;
      }
      else if (recvData.equalsIgnoreCase("off")) {
        POWER_STATE = POWER_OFF;
      }
      break;
  }
}

void sendNewStateToGateway() {
  send(msgHVACSetPointC.set(TEMP_STATE));
  send(msgHVACSpeed.set(FAN_STATE));
  send(msgHVACFlowState.set(MODE_STATE));
}

void sendHeatpumpCommand() {
  Serial.println("Power = " + (String)POWER_STATE);
  Serial.println("Mode = " + (String)MODE_STATE);
  Serial.println("Fan = " + (String)FAN_STATE);
  Serial.println("Temp = " + (String)TEMP_STATE);

  //heatpumpIR->send(irSender, POWER_ON, MODE_FAN, FAN_5, TEMP_STATE, VDIR_UP, HDIR_AUTO);
  heatpumpIR->send(irSender, POWER_STATE, MODE_STATE, FAN_STATE, TEMP_STATE, VDIR_AUTO, HDIR_AUTO);
}

