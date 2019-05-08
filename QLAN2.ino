

/**
 * STM32F103C "Blue Pill"
 * Sinclair QL network interface
 * 
 * Copyright (c) 2019 Jason Lucas / Dandelion Labs
 * 
 *Permission is hereby granted, free of charge, to any person obtaining a copy
 *of this software and associated documentation files (the "Software"), to deal
 *in the Software without restriction, including without limitation the rights
 *to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *copies of the Software, and to permit persons to whom the Software is
 *furnished to do so, subject to the following conditions:
 *
 *The above copyright notice and this permission notice shall be included in all
 *copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *SOFTWARE.
 *
 * 
 * QL standard network header format:
 * Please refer to the Toolkit2 documentation for
 * further details on the network protocol.
 * 
 * byte     Description
 * 0        Destination Station Number
 * 1        Sending Station Number
 * 2        LSB Block Number
 * 3        MSB Block Number
 * 4        Block type - 0 data, 1 last block (eof)
 * 5        Number bytes in block
 * 6        Data Checksum (simple addition)
 * 7        Header Checksum (simple addition)
 * 
 * PINS
 * 
 * PB13 - Debug Out
 * PB14 - Data Out (-ve logic)
 * PB15 - Data In
 * 
 * 
 * 
 */

 /*
  * TODO
  * 
  *
  * - TK2 broadcast mode
  */
  
 
//SD Card
#include <SPI.h>
#include <SdFat.h>

//Error Codes
#define E_OK 0 //OK
#define E_EOF 1 //End of file
#define E_NTS 2 //Block not for this station
#define E_TO 3 //Timeout error
#define E_MTO 4 //Maximum timeout retries exceeded
#define E_CMD 5 //Command received
#define E_CHK 6 //Checksum error
#define E_CON 7 //Network Contention error
#define E_FNF 8 //File not found
#define E_FZB 9 //File has zero bytes

//Timings (all in uS unless otherwise stated)
#define T_BIT 11  //Bit length 11
#define T_STOP 55 //Stop bits 55
#define T_ACT 22 //Network Active 22
#define T_HDGAP 150 //Gap between header ACK and data block 150
#define T_IBLK 3000 //Inter-Block gap 3000

//Commands
#define C_FILE "aA" //Set currently active file - parameter: filename
#define C_DIR "aB" //Show current directory listing - parameter: none
#define C_DEL "aC" //Delete file - parameter: filename
#define C_CHDIR "aD" //Change directory - parameter: dirname
#define C_MKDIR "aE" //Make new directory - parameter: dirname
#define C_RMDIR "aF" //Remove directory - parameter: dirname
#define C_MOVE "aG" //Move file

SdFat SD;

class fileData{
  public:
    String fname="boot";
    File handle;
    boolean useTK2Fix=true;
};


fileData FD;


String command="",parameter=""; //Current command & parameter
String currentPath="/";
byte clientNETID=1; //Currently active client NETID (63-self inactive)

static byte NETID=63; //Station ID
static byte ScoutRetries=5; //Number of scout send retries before failure
static short Write_Retries=5;
short Read_Retries=50;

volatile boolean ignorePinTrig=true,pinTrigUp=false,pinTrigDown=false;

char Buf[256],Header[8];

void setup() {
  // put your setup code here, to run once:
  
  //Serial
  Serial.begin(115200);
  delay(2000);

  /* Generic STM32F103C board definition
   *Board definition fnames can be found by setting the compile verbose setting
   *& looking for the string after one of the -D compile options.
   *
   *E.G, Arduino nano  -DARDUINO_AVR_NANO
  */
  #ifdef ARDUINO_GENERIC_STM32F103C
    Serial.println("STM32F103C");
    pinMode(PB12,OUTPUT);//Debug pin
    pinMode(PB13,OUTPUT);//LED pin
    pinMode(PB14,OUTPUT);//Data Out - ***Inverted logic***
    pinMode(PB15,INPUT);//Data In
    digitalWrite(PB14,HIGH);
    #define INT_PIN PB15  //Hardware interrupt pin
    //Direct port access
    #define SET_DATA_PIN GPIOB->regs->BSRR = 0b0100000000000000 //set PB14
    #define CLR_DATA_PIN GPIOB->regs->BRR = 0b0100000000000000 //clear PB14
    #define READ_DATA_PIN GPIOB->regs->IDR & 0b1000000000000000 //Read PB15
    #define SET_LED_PIN GPIOB->regs->BSRR = 0b0010000000000000; //LED set PB13
    #define CLR_LED_PIN GPIOB->regs->BRR = 0b0010000000000000; //LED clear PB13
    #define SET_DEBUG_PIN GPIOB->regs->BSRR = 0b0001000000000000 //Debug set PB12
    #define CLR_DEBUG_PIN GPIOB->regs->BRR = 0b0001000000000000 //Debug clear PB12
  #endif

  //Net data in interrupt
  attachInterrupt(digitalPinToInterrupt(INT_PIN), pinTrig_isr, CHANGE);
  
  //Init SD card
  Serial.print("Initializing SD card...");
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  } 
  Serial.println("initialization done.");
  Serial.println("Server Started.");
  //printDirectory("/");
  
}

void pinTrig_isr(){
  if(ignorePinTrig) return;
  if(READ_DATA_PIN){
    pinTrigUp=true;
  } else {
    pinTrigDown=true;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("Sending broadcast");
  
  byte error=0;
  error=sendFile(FD.fname,clientNETID,NETID);
  if(error>=E_CON) {
    error=receiveFile(FD.fname);
    if(error==E_OK) Serial.println("Receive Success");
    if(error==E_CMD) execCommand();
  }
  
}
