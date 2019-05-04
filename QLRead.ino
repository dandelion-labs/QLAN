/*
 * Standard QL Network Read routines
 * 
 * Read Data from QL
 * 
 */
byte receiveFile(String fName){
  byte error=0;
  short blockcount=0;
  //Clear input buffer
  for(byte count=0;count<255;count++) Buf[count]=0;
  
  
  //Main Read loop
  do{
    error=readBlock();
    //Save to SD card
    if((error==E_OK) || (error==E_EOF)){//Data is for this station
      if((Header[2]+Header[3])==0){//First block
        clientNETID=Header[1];
        if((Header[4]==1) && (Header[5]<19)){//Block is last block with <19 bytes - might be a command
          if(checkCommand()) return E_CMD; //Was a command - exit routine.
        }
        //Open file if no error
        SET_LED_PIN;
        SD.remove(fName); //Overwrite file
        currentFile=SD.open(fName,O_READ | O_WRITE | O_CREAT);
        if(!currentFile){
          Serial.println("File error");
          return E_FNF;
        }
        /*
        //Check for file header in buffer - Needed by TK2 (This is a bug in TK2 LOAD)
        if(Buf[0]!=0xFF){ //If no header, add one - populate it later with file size
          currentFile.write(0xFF);
          for(byte count=0;count<14;count++) currentFile.write((byte)0x00);
        }
        */
      }
      //Serial.print("Block: ");
      //Serial.println(blockcount);
      //blockcount++;
      currentFile.write(Buf,Header[5]); 
    }
  } while ((error==E_OK) || (error==E_CHK));
  if(error==E_EOF) error=E_OK; //Report EOF as Success
  /*
  //Populate FileHeader - Needed by TK2
  unsigned long fSize=currentFile.size()-15;
  currentFile.seek(0x01);
  currentFile.write((fSize & 0xFF000000)>>24);
  currentFile.write((fSize & 0x00FF0000)>>16);
  currentFile.write((fSize & 0x0000FF00)>>8);
  currentFile.write((fSize & 0x000000FF));
  //Serial.println(fSize);
  */
  //Close File
  currentFile.close();
  
  //Read_Retries=5;
  CLR_LED_PIN;
  return error;
}

byte readBlock(){
  byte chksum=0;
  short retries=0;
  while(waitScout()){
    retries++;
    if(retries==Read_Retries) return E_TO;//Timeout
  }
  //Serial.print("Retries: ");
  //Serial.println(retries,DEC);
  //Read header
  for(byte count=0;count<7;count++){
    Header[count]=readByte();
    chksum+=Header[count];
  }
  
  //Header Checksum
  Header[7]=readByte();
  if(chksum!=Header[7]) return E_CHK; //Checksum error
  
  if(Header[0]!=NETID) return E_NTS; //Block not for this station. Also ignore broadcast (NETID 0) blocks
  sendACK();
  //Read data
  
  chksum=0;
  for(byte count=0;count<Header[5];count++){
    Buf[count]=readByte();
    chksum+=Buf[count];
  }
  //Data Checksum
 if(chksum!=Header[6]) return E_CHK; //Checksum error
  
  sendACK();
  return Header[4];//E_OK(0) if more data, E_EOF(1) if EOF
}

byte readByte(){
  byte outData=0;
  boolean inData=0;
  unsigned long timeout=micros();
  pinTrigDown=false;//reset trig pin
  ignorePinTrig=false;
  while(!pinTrigDown){
    if(micros()>timeout+200){
      ignorePinTrig=true;  
      return E_TO;  //Timeout here will cause data errors 
    }
    
   };
  ignorePinTrig=true;
  //debugPin();
  noInterrupts();
  delayMicroseconds(T_BIT);
  for(byte count=0;count<8;count++){
    //SET_DEBUG_PIN;
    inData=READ_DATA_PIN;
    outData+=(inData<<count);
    //CLR_DEBUG_PIN;
    delayMicroseconds(T_BIT);
  }
  delayMicroseconds(20);
  interrupts();
  return outData;
}

byte waitScout(){
  //debugPin();
  unsigned long timeout=micros();
  pinTrigUp=false;  //Enable pin interrupt to detect contention during wait phase
  ignorePinTrig=false;
  while(micros()<(timeout+T_IBLK)){};//3ms wait
  ignorePinTrig=true;
  if(pinTrigUp) return E_CON; //Contention detected

  
  pinTrigUp=false;//reset trig pin
  ignorePinTrig=false;
  while(!pinTrigUp){
     if(micros()>timeout+10000){//Timeout
      ignorePinTrig=true;  
      return E_TO; 
     }
    };
  ignorePinTrig=true;
  //debugPin();
  delayMicroseconds(500);
  return E_OK;
}

void sendACK(){
  //Set net active
  noInterrupts();
  delayMicroseconds(T_ACT*2);
  CLR_DATA_PIN;
  delayMicroseconds(T_ACT);
  SET_DATA_PIN;
  writeByte(1);
  interrupts();
  debugPin();
  
}
