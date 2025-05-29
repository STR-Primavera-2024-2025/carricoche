#include <mbed.h>
#include <rtos.h>
#include <RPC.h>
#include <SPI.h>
#include <WiFi.h>
#include <CAN.h>

using namespace mbed;
using namespace rtos;
// using namespace std::chrono_literals;

Thread redLedThread(osPriorityHigh7);//, DEFAULT_STACK_SIZE
void redLedThreadCode(void);

Thread canRxThread(osPriorityHigh7);//, DEFAULT_STACK_SIZE
void canRxThreadCode(void);

Thread canTxThread(osPriorityHigh7);//, DEFAULT_STACK_SIZE
void canTxThreadCode(void);

Thread can2WiFiThread(osPriorityHigh7);//, DEFAULT_STACK_SIZE
void can2WiFiThreadCode(void);

// Thread wiFi2CanThread(osPriorityHigh7);//, DEFAULT_STACK_SIZE
// void wiFi2CanThreadCode(void);

char ssid[] = "can2WiFi_AP";        // your network SSID (name)
char pass[] = "can2WiFi_AP";        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
IPAddress ip(192, 168, 1, 102);
unsigned int localPort = 8888;      // local port to listen on
IPAddress remoteIP(192, 168, 1, 101);
unsigned int remotePort = 8888;      // remote port to write to
unsigned int remoteScopePort = 9999;
char packetBuffer[256]; //buffer to hold incoming packet
char  ReplyBuffer[] = "acknowledged";       // a string to send back
WiFiServer server(80);
WiFiUDP Udp;  

mbed::CAN can(PB_5, PB_13);
mbed::CANMessage msgRx;

void printWiFiStatus(void);

void setup() 
{
  Serial.begin(115200);

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  
  can.frequency(500000);

  // if (!CAN.begin(CanBitRate::BR_500k))
  // {
  //   Serial.println("CAN.begin(...) failed.");
  //   // for (;;) {}
  // }
  // Serial.println("CAN.begin(...) OK!");

   // check for the WiFi module:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // // attempt to connect to Wifi network:
  // while (status != WL_CONNECTED) 
  // {
  //   Serial.print("Attempting to connect to SSID: ");
  //   Serial.println(ssid);
  //   // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  //   status = WiFi.begin(ssid, pass);

  //   // wait 3 seconds for connection:
  //   delay(1000);
  // }
  // Serial.println("Connected to wifi");
  // printWifiStatus();

  // by default the local IP address will be 192.168.4.1
  // you can override it with the following:
  WiFi.config(IPAddress(192, 168, 0, 101));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // // Create open network. Change this line if you want to create an WEP network:
  // status = WiFi.beginAP(ssid, pass);
  // if (status != WL_AP_LISTENING) {
  //   Serial.println("Creating access point failed");
  //   // don't continue
  //   while (true)
  //     ;
  // }
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 3 seconds for connection:
    delay(3000);
  }

  // wait 1 seconds for connection:
  delay(1000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

  Udp.begin(localPort);

  redLedThread.start(redLedThreadCode);
  canRxThread.start(canRxThreadCode);
  canTxThread.start(canTxThreadCode);
  can2WiFiThread.start(can2WiFiThreadCode);
}

void redLedThreadCode(void) 
{
  const unsigned int TRedLedThread=500; //periodicity of the red led thread in ms
  uint64_t nextWakeTime = TRedLedThread;

  for (;;) 
  {  // Repeat forever
    digitalWrite(LEDR,digitalRead(LEDR)^1);

    nextWakeTime=TRedLedThread-(get_ms_count()%TRedLedThread);
    ThisThread::sleep_for(nextWakeTime);
  }
}

//TODO: move to external pin interrupt driven thread
void canRxThreadCode(void) 
{
  const unsigned int TcanRxThread=50; //periodicity of the red led thread in ms
  uint64_t nextWakeTime = TcanRxThread;

  for (;;) 
  {  // Repeat forever    
    if (can.read(msgRx))
    {      
      Serial.print("CAN Rx (thread) Id=");
      Serial.print(msgRx.id);
      Serial.print("--> ");
      for(byte i = 0; i<msgRx.len; i++)
      {
          Serial.print(msgRx.data[i]); 
          Serial.print(" ");
      }
      Serial.println();   
    }     

    nextWakeTime=TcanRxThread-(get_ms_count()%TcanRxThread);
    ThisThread::sleep_for(nextWakeTime);
  }
}

void canTxThreadCode(void) 
{
  const unsigned int TcanTxThread=1000; //periodicity of the red led thread in ms
  uint64_t nextWakeTime = TcanTxThread;
  
  long unsigned int txId=66;
  unsigned char len = 8;
  unsigned char txBuf[8]={66,66,66,66,66,66,66,66};
  // mbed::CANMessage(txId, txBuf, len) msgTx;
  mbed::CANMessage msgTx;

  for (;;) 
  {  // Repeat forever    
    msgTx.id=66;
    msgTx.len=8;
    msgTx.data[0]=66;
    msgTx.data[1]=66;
    msgTx.data[2]=66;
    msgTx.data[3]=66;
    msgTx.data[4]=66;
    msgTx.data[5]=66;
    msgTx.data[6]=66;
    msgTx.data[7]=msgTx.data[7]+1;
    can.write(msgTx);
    Serial.print("CAN Tx (thread)");
    Serial.print(" Id=");
    Serial.print(msgTx.id);
    Serial.print("--> ");
    for(byte i = 0; i<msgTx.len; i++)
      {
          Serial.print(msgTx.data[i]); 
          Serial.print(" ");
      }        
      Serial.println();

    nextWakeTime=TcanTxThread-(get_ms_count()%TcanTxThread);
    ThisThread::sleep_for(nextWakeTime);
  }
}


  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8]={0,0,0,0,0,0,0,0};

void can2WiFiThreadCode(void) 
{
  const unsigned int Tcan2WiFiThread=10; //periodicity of the red led thread in ms
  uint64_t nextWakeTime = Tcan2WiFiThread;

  // long unsigned int rxId;
  // unsigned char len = 0;
  // unsigned char rxBuf[8]={0,0,0,0,0,0,0,0};

  for (;;) 
  {  // Repeat forever    
    // if(!digitalRead(CAN_INT_PIN)) // If CAN_INT pin is low, read receive buffer  
    // {       
    //   CAN.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    //   Serial.print("CAN Rx (loop) "); 
    //   Serial.print(" Id=");
    //   Serial.print(rxId);
    //   Serial.print("--> ");      
    //   for(byte i = 0; i<len; i++)
    //   {
    //       Serial.print(rxBuf[i]); 
    //       Serial.print(" ");
    //   }        
    //   Serial.println();
    // }

    Udp.beginPacket(remoteIP, remoteScopePort);
    Udp.println("can2WiFi");
    Udp.print(" Id=");
    Udp.print(rxId);
    Udp.print("--> ");      
    for(byte i = 0; i<len; i++)
    {
        Udp.print(rxBuf[i]); 
        Udp.print(" ");
    }        
    Udp.println();
    Udp.endPacket();
    Udp.flush();

    nextWakeTime=Tcan2WiFiThread-(get_ms_count()%Tcan2WiFiThread);
    ThisThread::sleep_for(nextWakeTime);
  }
}

void loop() 
{
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }

  WiFiClient client = server.available();  // listen for incoming clients

  if (client) {                    // if you get a client,
    Serial.println("new client");  // print a message out the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected()) {   // loop while the client's connected
      delayMicroseconds(10);       // This is required for the Arduino Nano RP2040 Connect - otherwise it will loop so fast that SPI will never be served.
      if (client.available()) {    // if there's bytes to read from the client,
        char c = client.read();    // read a byte, then
        Serial.write(c);           // print it out the serial monitor
        if (c == '\n') {           // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

client.println("<!DOCTYPE html>");
client.println("<html>");
client.println(" <head>");
client.println(" <meta name='viewport' content='width=device-width, initial-scale=1.0'/>");
client.println(" <meta charset='utf-8'>");
client.println(" <meta http-equiv='refresh' content='1'>");
client.println(" <style>");
client.println("   body {font-size:100%;} ");
client.println("   #main {display: table; margin: auto;  padding: 0 10px 0 10px; } ");
client.println("   h2 {text-align:center; } ");
client.println("   p { text-align:center; }");
client.println(" </style>");
client.println("   <title>Auto Update Example Using HTML</title>");
client.println(" </head>");

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/HR\">here</a> turn the RED LED on<br>");
            client.print("Click <a href=\"/LR\">here</a> turn the RED LED off<br>");
            client.print("Click <a href=\"/HG\">here</a> turn the GREEN LED ON<br>");
            client.print("Click <a href=\"/LG\">here</a> turn the GREEN LED off<br>");
            client.print("Click <a href=\"/BH\">here</a> turn the BLUE LED on<br>");
            client.print("Click <a href=\"/BL\">here</a> turn the BLUE LED off<br>");

            // String txt="CAN Rx (loop)  Id=" + String(rxId) + rxBuf[0] + " " + rxBuf[1] + " " + rxBuf[2] + " " + rxBuf[3] + " " + rxBuf[4] + " " + rxBuf[5] + " " + rxBuf[6] + " " + rxBuf[7];
            client.print(" Id=");
            client.print(msgRx.id);
            client.print("--> ");
            for(byte i = 0; i<msgRx.len; i++)
            {
                client.print(msgRx.data[i]); 
                client.print(" ");
            }        
            client.println();
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request (turns ON/OFF the different LEDs)
        if (currentLine.endsWith("GET /HR")) {
          digitalWrite(LED_RED, LOW);  
        }
        if (currentLine.endsWith("GET /LR")) {
          digitalWrite(LED_RED, HIGH); 
        }
        if (currentLine.endsWith("GET /HG")) {
          digitalWrite(LED_GREEN, LOW); 
        }
        if (currentLine.endsWith("GET /LG")) {
          digitalWrite(LED_GREEN, HIGH);  
        }
        if (currentLine.endsWith("GET /BH")) {
          digitalWrite(LED_BLUE, LOW);  
        }
        if (currentLine.endsWith("GET /BL")) {
          digitalWrite(LED_BLUE, HIGH);  
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void printWiFiStatus(void) 
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
