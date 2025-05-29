#include <mbed.h>
#include <rtos.h>
#include <RPC.h>
#include <SPI.h>
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

mbed::CAN can(PB_5, PB_13);
mbed::CANMessage msgRx;


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

  redLedThread.start(redLedThreadCode);
  canRxThread.start(canRxThreadCode);
  canTxThread.start(canTxThreadCode);
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
  
  long unsigned int txId=56;
  unsigned char len = 8;
  unsigned char txBuf[8]={55,55,55,55,55,55,55,55};
  // mbed::CANMessage(txId, txBuf, len) msgTx;
  mbed::CANMessage msgTx;

  for (;;) 
  {  // Repeat forever    
    msgTx.id=56;
    msgTx.len=8;
    msgTx.data[0]=55;
    msgTx.data[1]=55;
    msgTx.data[2]=55;
    msgTx.data[3]=55;
    msgTx.data[4]=55;
    msgTx.data[5]=55;
    msgTx.data[6]=55;
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

void loop(){}
