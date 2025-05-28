#include <SPI.h>
#include <mcp_can.h>

#define configTICK_RATE_HZ 1
#define RTC_PERIOD_HZ( x )    ( 32768 * ( ( 1.0 / x ) ) )

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];                        // Array to store serial string
unsigned char buf[8];
unsigned char flagRecv = 0;

const int SPI_CS_PIN = 9;
const int CAN_INT_PIN = 2;                             // Set INT to pin 2
MCP_CAN CAN(SPI_CS_PIN);                               // Set CS to pin 10


void setup() 
{
  Serial.begin(115200);
  
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  //if(CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK)
  if(CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
    Serial.println("MCP2515 Initialized Successfully!");
  else
    Serial.println("Error Initializing MCP2515...");

  SPI.begin();
  
  CAN.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.
  pinMode(CAN_INT_PIN, INPUT);                            // Configuring pin for /INT input
  
  while( RTC.STATUS > 0 ) {; }                           
  RTC.CTRLA = RTC_PRESCALER_DIV1_gc | 1 << RTC_RTCEN_bp; 
  RTC.PER = RTC_PERIOD_HZ( configTICK_RATE_HZ );         
  RTC.INTCTRL |= 1 << RTC_OVF_bp;    
}

void MCP2515_ISR() {
    flagRecv = 1;
}

unsigned char stmp[8] = {150, 150, 150, 150, 150, 150, 150, 150};
ISR(RTC_CNT_vect)
{
  unsigned long int canId=150;
  stmp[7]=stmp[7]+1;
  CAN.sendMsgBuf(canId, 0, 8, stmp);
  Serial.print("CAN Tx (timer)");
  Serial.print(" Id=");
  Serial.print(canId);
  Serial.print("--> ");
  Serial.print(stmp[0]);
  Serial.print(" ");
  Serial.print(stmp[1]);
  Serial.print(" ");
  Serial.print(stmp[2]);
  Serial.print(" ");
  Serial.print(stmp[3]);
  Serial.print(" ");
  Serial.print(stmp[4]);
  Serial.print(" ");
  Serial.print(stmp[5]);
  Serial.print(" ");
  Serial.print(stmp[6]);
  Serial.print(" ");
  Serial.println(stmp[7]);

  RTC.INTFLAGS = RTC_OVF_bm;  // Clear flag by writing '1'
}

void loop() {
  if(!digitalRead(CAN_INT_PIN))                         // If CAN_INT pin is low, read receive buffer
  //if (flagRecv) 
  {
    // check if get data
    
    flagRecv = 0;                   // clear flag
    CAN.readMsgBuf(&rxId, &len, buf);      // Read data: len = data length, buf = data byte(s)
    Serial.print("CAN Rx (loop) "); 
    Serial.print(" Id=");
    Serial.print(rxId);
    Serial.print("--> ");      
    //if((rxId & 0x80000000) == 0x80000000)     // Determine if ID is standard (11 bits) or extended (29 bits)
    //  sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    //else
    //  sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);
  
    //Serial.print(msgString);
  
    //if((rxId & 0x40000000) == 0x40000000){    // Determine if message is a remote request frame.
    //  sprintf(msgString, " REMOTE REQUEST FRAME");
     // Serial.print(msgString);
    //} else {
      for(byte i = 0; i<len; i++){
        //sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(buf[i]); Serial.print(" ");
        Serial.print(msgString);
     // }
    }        
    Serial.println();
  }
}
