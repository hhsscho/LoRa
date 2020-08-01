#include<SoftwareSerial.h>
int sensorData = NULL;
#define sensorPin 12

SoftwareSerial LoRaSerial(2,3); //RX, TX 
int count = 0;
int toggle = 0;
void setup(){
  pinMode(sensorPin, INPUT);
  LoRaSerial.begin(38400);
  Serial.begin(38400);
  sendATcommand("AT+DEVEUI 0000000000000000", 2000);
  sendATcommand("AT+APPKEY 00000000000000000000000000000000", 2000);
  sendATcommand("AT+CLASS A", 2000);
  sendATcommand("AT+CFM 0", 2000);
  sendATcommand("AT+JOIN", 30000);
  
}

void loop(){
  sensorData = digitalRead(sensorPin);
  
  if (sensorData == 1 && toggle == 0){
    sendATcommand("AT+SEND 2:1", 500);
    toggle = 1;
  }
  if (sensorData == 0 && toggle == 1){
    sendATcommand("AT+SEND 2:0", 500);
    toggle = 0;
  }
}

String sendATcommand(const char *toSend, unsigned long milliseconds) {
  String result;
  Serial.println(toSend);
  LoRaSerial.println(toSend);
 
  unsigned long startTime = millis();
  while (millis() - startTime < milliseconds) {
    if (LoRaSerial.available()) {
      char c = LoRaSerial.read();
      Serial.write(c);
      result += c;  // append to the result string
    }
  }
Serial.println();  // new line after timeout.
return result;
}
