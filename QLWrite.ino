/*
 * Standard QL Network Write routines
 * 
 * Write Data to QL
 */

byte sendFile(String fname,byte destID,byte srcID){

  byte error=0;
  unsigned long fSize=0,fPosition=0;
  short retries=0,maxRetries=Write_Retries;
  //Clear input buffer
  for(byte count=0;count<255;count++) Buf[count]=0;
  //header
  Header[0]=destID; //Destination
  Header[1]=srcID; //Source
  Header[2]=0; //Block LSB
  Header[3]=0; //Block MSB
  
  //Open file
  SET_LED_PIN;
  error=checkSDFile(fname);//Check if file is valid.
  CLR_LED_PIN;
  if(error!=E_OK) return error;
 

  fSize=FD.handle.size();
  fPosition=0;
  do{  //Main write loop
    //Populate output buffer
    fPosition=FD.handle.position();
    if(fSize-fPosition>255){
      Header[5]=255;//bytes in block
      Header[4]=0;//data block
    } else { //Last block
      Header[5]=fSize-fPosition;
      Header[4]=1;
    }
    SET_LED_PIN;
    FD.handle.read(Buf,Header[5]);
    
    // Fix for TK2 LOAD erroneously expecting a file header 
    if(FD.useTK2Fix && (Header[2]+Header[3]==0)){ //First block
      if(Buf[0]!=0xFF){ //File with no header info
      //Create file header
      //Serial.println("Adding TK2 file header");
      for(byte count=0;count<15;count++) Buf[count]=0x00;
      Buf[0]=0xFF;
      unsigned long fSize=FD.handle.size();
      Buf[1]=(fSize & 0xFF000000)>>24;
      Buf[2]=(fSize & 0x00FF0000)>>16;
      Buf[3]=(fSize & 0x0000FF00)>>8;
      Buf[4]=(fSize & 0x000000FF);
      //reload data
      FD.handle.seek(0);
        if(fSize>240){
          Header[5]=255;
          FD.handle.read(Buf+15,240);
        } else {
          Header[5]=fSize+15;
          Header[4]=1;
          FD.handle.read(Buf+15,fSize);
        }
      }
    }
   CLR_LED_PIN;
    

    
    //Data checksum
    Header[6]=0;
    for(byte count=0;count<Header[5];count++){
      Header[6]+=Buf[count];
    }
    //header checksum
    Header[7]=0;
    for(byte count=0;count<7;count++){
      Header[7]+=Header[count];
    }
  
    //Write Data
    retries=0;
    do{ //Retry loop
      error=writeBlock();
      if(error==E_CON){
        //debugPin();
        //CLR_LED_PIN;
        FD.handle.close();
       // delay(2); //Skip block that caused contention, so ready to read next block
        return E_CON; //Contention
      }
      retries++;
      if(retries==maxRetries) error=E_MTO; //Max Timeout
    } while (error==E_TO);
          
    //Increment block counters
    if(Header[2]<255){
      Header[2]++;
    } else {
      Header[2]=0;
      Header[3]++;
    }
    //Increase retries once file starts transferring
    //BASIC programs are VERY slow loading, requiring many retries (typically 100-200)
    if((Header[2]+Header[3])>0) maxRetries=300; 

    /*
    if(error==0){ //Debug
      Serial.print("Block: ");
      Serial.println(((Header[3]<<8) + Header[2]),DEC);
      Serial.print(retries);
    }
    */
    
  } while(error==E_OK); 

  if(error==E_EOF) error=E_OK; //Report EOF as success
  //Serial.print("Bytes: ");
  //Serial.println(fPosition,DEC);
  //CLR_LED_PIN;
  FD.handle.close();
  return error;
}

byte checkSDFile(String ffname){
  FD.handle=SD.open(ffname,FILE_READ);
  if(!FD.handle){ //File not found
    //fname="";
    return E_FNF;
  }
  //Serial.print("Size: ");
  //Serial.println(FD.handle.size());
  if(FD.handle.size()==0){
    //Serial.println("Zero byte file");
    FD.handle.close();
    return E_FZB;
  }
  //Get the actual case sensitive fname of the file, ignoring dirlist file.
  if(ffname!="dirlist"){
    char tbuf[33];
    FD.handle.getName(tbuf,33);
    FD.fname=tbuf;
  }
  return E_OK;
}



byte writeBlock(){
  boolean lastByte=0;
  //Send scout
  while(sendScout()!=E_OK){ //Wait for a header to be sent (scout)
        return E_CON; //Contention detected
    };
  //set net active
  noInterrupts();
  CLR_DATA_PIN;
  delayMicroseconds(T_ACT);//Net Active T_ACT
  //Send header Data
  for(byte count=0;count<8;count++){
    writeByte(Header[count]);
  }
  interrupts();
  //Wait for ACK
  
    if(getACK()==E_OK){
      //set net active
      noInterrupts();
      delayMicroseconds(T_HDGAP);
      CLR_DATA_PIN;
      delayMicroseconds(T_ACT);
     
      //send data
      for(byte count=0;count<Header[5];count++){
        writeByte(Buf[count]);
      }
      SET_DATA_PIN;
      interrupts();
    } else { //No header ACK
      return E_TO;
    }
  
  //Wait for ACK
  if(getACK()==E_OK) { //ACK received  
    //debugPin();
    if(Header[0]==0) return E_TO; //Block is a broadcast block - force retry.
    if (Header[4]) {
      return E_EOF;
    } else {
      return E_OK; //Next Block
    }
  } else { //No data ACK
    return E_TO;
  }
 
  
}


void writeByte(byte outData){

  //Start Bit
  SET_DATA_PIN;
  delayMicroseconds(T_BIT);
  //delayUS_ASM(T_BIT);
  //Data bits
  for(byte count=0;count<8;count++){
    if(outData & (1 << count)){
      CLR_DATA_PIN;
    } else {
      SET_DATA_PIN;
    }
    delayMicroseconds(T_BIT);
    //delayUS_ASM(T_BIT);
  }
  
  //Stop bits
  CLR_DATA_PIN;
  delayMicroseconds(T_STOP);
  SET_DATA_PIN;
}

byte sendScout(){
  //Wait
  unsigned long timeout=micros();
  pinTrigUp=false;  //Enable pin interrupt to detect contention during wait phase
  ignorePinTrig=false;
  while(micros()<(timeout+T_IBLK)){};//3ms wait
  ignorePinTrig=true;
  if(pinTrigUp) return E_CON; //Contention detected

  //Send scout -TODO check for contention during this period
  CLR_DATA_PIN;
  delayMicroseconds(300); //300
  SET_DATA_PIN;
  delayMicroseconds(260); //260

  return E_OK; //OK
}

byte getACK(){
  
  if(Header[0]==0){ //Network broadcast - No ACKs.
    delayMicroseconds(T_HDGAP);//delay between header and data for broadcast. 
    return 0;
  }
  //debugPin();
  byte ACKbyte=0;
  ACKbyte=readByte();
  if(ACKbyte==1){ //0x01
    debugPin();
    return E_OK;
  }
  return E_TO;
}

