#include <SoftwareSerial.h>
#include "DHT.h"
#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

SoftwareSerial LoRaSerial(2,3); //RX, TX 

int toggle = 0;

void setup(){

  LoRaSerial.begin(38400);
  Serial.begin(38400);
  sendATcommand("AT+DEVEUI 0000000000000000", 2000);
  sendATcommand("AT+APPKEY 00000000000000000000000000000000", 2000);
  sendATcommand("AT+CLASS A", 2000);
  sendATcommand("AT+CFM 0", 2000);
  sendATcommand("AT+JOIN", 20000);
  
}

void loop(){
  delay(1000);
  int hum = dht.readHumidity();
  int temp = dht.readTemperature();

  if (toggle == 0){
    if (hum >= 60 && temp >= 28){
      sendATcommand("AT+SEND 2:1", 3000);
      toggle = 1;
    }  
  }

  if (toggle == 1){
    if (hum < 45 && temp < 30){
      sendATcommand("AT+SEND 2:0", 3000);
      toggle = 0;
    }   
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
