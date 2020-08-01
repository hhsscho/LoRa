#include "eWBMLoRa.h"

eWBMLoRaModem loramodem;

#define BAUDRATE 38400
#define TEST_HEX_CODE
#define LED_RX 13
#define LORA_SEND_INTERVAL 60000  // limit 10s
#define DATA_PORT 5
#define CTRL_PORT 222
#define RCV_SLICE 16

int lora_joined = 0;
unsigned long startSendMillis=0;

String devEui = "0000000000000000";
String appEui = "0000000000000000";
String appKey = "00000000000000000000000000000000";

bool DataSend(){
  int ret=0;
  unsigned long calledMillis = millis();
  if (!lora_joined)
    return false;
  if(calledMillis - startSendMillis < LORA_SEND_INTERVAL)
    return false;
  startSendMillis = millis();
#ifdef TEST_HEX_CODE
  loramodem.SendData(DATA_PORT,"4a4f494e5f54455354", true, false);
#else
  loramodem.SendData(DATA_PORT,"JOIN_TEST", false, false);
#endif
  return true;
}

void DataRcv(String strRecved){
  if(!lora_joined){
    if(strRecved.endsWith("JOINED")){
      lora_joined = 1;
      delay(1000);
    }
  }
  String rcv_data = strRecved.substring(RCV_SLICE);
  /* Process your Received Data */
  Serial.println(rcv_data);
  if(rcv_data == "30")
    digitalWrite(8, LOW);
  if(rcv_data == "31")
    digitalWrite(8, HIGH);
  delay(1000);
}

void setup(){
  uint8_t ret = 0;
  pinMode(LED_RX, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.begin(BAUDRATE);
  while (!Serial){;}

  while(!loramodem.ResetHW()){}
  
  ret = loramodem.OTAAJoin(appEui, appKey, devEui);
  if(ret == 1){
    lora_joined = 1;
    delay(1000);
  }
  Serial.println("AT+CLASS C");
  loramodem.SendData(DATA_PORT,"JOIN_TEST", false, false);
}

void loop(){
  String rcv = "";
  rcv.reserve(LORA_RX_BUFFER);

  if(loramodem.isRXResponse(rcv))
    DataRcv(rcv);
}
