#include <Arduino.h>
#include "mbed.h"
#include "rtos.h"
#include "WiFi.h" 
#include "MotorDriver.h"

using namespace mbed;
using namespace rtos;
using namespace std::chrono;

MotorDriver m;

//WiFi for udp, ur3 comm, telemetry, http, etc.
char ssid[] = "Grup4";   //Access point
char pass[] = "Grup4Pass";   //Access point
// char ssid[] = "wlan_str"; // your WPA network SSID (name)
// char pass[] = "wlan_str"; // your WPA network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
// IPAddress ip(192, 168, 1, 111); //static ip is not working!
IPAddress ipRemote=IPAddress(192, 168, 1, 111);//fixed ip not working...!!!
IPAddress ipLocal=IPAddress(192, 168, 1, 112);//fixed ip not working...!!!
WiFiServer server(80);
WiFiUDP Udp;  
WiFiClient client;
String ur3Command="";
IPAddress ur3(192,168,1,100);//UR3 IP address (left robot)
const unsigned int ur3PORT=30003;
const unsigned int gripperPORT=41414;


//Global Variables
int dir = 0;
char udpBufferRead[256];

//Threads
Thread threadMotors(osPriorityHigh), threadReadSerial(osPriorityHigh), threadScreen(osPriorityHigh);
Thread threadSendUdp(osPriorityHigh);
Thread threadReadUdp(osPriorityHigh);

//Functions
void moveMotors();
void TaskReadUdp();
void TaskSendUdp();
void wifiInit();

void setup() {
  Serial.begin(115200);
  wifiInit(); // wifiInitWPA();
  Udp.begin(8888);
  threadMotors.start(moveMotors);
  threadReadUdp.start(TaskReadUdp);
  /*
  m.motor(1,FORWARD,255);
  m.motor(4,FORWARD,255);  
  delay(1000);
  m.motor(4,FORWARD,55);  
  delay(1000);
  m.motor(4,FORWARD,255);  
  delay(1000);
  m.motor(4,FORWARD,0);  
  delay(1000);
  m.motor(4,FORWARD,255);
  */
}

void loop() {}
/*
Rodes
4 - front right
3 - front left
2 - back right
1 - back left
*/

void moveMotors() {
  for (;;) {     
    switch(dir){
      case 1: //FORWARD
        m.motor(1,FORWARD,150);
        m.motor(2,FORWARD,150);  
        m.motor(3,FORWARD,150);  
        m.motor(4,FORWARD,150);

        break;
      case 2: //FORWARD-RIGHT
        m.motor(1,FORWARD,150);
        m.motor(2,BACKWARD,150);  
        m.motor(3,FORWARD,150);  
        m.motor(4,FORWARD,150);
        break;
      case 3: //RIGHT-ROTATION
        m.motor(1,FORWARD,150);
        m.motor(2,BACKWARD,150);  
        m.motor(3,FORWARD,150);  
        m.motor(4,BACKWARD,150);
        break;
      case 4: //BACKWARD-RIGHT
        m.motor(1,BACKWARD,150);
        m.motor(2,FORWARD,150);  
        m.motor(3,BACKWARD,150);  
        m.motor(4,BACKWARD,150);
        break;
      case 5: //BACKWARD
        m.motor(1,BACKWARD,150);
        m.motor(2,BACKWARD,150);  
        m.motor(3,BACKWARD,150);  
        m.motor(4,BACKWARD,150);
        break;
      case 6: //BACKWARD-LEFT
        m.motor(1,FORWARD,150);
        m.motor(2,BACKWARD,150);  
        m.motor(3,BACKWARD,150);  
        m.motor(4,BACKWARD,150);
        break;
      case 7: //LEFT-ROTATION
        m.motor(1,BACKWARD,150);
        m.motor(2,FORWARD,150);  
        m.motor(3,BACKWARD,150);  
        m.motor(4,FORWARD,150);
        break;
      case 8: //FORWARD-RIGHT
        m.motor(1,BACKWARD,150);
        m.motor(2,FORWARD,150);  
        m.motor(3,FORWARD,150);  
        m.motor(4,FORWARD,150);
        break;
      case 9: //FULL-LEFT
        m.motor(1,FORWARD,150);
        m.motor(2,BACKWARD,150);  
        m.motor(3,BACKWARD,150);  
        m.motor(4,FORWARD,150);
        break;
      case 10: //FULL-RIGHT
        m.motor(1,BACKWARD,150);
        m.motor(2,FORWARD,150);  
        m.motor(3,FORWARD,150);  
        m.motor(4,BACKWARD,150);
        break;
      default: //BRAKE
        m.motor(1,BRAKE,150);
        m.motor(2,BRAKE,150);  
        m.motor(3,BRAKE,150);  
        m.motor(4,BRAKE,150);
        break;  
    }
    ThisThread::sleep_for(50ms);
  }
}

char message[50]="Udp from giga...";
void TaskSendUdp(void) {
  for(;;){
  // IPAddress remoteIP = Udp.remoteIP();
  //   unsigned int udpRemotePort = 8888;//Udp.remotePort();

  //   // Udp.beginPacket(remoteIP, udpRemotePort);
  //   time_t seconds = time(NULL);
  //   Udp.beginPacket(ipRemote, 8888);
  //   // Udp.print("t=");
  //   // Udp.print(seconds);
  //   // Udp.print("s, ");
    
  //   // Udp.println(message);

  //   Udp.println(dir);
  //   // Udp.println(normx+100);
  //   // Udp.println(".");
  //   // Udp.println(normy+100);
  //   // Udp.println(dir);

  //   Udp.endPacket();
  // ThisThread::sleep_for(50ms);
  }
}

void TaskReadUdp(void){
  for(;;){
    int udpBufferSize = Udp.parsePacket();
    if (udpBufferSize){
      int len = Udp.read(udpBufferRead, 127); //255
      if (len > 0) udpBufferRead[len] = 0;
      Serial.println(udpBufferRead);
      dir = atoi(udpBufferRead);
    }
    ThisThread::sleep_for(50ms);
  }
}

void wifiInit(void)
{
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) 
  {
    Serial.println("Communication with WiFi module failed!");
  }
  // WiFi.config(ip);
  WiFi.config(ipLocal);
  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);
  // Create open network. Change this line if you want to create an WEP network:
  int status = WL_IDLE_STATUS;
  status = WiFi.beginAP(ssid, pass); //wifi.beginAP(const char* ssid, const char* passphrase, uint8_t channel = DEFAULT_AP_CHANNEL);
  if (status != WL_AP_LISTENING) 
  {
    Serial.println("Creating access point failed");
  }
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
    // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
  // print your board's IP address:
  ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
    for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}
