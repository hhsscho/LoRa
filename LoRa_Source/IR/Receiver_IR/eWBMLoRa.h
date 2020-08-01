#define mySerial Serial
#define LORA_MODEM_RESET 7

typedef const char* ConstStr;
#define GFP(x) x
#define GF(x)  x

#if !defined(LORA_RX_BUFFER)
#define LORA_RX_BUFFER 256
#endif

static const char LORA_NL[] = "\r\n";
static const char LORA_EVENT[] = "[EVENT]";
static const char LORA_OK[] = "OK";
static const char LORA_ERROR[] = "<Failed";
static const char LORA_CERROR[] = "<C";  //<Command Not Found>
static const char LORA_WAIT_RCV[] = "WAIT";
static const char LORA_JOINED[] = "JOINED";


typedef enum {
  ABP = 0,
  OTAA,
} _lora_mode;

typedef enum {
  APP_EUI = 0,
  APP_KEY,
  DEV_EUI,
  DEV_ADDR,
  NWKS_KEY,
  APPS_KEY,
  NWK_ID,
  ECHO,
  CFM,        //Confirm mode
} _lora_property;

typedef enum {
  CLASS_A = 'A',
  CLASS_B,
  CLASS_C,
} _lora_class;

class eWBMLoRaModem
{
private:
  bool network_joined = false;
  bool bRcvStart = false;
  String ATcmd;
  String ATRcvData;
  
  /* Utilities */
  /*Joined 이후 LoRa Modem에서 입력 들어오는 데이터 처리. Ex) [EVENT] ……*/
  char streamRead(){ 
    char c;
    c = mySerial.read();
    
    if(c=='\r' || c=='\n'){
      bRcvStart = false;
      return -1;
    }
    //else if(c=='<' || c=='[')
    else if(c=='['){
      ATRcvData="";
      bRcvStart = true;
    } 
    
    if(bRcvStart)
      return c;
    else
      return -2;
  }
  
  // 현재로써는 rcv data 짧은 길이만 가능
  // uart rx 데이터 확인하고 바로 지움
  int waitResponse(uint32_t timeout, ConstStr r1 = GFP(LORA_OK)){
    ATRcvData.reserve(LORA_RX_BUFFER);
    int index = -1;
    int length = 0;
    int DataEvt = 0;
    unsigned long startMillis = millis();
    char c;
  
    do{
      while (mySerial.available() > 0){
        c = mySerial.read();
        if(c > -1)
          ATRcvData +=  c;
          
        if (r1 && ATRcvData.endsWith(r1)){
          index = 1;
          goto finish;
        }
      }
    }while (millis() - startMillis < timeout);
    
  finish:
    ATRcvData = "";
    return index;
  }
  
  // Manually Send AT Cmd
  uint8_t atsend(String ATCmd, uint32_t timeout, ConstStr res){
    int ret=0; 
    
    for (int i = 0; i<ATCmd.length(); i++){
      delay(20);
      char c = ATCmd.charAt(i);
      mySerial.write(c);
      mySerial.flush();
    }
    ret = waitResponse(timeout, res);
    return ret;
  }
  
  template<typename T>
  void streamWrite(T last) {
    String strlast = last;
    for (int i = 0; i<strlast.length(); i++){      
      mySerial.write(strlast.charAt(i));
      mySerial.flush();
      delay(20);
    }
  }
  
  template<typename T, typename... Args>
  void streamWrite(T head, Args... tail) {
    String str(head);
    streamWrite(str);
    streamWrite(tail...);
  }
  
  template<typename... Args>
  void sendAT(Args... cmd) {
    streamWrite("AT", cmd..., LORA_NL);
    mySerial.flush();
  }
  
  bool Set_AT_NJM(_lora_mode mode){
    sendAT(GF("+NJM="), mode);
    if (waitResponse(1000, LORA_OK) != 1){
      return false;
    }
    return true;
  }
  
  size_t Get_AT_NJS(){
    sendAT(GF("+NJS"));
    if (waitResponse(2000, LORA_OK) != 1){
      return 0;
    }
    return 1;
  }
  
  bool Set_AT_JOIN(){
    sendAT(GF("+JOIN"));
    if (waitResponse(60000L, "JOINED") != 1){
      return false;
    }
    return true;
  }

public:
  boolean ResetHW(){
    unsigned long timeout = 10000L;
    Serial.println("ResetHW");
  
    pinMode(LORA_MODEM_RESET, OUTPUT);
    digitalWrite(LORA_MODEM_RESET, LOW);
    delay(1000);
    pinMode(LORA_MODEM_RESET, INPUT);
    ATRcvData = "";
    delay(500);
  
    for (unsigned long start = millis(); millis() - start < timeout;){
      sendAT(GF(""));
      if (waitResponse(200, LORA_OK) == 1){
        delay(100);
        return true;
      }
      delay(100);
    }
    return false;  
  }
  
  /* 아두이노에서 직접 AT Command 실행을 위한 함수 */
  bool Set_AT(_lora_property prop, String value){
    switch (prop){
    case APP_EUI:
      sendAT(GF("+APPEUI="), value);
      break;
    case APP_KEY:
      sendAT(GF("+APPKEY="), value);
      break;
    case DEV_EUI:
      sendAT(GF("+DEVEUI="), value);
      break;
    case DEV_ADDR:
      sendAT(GF("+DADDR="), value);
      break;
    case NWKS_KEY:
      sendAT(GF("+NWKSKEY="), value);
      break;
    case NWK_ID:
      sendAT(GF("+IDNWK="), value);
      break;
    case APPS_KEY:
      sendAT(GF("+APPSKEY="), value);
      break;
    case ECHO:
      sendAT(GF("+ECHO="), value);
      break;
    case CFM:
      sendAT(GF("+CFM="), value);
      break;    
    default:
      return false;
    }
    if (waitResponse(1000, LORA_OK) != 1){
      return false;
    }
    return true;
  }

  /* ABP Mode Join 실행 */
  void ABPJoin(String devAddr, String nwkSKey, String appSKey){
    ATRcvData.reserve(LORA_RX_BUFFER);
    Set_AT_NJM(ABP);
    Set_AT(DEV_ADDR, devAddr);
    Set_AT(NWKS_KEY, nwkSKey);
    Set_AT(APPS_KEY, appSKey);
    network_joined = Set_AT_JOIN();
  }

  /* OTAA Mode Join 실행. DevEUI를 변경할 때 사용 */
  bool OTAAJoin(String appEui, String appKey, String devEui){
    ATRcvData.reserve(LORA_RX_BUFFER);
    Set_AT(ECHO, "1");
    Set_AT_NJM(OTAA);
    Set_AT(APP_EUI, appEui);
    Set_AT(APP_KEY, appKey);
    Set_AT(DEV_EUI, devEui);

    network_joined = Set_AT_JOIN();
      
    return network_joined;
  }
  
  /* OTAA Mode Join 실행. DevEUI를 변경없이 사용 */
  bool OTAAJoin(String appEui, String appKey){
    ATRcvData.reserve(LORA_RX_BUFFER);
    Set_AT(ECHO, "1");
    Set_AT_NJM(OTAA);   
    Set_AT(APP_EUI, appEui);
    Set_AT(APP_KEY, appKey);
    network_joined = Set_AT_JOIN();
      
    return network_joined;
  }

  /* Join 이후 Application Data 전송 */
  uint8_t SendData(uint32_t port, String data, bool isBin, bool confirm_mode){
    String strSend;
    if(confirm_mode)
      Set_AT(CFM, "1");
    else
      Set_AT(CFM, "0");
  
    if (isBin)
      strSend = "AT+SENDB=";
    else
      strSend = "AT+SEND=";
  
    strSend += String(port,10);
    strSend += ":";
    strSend += data;
    strSend += LORA_NL;
      
    for (int i = 0; i<strSend.length(); i++){
      delay(20);
      char c = strSend.charAt(i);
      mySerial.write(c);
      mySerial.flush();
    }
    ATRcvData="";
    delay(1000); //RX1 Delay
  }

  /* 모뎀단에서 전송한 데이터에서 Application 데이터를 아두이노에서 받아감 */
  bool isRXResponse(String& strRecvData){
    char c;
    strRecvData="";
   
    while (mySerial.available() > 0){
        c = streamRead();
        if(c > -1)
          ATRcvData +=  c;
        else if(c == -1){
          strRecvData = ATRcvData;
          ATRcvData="";
        }
    }
    if(strRecvData.length()>5)
      return true;
    else
      return false;
  }
};
