#include "mbed.h"
#include "rtos.h"
#include "Arduino_GigaDisplay_GFX.h"
#include "LD06forArduino.h"
#include "ArduinoGraphics.h"
#include "CAN.h"


/*
Connections:
L   White   ----------\/-----Yellow---- Not connected-----------A G
I   Yellow  ----------/\-----White----- Pin0--------------------R I
D   Red     -------\         Void                               D G
A   Blue    ----\    --------Red------- 5V (3.3Vnot working)----U A
R                ------------Blue------ GND---------------------INO                 
Color conversion: https://rgbcolorpicker.com/565
*/


using namespace mbed;
using namespace rtos;

#define GC9A01A_CYAN    (uint16_t)0x07FF
#define GC9A01A_RED     (uint16_t)0xf800
#define GC9A01A_BLUE    (uint16_t)0x001F
#define GC9A01A_GREEN   (uint16_t)0x07E0
#define GC9A01A_MAGENTA (uint16_t)0xF81F
#define GC9A01A_WHITE   (uint16_t)0xffff
#define GC9A01A_BLACK   (uint16_t)0x0000
#define GC9A01A_YELLOW  (uint16_t)0xFFE0
#define STOP 0


Thread lidarThread(osPriorityHigh7);//, DEFAULT_STACK_SIZE
void lidar(void);

// Thread canRxThread(osPriorityHigh7);//, DEFAULT_STACK_SIZE
// void canRxThreadCode(void);

Thread canTxThread(osPriorityHigh7);//, DEFAULT_STACK_SIZE
void canTxThreadCode(void);

mbed::CAN can(PB_5, PB_13);
mbed::CANMessage msgRx;

int diameter = 400;

GigaDisplay_GFX Display;

float centerX = Display.width() / 2;
float centerY = Display.height() / 2;
LD06 ld06(Serial1);

void setup() {
  Serial.begin(115200);
  
  can.frequency(500000);
  
  Display.begin();
  ld06.init();
  ld06.disableCRC();
    
  float x1 = 0;
  float y1 = 0;
  float x2 = 0;
  float y2 = 0;

  Display.fillScreen(0xffff);
  Display.drawCircle(centerX, centerY, diameter / 2, 0x0410);

  for (int i = 0; i <= 360; i = i + 1) {
    x1 = (diameter / 2 - 5) * cosf(i * 2.0 * PI / 360.0);
    y1 = (diameter / 2 - 5) * sinf(i * 2.0 * PI / 360.0);
    x2 = (diameter / 2 + 5) * cosf(i * 2.0 * PI / 360.0);
    y2 = (diameter / 2 + 5) * sinf(i * 2.0 * PI / 360.0);
    Display.drawLine(centerX + x1, centerY + y1, centerX + x2, centerY + y2, 0x0410);
  }
  for (int i = 0; i <= 360; i = i + 5) {
    x1 = (diameter / 2 - 10) * cosf(i * 2.0 * PI / 360.0);
    y1 = (diameter / 2 - 10) * sinf(i * 2.0 * PI / 360.0);
    x2 = (diameter / 2 + 10) * cosf(i * 2.0 * PI / 360.0);
    y2 = (diameter / 2 + 10) * sinf(i * 2.0 * PI / 360.0);
    Display.drawLine(centerX + x1, centerY + y1, centerX + x2, centerY + y2, 0x0410);
  }
  for (int i = 0; i <= 360; i = i + 10) {
    x1 = (diameter / 2 - 15) * cosf(i * 2.0 * PI / 360.0);
    y1 = (diameter / 2 - 15) * sinf(i * 2.0 * PI / 360.0);
    x2 = (diameter / 2 + 15) * cosf(i * 2.0 * PI / 360.0);
    y2 = (diameter / 2 + 15) * sinf(i * 2.0 * PI / 360.0);
    Display.drawLine(centerX + x1, centerY + y1, centerX + x2, centerY + y2, 0x0410);
  }

  lidarThread.start(lidar);
  // canRxThread.start(canRxThreadCode);
  canTxThread.start(canTxThreadCode);
}


int colour = 0x867D;

void lidar(void){
  const unsigned int TlidarThread=20; //periodicity of the red led thread in ms
  uint64_t nextWakeTime = TlidarThread;

  for (;;) 
  {  // Repeat forever
    if (ld06.readScan()) {            
    uint16_t n = ld06.getNbPointsInScan();  
    int maxDist = 3000; //mm

//    float oldX = centerX  + cosf(ld06.getPoints(n-1)->angle * 2.0 * PI / 360.0) * map(min(ld06.getPoints(n-1)->distance, maxDist), 0, maxDist, 0, diameter/2 - 10);
//    float oldY = centerY + sinf(ld06.getPoints(n-1)->angle * 2.0 * PI / 360.0) * map(min(ld06.getPoints(n-1)->distance, maxDist), 0, maxDist, 0, diameter/2 - 10);

    Display.fillCircle(centerX, centerY, diameter/2 - 5 , 0xffff);  
    
    float minAngle1 = 100000;
    float minDist1 = 100000;
    float minAngle2 = 100000;
    float minDist2 = 100000;

    for (uint16_t i = 0; i < n; i+=2) {
      float angle = ld06.getPoints(i)->angle+90.0;
      float dist = ld06.getPoints(i)->distance;
      
      // if (angle < PI/2 and angle > PI*3/2) { //RADIANTS
      // if (angle < 90 and angle > 270) { // graus º
      if (angle < 90 and angle > 270) { // MIREM QUE ELS DOS PUNTS ESTIGUIN A LA DRETA
        switch (dist) {
          case > 500: // QUE ESTIQUIN MINIM MÉS A PROP QUE 50 CM
            break;
          case < minDist1: 
            minDist1 = dist;
            minAngle1 = angle;
            break;
          case < minDist2:
            minDist2 = dist;
            minAngle2 = angle;
            break;
        }
      }

      if (dist < 300) {
        colour = GC9A01A_RED;
      }
      else colour = 0x867D;

      if(80 < dist){ 
        float newX = centerX  + cosf(angle * 2.0 * PI / 360.0) * map(min(dist, maxDist), 0, maxDist, 0, diameter/2 - 10);
        float newY = centerY + sinf(angle * 2.0 * PI / 360.0) * map(min(dist, maxDist), 0, maxDist, 0, diameter/2 - 10);
      
        //Display.drawPixel(newX,newY,0x867D);
        Display.fillRect(newX,newY,5,5,colour);
//      Display.drawLine(oldX, oldY, newX, newY, 0x867D);
//      Display.drawLine(oldX+1, oldY+1, newX+1, newY+1, 0xc141);
//      Display.drawLine(oldX-1, oldY-1, newX-1, newY-1, 0xc141);
//      Display.drawLine(oldX+2, oldY+2, newX+2, newY+2, 0xc141);
//      Display.drawLine(oldX-2, oldY-2, newX-2, newY-2, 0xc141);
      }      
      //oldX = newX;
      //oldY = newY;      
    }

  int distanciaMigCotxe = 200; // 20 centímetres aprox
  if (minDist1 != 100000 and minDist2 != 100000) { // SI EXISTEIXEN DOS PUNTS MÍNIMS EN UN MATEIX COSTAT
    float meitat1 = sinf(minAngle1 * 2.0 * PI / 360.0)*minDist1;
    float meitat2 = sinf(minAngle2 * 2.0 * PI / 360.0)*minDist2;
    if (meitat1 < 0) meitat1 *= -1;
    else if (meitat2 < 0) meitat2 *= -1;
    if (meitat1 1 > distanciaMigCotxe and meitat2 > distanciaMigCotxe) {
      STOP = 1;
    }
  }

    Display.drawLine(centerX+4, centerY, centerX-2, centerY+3, 0xc141);
    Display.drawLine(centerX-2, centerY+3, centerX-2, centerY-3, 0xc141);
    Display.drawLine(centerX-2, centerY-3, centerX+4, centerY, 0xc141);
  }
  nextWakeTime=TlidarThread-(get_ms_count()%TlidarThread);
  ThisThread::sleep_for(nextWakeTime);
  }
}


//TODO: move to external pin interrupt driven thread
// void canRxThreadCode(void) 
// {
//   const unsigned int TcanRxThread=50; //periodicity of the red led thread in ms
//   uint64_t nextWakeTime = TcanRxThread;

//   for (;;) 
//   {  // Repeat forever    
//     if (can.read(msgRx))
//     {      
//       Serial.print("CAN Rx (thread) Id=");
//       Serial.print(msgRx.id); 
//       Serial.print("--> ");
//       for(byte i = 0; i<msgRx.len; i++)
//       {
//           Serial.print(msgRx.data[i]); 
//           Serial.print(" ");
//       }
//       Serial.println();   
//     }     

//     nextWakeTime=TcanRxThread-(get_ms_count()%TcanRxThread);
//     ThisThread::sleep_for(nextWakeTime);
//   }
// }

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
    msgTx.data[7]=0;

    if (STOP) {
      STOP = 0;
      msgTx.data[7]=1;
    }

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

void loop(void) {
}
