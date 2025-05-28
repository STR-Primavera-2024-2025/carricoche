#include "Arduino.h"
#include "mbed.h"
#include "rtos.h"
#include "RPC.h"
#include "WiFi.h" 
#include "TM1637TinyDisplay.h"//TM1637 7-segment 4x display
#include "TinyGPS.h"//GPS Global Positioning System
#include "MPU6050.h"//IMU Inertial Measurement Unit MPU6050
#include "Adafruit_GFX.h"    //TFT screen // Core graphics library
#include "Adafruit_ST7735.h" // Hardware-specific library for ST7735
#include "ArduinoJson.h"//JSON packaging

using namespace mbed;
using namespace rtos;
using namespace std::chrono;

//Joystick stuff
const uint8_t BUTTON_UP=D2;
const uint8_t BUTTON_RIGHT=D3;
const uint8_t BUTTON_DOWN=D4;
const uint8_t BUTTON_LEFT=D5;
const uint8_t BUTTON_E=D6;
const uint8_t BUTTON_F=D7;
const uint8_t BUTTON_K=D8;
const uint8_t PIN_ANALOG_X=A0;
const uint8_t PIN_ANALOG_Y=A1;

PinStatus ButtonUpState=HIGH;
PinStatus ButtonRightState=HIGH;
PinStatus ButtonDownState=HIGH;
PinStatus ButtonLeftState=HIGH;
PinStatus ButtonEState=HIGH;
PinStatus ButtonFState=HIGH;
PinStatus ButtonKState=HIGH;
unsigned int JoystickAnalogX=512;
unsigned int JoystickAnalogY=512;
PinStatus JoystickXRightState=HIGH;
PinStatus JoystickXLeftState=HIGH;
PinStatus JoystickYUpState=HIGH;
PinStatus JoystickYDownState=HIGH;
const int joystickDeadband = 100; //50
int8_t x=0;
int8_t y=0;
int8_t z=0;
int8_t xLast=0;
int8_t yLast=0;
int8_t zLast=0;
int normx;
int normy;

//TM1637 4-segments display
const uint8_t CLK=A2;
const uint8_t DIO=A3;
TM1637TinyDisplay display(CLK, DIO);//,0;
bool displayDots=true;
byte minuteRunning=0;
byte secondRunning=0;

//global positioning system (it takes half an hour to update the first values!)
TinyGPS gps;
float lat=0;//41.38;
float lon=0;//2.11;
unsigned long age=0;
int year=0;
byte day=0;
byte month=0;
byte hour=0;
byte minute=0;
byte second=0;
byte hundredths=0;

//inertial measurement unit 
float Temp=0;
float pitch=0;
float roll=0;
float yaw=0;
float pitchLast=0;
float rollLast=0;
float yawLast=0;
float xBall=0;
float yBall=0;
float xBallLast=0;
float yBallLast=0;

//laser output
PinStatus laserState=LOW;

//thin-film-transistor liquid-crystal display 
const uint8_t TFT_CS=D13;
const uint8_t TFT_DC=D11; 
const uint8_t TFT_MOSI=D10;
const uint8_t TFT_SCLK=D9;
const uint8_t TFT_RST=D12;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

//WiFi for udp, ur3 comm, telemetry, http, etc.
char ssid[] = "Grup4";   //Access point
char pass[] = "Grup4Pass";   //Access point
// char ssid[] = "wlan_str"; // your WPA network SSID (name)
// char pass[] = "wlan_str"; // your WPA network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
// IPAddress ip(192, 168, 1, 111); //static ip is not working!
IPAddress ipRemote=IPAddress(192, 168, 1, 112);//fixed ip not working...!!!
IPAddress ipLocal=IPAddress(192, 168, 1, 111);//fixed ip not working...!!!
WiFiServer server(80);
WiFiUDP Udp;  
WiFiClient client;
String ur3Command="";
// IPAddress ur3(192,168,1,100);//UR3 IP address (left robot)
const unsigned int ur3PORT=30003;
const unsigned int gripperPORT=41414;

//a mutex used when printing
Mutex printMutex;// Since we're printing from multiple threads, we need a mutex

//capacitive touch button
PinStatus capacitiveValue=LOW;

//additional global variables
unsigned int counter=0;
int M5batt;
int dir = 0;
float angle_deg;

char udpBufferRead[256];

//Function prototypes
void ledInit(void);
void ledUpdate(void);
void wifiInitAccessPoint(void);
void wifiInitWPA(void);
void tftInit(void);
void tftUpdate(void);
void motorInit(void);
void motorON(void);
void motorOFF(void);
void laserInit(void);
void laserON(void);
void laserOFF(void);
void capacitiveInit(void);
void capacitiveUpdate(void);
void imuInit(void);
void imuGetData(void);
void gpsInit(void);
void gpsGetData(void);
void joystickInit(void);
void joystickGetData(void);
void displayInit(void);
void displayUpdate(void);
void supervisionUpdate(void);

void calcDirection(void);
void TaskReadUdp(void);
void TaskSendUdp(void);

Thread threadSendUdp(osPriorityHigh);
//Thread threadReadUdp(osPriorityHigh);
Thread threadJoystick(osPriorityHigh);
//Thread threadDisplay(osPriorityHigh);
Thread threadCalcDir(osPriorityHigh);
Thread threadTFT(osPriorityHigh);

void setup()
{
  Serial.begin(115200);
  Serial.println("Init...");
  
  ledInit();
  // wifiInitAccessPoint(); //
  wifiInitWPA();
  Udp.begin(8888);
  
  tftInit();
  motorInit();
  //laserInit();
  //capacitiveInit();
  //imuInit();
  //gpsInit();
  displayInit();
  joystickInit();
 
  threadSendUdp.start(TaskSendUdp); // periodica
  //threadReadUdp.start(TaskReadUdp); // periodica
  threadJoystick.start(joystickGetData); //periodica
  //threadDisplay.start(displayUpdate); //periodic
  threadCalcDir.start(calcDirection);
  threadTFT.start(tftUpdate);
  
}

void loop() {}

void TaskSendUdp(void){
  for(;;){
    unsigned int udpRemotePort = 8888;//Udp.remotePort();
    Udp.beginPacket(ipRemote, 8888);
    Udp.println(dir);
    Udp.endPacket();
  ThisThread::sleep_for(50ms);
  }
}

void TaskReadUdp(void){
  for(;;){
    int udpBufferSize = Udp.parsePacket();
    if (udpBufferSize){
      int len = Udp.read(udpBufferRead, 127); //255
      if (len > 0) udpBufferRead[len] = 0;
    }
    ThisThread::sleep_for(200ms);
  }
}


void ledInit(void)
{
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
}

void ledUpdate(void)
{
  digitalWrite(LEDR,random(0,2));//random(min,max)//min is inclusive, max is exclusive
  digitalWrite(LEDG,random(0,2));
  digitalWrite(LEDB,random(0,2));
}

void wifiInitAccessPoint(void)
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
  status = WiFi.begin(ssid, pass); //wifi.beginAP(const char* ssid, const char* passphrase, uint8_t channel = DEFAULT_AP_CHANNEL);
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

void wifiInitWPA(void)
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
  status = WiFi.begin(ssid, pass); 
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

void tftInit(void)
{
  tft.initR(INITR_BLACKTAB); // Init ST7735S chip 128x160 black tab
  //tft.setFont(&FreeSerif9pt7b);
  tft.fillScreen(0xffff);
  //tft.fillScreen(0x0000);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  tft.setTextColor(0x0000,0xffff);
  tft.cp437(true);   // Use correct CP437 character codes

  tft.fillScreen(0xffff);
  //tft.fillScreen(0x0000);
  tft.setCursor(20, 80);
  // tft.setTextColor(0x681f,0xffff);
  // tft.print("Initializing...");
  tft.setTextColor(0x0000,0xffff);
  tft.setCursor(10, 3);
  // tft.print(ip);
  tft.print(ipLocal);
  tft.fillCircle(5, 6, 2, 0x07fc);
  tft.drawCircle(5, 6, 3, 0x195f);

  tft.drawCircle(115, 140, 2, 0xd0a0); //Up-button
  tft.drawCircle(120, 145, 2, 0xd0a0); //Right-button 
  tft.drawCircle(115, 150, 2, 0xd0a0); //Down-button
  tft.drawCircle(110, 145, 2, 0xd0a0); //Left-button
  tft.drawCircle(104, 150, 2, 0xd0a0); //E-button
  tft.drawCircle(98, 150, 2, 0xd0a0);  //F-button
  tft.drawCircle(87, 145, 2, 0xd0a0);//K-button
  tft.drawTriangle(87, 136, 84, 141, 90, 141, 0xd0a0);//Joy +y
  tft.drawTriangle(91, 142, 91, 148, 96, 145, 0xd0a0);//Joy +x
  tft.drawTriangle(87, 154, 84, 149, 90, 149, 0xd0a0);//Joy -y
  tft.drawTriangle(78, 145, 83, 148, 83, 142, 0xd0a0);//Joy -x
}

void tftUpdate(void)
{
  for(;;){

    tft.setTextColor(0x0000,0xffff);
    tft.setCursor(5, 50);  
    tft.print("Dir = ");
    tft.print(dir);
    tft.print(" ");


    // tft.setTextColor(0x0000,0xffff);
    // tft.setCursor(5, 60);  
    // tft.print("Deg = ");
    // tft.print(angle_deg);

    tft.setTextColor(0x0000,0xffff);
    tft.setCursor(5, 120);  
    tft.print("x=");
    if (normx<0) {
      tft.print("-");
      normx = normx*(-1);
    }
    tft.print(normx);
    tft.print("   ");

    tft.setTextColor(0x0000,0xffff);
    tft.setCursor(5, 130);  
    tft.print("y=");
    if (normy<0) {
      tft.print("-");
      normy = normy*(-1);
    }
    tft.print(normy);
    tft.print("   ");

    // tft.setTextColor(0x0000,0xffff);
    // tft.setCursor(5, 80);  
    // tft.print("z=");
    // tft.print(z);
    // tft.print("   ");

    // tft.setTextColor(0x0000,0xffff);
    // tft.setCursor(5, 100);  
    // tft.print("PitchRef=");
    
    // int msg = atoi(udpBufferRead);;
    // float pitchRef = ((int)(msg/1000))/100.0;

    // int pitchRefSign = pitchRef/1000;

    // pitchRef = pitchRef - (pitchRefSign*1000);
    // if (pitchRefSign == 8) {
    //   pitchRef = pitchRef*(-1.0);
    // }
    // tft.print(pitchRef);
    // tft.print("   ");

    if (ButtonUpState==LOW)
    {
      tft.fillCircle(115, 140, 2, 0xd0a0); //Up-button
    }else{
      tft.fillCircle(115, 140, 2, 0xffff); //Up-button
      tft.drawCircle(115, 140, 2, 0xd0a0); //Up-button1
    }
    if (ButtonRightState==LOW)
    {
      tft.fillCircle(120, 145, 2, 0xd0a0);
    }else{
      tft.fillCircle(120, 145, 2, 0xffff);
      tft.drawCircle(120, 145, 2, 0xd0a0);
    }
    if (ButtonDownState==LOW)
    {
      tft.fillCircle(115, 150, 2, 0xd0a0);
    }else{
      tft.fillCircle(115, 150, 2, 0xffff);
      tft.drawCircle(115, 150, 2, 0xd0a0);
    }
    if (ButtonLeftState==LOW)
    {
      tft.fillCircle(110, 145, 2, 0xd0a0);
    }else{
      tft.fillCircle(110, 145, 2, 0xffff);
      tft.drawCircle(110, 145, 2, 0xd0a0);
    } 
    if (ButtonEState==LOW)
    {
      tft.fillCircle(104, 150, 2, 0xd0a0);
    }else{
      tft.fillCircle(104, 150, 2, 0xffff);
      tft.drawCircle(104, 150, 2, 0xd0a0);
    } 
    if (ButtonFState==LOW)
    {
      tft.fillCircle(98, 150, 2, 0xd0a0);
    }else{
      tft.fillCircle(98, 150, 2, 0xffff);
      tft.drawCircle(98, 150, 2, 0xd0a0);
    } 
    if (ButtonKState==LOW)
    {
      tft.fillCircle(87, 145, 2, 0xd0a0);
    }else{
      tft.fillCircle(87, 145, 2, 0xffff);
      tft.drawCircle(87, 145, 2, 0xd0a0);
    } 
    if (JoystickXRightState==HIGH)
    {
      tft.fillTriangle(91, 142, 91, 148, 96, 145, 0xd0a0);
    }else{
      tft.fillTriangle(91, 142, 91, 148, 96, 145, 0xffff);
      tft.drawTriangle(91, 142, 91, 148, 96, 145, 0xd0a0);
    } 
    if (JoystickXLeftState==HIGH)
    {
      tft.fillTriangle(78, 145, 83, 148, 83, 142, 0xd0a0);
    }else{
      tft.fillTriangle(78, 145, 83, 148, 83, 142, 0xffff);
      tft.drawTriangle(78, 145, 83, 148, 83, 142, 0xd0a0);
    } 
    if (JoystickYUpState==HIGH)
    {
      tft.fillTriangle(87, 136, 84, 141, 90, 141, 0xd0a0);
    }else{
      tft.fillTriangle(87, 136, 84, 141, 90, 141, 0xffff);
      tft.drawTriangle(87, 136, 84, 141, 90, 141, 0xd0a0);
    } 
    if (JoystickYDownState==HIGH)
    {
      tft.fillTriangle(87, 154, 84, 149, 90, 149, 0xd0a0);
    }else{
      tft.fillTriangle(87, 154, 84, 149, 90, 149, 0xffff);
      tft.drawTriangle(87, 154, 84, 149, 90, 149, 0xd0a0);
    } 
    ThisThread::sleep_for(200ms);
  }
}

void motorInit(void)
{
  pinMode(37,OUTPUT);
  pinMode(39,OUTPUT);
  pinMode(41,OUTPUT);
  digitalWrite(37,LOW);
  digitalWrite(39,HIGH);
}
void motorON(void)
{
  digitalWrite(41,HIGH);
}
void motorOFF(void)
{
  digitalWrite(41,LOW);
}

void laserInit(void)
{
  pinMode(D53,OUTPUT);
  pinMode(D51,OUTPUT);
  digitalWrite(D53,HIGH);
}
void laserON(void)
{
  laserState=HIGH;
  digitalWrite(D51,laserState);
  
}
void laserOFF(void)
{
  laserState=LOW;
  digitalWrite(D51,laserState);
}

void capacitiveInit(void)
{
  pinMode(23,INPUT);
  pinMode(25,OUTPUT);
  digitalWrite(25,LOW);
}
void capacitiveUpdate(void)
{
  capacitiveValue=digitalRead(23);
}

void imuInit(void)
{
  Wire.begin();
  //Activate the MPU-6050
  Wire.beginTransmission(0x68);// Device Address
  Wire.write(0x6B);// Register to write too
  Wire.write(0x00);// Value to write
  Wire.endTransmission();

  // Set Accelerometer sensitivity
  // Wire.write; 2g -> 0x00, 4g -> 0x08, 8g -> 0x10, 16g -> 0x18
  Wire.beginTransmission(0x68);
  Wire.write(0x1C);
  Wire.write(0x08);
  Wire.endTransmission();

  // Set Gyro sensitivity
  // 250 deg/s -> 0x00, 500 deg/s -> 0x08, 1000 deg/s -> 0x10, 2000 deg/s -> 0x18
  Wire.beginTransmission(0x68);
  Wire.write(0x1B);
  Wire.write(0x08);
  Wire.endTransmission();  
}

void imuGetData(void)
{
   // Set the Register to read from
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission();

  // Request 14 bytes from MPU6050
  Wire.requestFrom(0x68, 14);

  // Read data 6x Acceleration Bytes, 2x Temp Bytes, 6x Gyro Bytes
  //               High Byte          Low Byte
  int16_t acc_x = (Wire.read() << 8 | Wire.read());
  int16_t acc_y = Wire.read() << 8 | Wire.read();
  int16_t acc_z = Wire.read() << 8 | Wire.read();
  int16_t temperature = Wire.read() <<8 | Wire.read();
  int16_t gyro_x = Wire.read()<<8 | Wire.read();
  int16_t gyro_y = Wire.read()<<8 | Wire.read();
  int16_t gyro_z = Wire.read()<<8 | Wire.read();

  //Acceleration Conversion
  float ax = acc_x/8192.0;
  float ay = acc_y/8192.0;
  float az = acc_z/8192.0;

  // Temperature Conversion
  Temp = (temperature/340.0)+36.53;

  // Gyroscope Conversion
  // double gX = gyro_x/65.5;
  // double gY = gyro_y/65.5;
  // double gZ = gyro_z/65.5;

  // mpu.getMotion6(&ax, &ay, &az, &wx, &wy, &wz);
  // Temp=mpu.getTemperature()/340.0f + 36.53f;
  roll =  360.0/6.28*atan2(ax,sqrtf(ay*ay+az*az));
  pitch = 360.0/6.28*atan2(ay,sqrtf(ax*ax+az*az));
}

void gpsInit(void)
{
  Serial1.begin(9600);
}

void gpsGetData(void)
{
  bool newData = false;
  while (Serial1.available())
  {
    newData = true;
    char c = Serial1.read();
    // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
    if (gps.encode(c)) // Did a new valid sentence come in?
      newData = true;
  }

  if (newData)
  {
    unsigned long age;
    gps.f_get_position(&lat, &lon, &age);
    gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths,&age);
    hour=hour+1;//CEST+1
  }

  if (lat>90) lat=0;
  if (lon>360) lon=0;

  if ( (hour>59) || (hour<0) )
  {
    hour=0;
    minute=0;
    second=0;
  }
}

void joystickInit(void)
{
	pinMode(BUTTON_UP, INPUT);
	pinMode(BUTTON_RIGHT, INPUT);
	pinMode(BUTTON_DOWN, INPUT);
	pinMode(BUTTON_LEFT, INPUT);
	pinMode(BUTTON_E, INPUT);
	pinMode(BUTTON_F, INPUT);
	pinMode(BUTTON_K, INPUT);
	pinMode(PIN_ANALOG_X, INPUT);
	pinMode(PIN_ANALOG_Y, INPUT);
}

void joystickGetData(void)
{
  for(;;) {
    JoystickAnalogX=analogRead(PIN_ANALOG_X);
    JoystickAnalogY=analogRead(PIN_ANALOG_Y);

    ButtonUpState=digitalRead(BUTTON_UP);
    ButtonRightState=digitalRead(BUTTON_RIGHT);
    ButtonDownState=digitalRead(BUTTON_DOWN);
    ButtonLeftState=digitalRead(BUTTON_LEFT);
    ButtonEState=digitalRead(BUTTON_E);
    ButtonFState=digitalRead(BUTTON_F);
    ButtonKState=digitalRead(BUTTON_K);
    
    if (JoystickAnalogX>512+joystickDeadband)
    {
      x=x+1;
      JoystickXRightState=HIGH;
      if (x>100)x=100;
    }else{
      JoystickXRightState=LOW;
    }

    if (JoystickAnalogX<512-joystickDeadband)
    {
      x=x-1;
      JoystickXLeftState=HIGH;
      if (x<-100)x=-100;
    }else{
      JoystickXLeftState=LOW;
    }

    if (JoystickAnalogY>512+joystickDeadband)
    {
      y=y+1;
      JoystickYUpState=HIGH;
      if (y>100)y=100;
    }else{
      JoystickYUpState=LOW;
    }

    if (JoystickAnalogY<512-joystickDeadband)
    {
      y=y-1;
      JoystickYDownState=HIGH;
      if (y<-100)y=-100;
    }else{
      JoystickYDownState=LOW;
    } 

    if (ButtonKState==RISING)
    {
      // x=0;
      // y=0;
    }

    if (ButtonUpState==LOW)
    {
      z=z+1;
      if (z>100)z=100;
    }

    if (ButtonDownState==LOW)
    {
      z=z-1;
      if (z<-100)z=-100;
    }

    if (ButtonRightState==LOW)
    {
      yaw=yaw+1.0;
    }

    if (ButtonLeftState==LOW)
    {
      yaw=yaw-1.0;
    }    

    xLast=x;
    yLast=y;

    normx = (((double)JoystickAnalogX-512)/512)*100;
    normy = (((double)JoystickAnalogY-512)/512)*100;

    // angle_deg = atan2(normy/100.0, normx/100.0) * (180.0 / PI);
    // Serial.println(angle_deg);

    // Serial.print("x = ");
    // Serial.println(normx);
    // Serial.print("y = ");
    // Serial.println(normy);
    // Serial.println("------------");

    ThisThread::sleep_for(100ms);
  }
}

void calcDirection(void)
{
  for(;;){
    /*
      8 1 2
      7 0 3
      6 5 4
    */
    dir = 0;
    if (JoystickYUpState                        ) dir = 1;
    if (                     JoystickXRightState) dir = 3;
    if (JoystickYDownState                      ) dir = 5;
    if (                     JoystickXLeftState ) dir = 7;
    if (JoystickYUpState   & JoystickXRightState) dir = 2;
    if (JoystickYDownState & JoystickXRightState) dir = 4;
    if (JoystickYDownState & JoystickXLeftState ) dir = 6;
    if (JoystickYUpState   & JoystickXLeftState ) dir = 8;
    if (ButtonLeftState==LOW)                     dir = 9;
    if (ButtonRightState==LOW)                    dir = 10;
    
    // Serial.print("Direction: ");
    // Serial.println(dir);

    ThisThread::sleep_for(200ms);
  }
}

void displayInit(void)
{
  display.begin();
  display.showString("Init");
}

void displayUpdate(void)
{
 for(;;){
    // counter++;

    // // secondRunning=(xTaskGetTickCount()/configTICK_RATE_HZ)%60;
    // // minuteRunning=(xTaskGetTickCount()/configTICK_RATE_HZ)/60;
    // auto timeSecondsNow = time_point_cast<seconds>(Kernel::Clock::now()); // Convert time_point to one in microsecond accuracy
    // long timeSeconds = timeSecondsNow.time_since_epoch().count();
    // secondRunning=timeSeconds%60;
    // minuteRunning=timeSeconds/60;

    // if (counter%1==0)
    // {
    //   displayDots=!displayDots;
    // }
    // //display.showNumberDec(int num, uint8_t dots = 0, bool leading_zero = false, uint8_t length = MAXDIGITS, uint8_t pos = 0);
    // if ( (minute!=0) && (hour!=0) )
    // {
    //   display.showNumberDec(minute,0b01000000*displayDots,true,2,2);
    //   display.showNumberDec(hour,0b01000000*displayDots,true,2,0);
    // }else{
    //   display.showNumberDec(secondRunning,0b01000000*displayDots,true,2,2);
    //   display.showNumberDec(minuteRunning,0b01000000*displayDots,true,2,0);
    // } 

    int msg = atoi(udpBufferRead);
    Serial.println(msg);
    int M5batt = msg%1000;

    // double daux = 3.8;
    // int aux = daux*100;
    // display.showNumberDec(aux,0b01000000*displayDots,true,2,2);
    // display.showNumberDec(aux);
    display.showNumberDec(M5batt, 0b01000000, true, 4, 0);

    // double daux = 3.85;
    // int decimals = (daux-int(daux))*100;
    // display.showNumberDec(int(daux), 0, false, 1, 0);
    // display.showNumberDec(decimals, 0, false, 2, 2);

    ThisThread::sleep_for(200ms);
  }
}

void supervisionUpdate(void)
{
  printMutex.lock();
  Serial.println("OSC");
  Serial.print(Temp);
  Serial.print(",");
  Serial.print(pitch);
  Serial.print(",");
  Serial.print(roll);
  Serial.print(",");    
  Serial.print(lat);
  Serial.print(",");
  Serial.print(lon);
  Serial.println(" ");
  printMutex.unlock();
}

// void ur3Init(void)
// {
//   client.println("set_digital_out(0,True)");
// }

// void ur3Update(void)
// {
//     bool success =	false;
  
//   if (client.connect(ur3, ur3PORT))
// 	{
//     	Serial.println("Connected to ur3");
//   }
//   client.println("set_digital_out(0,True)");
// }

// bool rg_grip(int rg_id = 0, float target_width = 100, float target_force = 10) {
//   // Ensure target_width and target_force are within the specified ranges
//   target_width = min(max(target_width, 0), 100);
//   target_force = min(max(target_force, 0), 40);

//   // Construct the XML request
//   String xml_request = "<?xml version=\"1.0\"?>\r\n"
//                        "<methodCall>\r\n"
//                        "  <methodName>rg_grip</methodName>\r\n"
//                        "  <params>\r\n"
//                        "    <param>\r\n"
//                        "      <value><int>" + String(rg_id) + "</int></value>\r\n"
//                        "    </param>\r\n"
//                        "    <param>\r\n"
//                        "      <value><double>" + String(target_width) + "</double></value>\r\n"
//                        "    </param>\r\n"
//                        "    <param>\r\n"
//                        "      <value><double>" + String(target_force) + "</double></value>\r\n"
//                        "    </param>\r\n"
//                        "  </params>\r\n"
//                        "</methodCall>\r\n";

//   if (!client2.connect(ur3, gripperPORT)) {
//     return false; 
//   }

//   String http_request = "POST / HTTP/1.1\r\n"
//                         "Host: " +
//                         String(ur3[0]) + String(".") +
//                         String(ur3[1]) + String(".") +
//                         String(ur3[2]) + String(".") +
//                         String(ur3[3]) +
//                         "\r\n"
//                         "Content-Type: application/x-www-form-urlencoded\r\n"
//                         "Content-Length: " + String(xml_request.length()) + "\r\n\r\n";
  
//   client2.print(http_request + xml_request);
//   Serial.print(http_request + xml_request);

//   delay(100); 
 
//   while (client2.available()) {
//     String response = client2.readStringUntil('\r');
//     if (response.indexOf("200 OK") != -1) {
//       return true; // Success
//     }
//   }

//   return false; 
// }
