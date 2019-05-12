byte checkCommand(){
  /*
   * Note - This is not part of the QL network protocol.
   * This protocol extension allows the user to select files for loading and saving
   * by PRINTing a command string to this network station.
   * 
   * See the command defines for the command codes
   * 
   * "CaA:fname.ext" - request a file called fname.ext
   * 
   * 
   * e.g: - to load the file 'example.bas':
   * 
   * OPEN #3,neto_63
   * PRINT #3,"CaA:example.bas"
   * CLOSE #3
   * LOAD neti_63
   * 
   */
  if(Buf[3]==':'){ 
    String cmd=Buf;
    command=cmd.substring(1,3);
    parameter=cmd.substring(4);
    parameter.trim();
    return E_CMD; //command
  }

  
  return E_OK; //Not a command
}

byte execCommand(){//File action commands
  if(command==C_NFILE && parameter!=""){//Set current file
    FD.fname=validateFilename(parameter);
    Serial.println("Current File:\"" + FD.fname +"\"");
    command="";  
  }
  if(command==C_NDIR){//Display current directory listing
    byte error=0;
    printDirectory(currentPath);
    boolean TK2Fix=FD.useTK2Fix;
    FD.useTK2Fix=false;
    do{
      error=sendFile("dirlist",clientNETID,NETID);
    } while ((error!=E_OK) && (error!=E_CON));
    FD.useTK2Fix=TK2Fix;
    SD.remove("dirlist");
    //Serial.println("Complete");
  }
  if(command==C_NDEL){//Delete file
    char filename[37];
    validateFilename(parameter).toCharArray(filename,37);
    SD.remove(filename);
  }
  if(command==C_NCHDIR){//Change directory
    char dirname[37];
    validateFilename(parameter).toCharArray(dirname,37);
    if(SD.chdir(dirname)) currentPath=dirname;
  }
  if(command==C_NMKDIR){//Make directory
    char dirname[37];
    validateFilename(parameter).toCharArray(dirname,37);
    SD.mkdir(dirname);
  }
  if(command==C_NRMDIR){//Remove directory
    char dirname[37];
    validateFilename(parameter).toCharArray(dirname,37);
    SD.rmdir(dirname);
  }
  if(command==C_NMOVE){//Move file
    char filesource[37];
    char filedest[37];
    short delimiter=parameter.indexOf(',');
    if(delimiter>2){//File name delimited ',' must be found
      validateFilename(parameter.substring(0,delimiter)).toCharArray(filesource,37);
      validateFilename(parameter.substring(delimiter+1)).toCharArray(filedest,37);
      File handle=SD.open(filesource);
      if(handle){
        handle.rename(filedest);
      }
    }
  }
  
}

String validateFilename(String fname){
  //Truncate filename to 36 chars (QL max)
  if(fname.length()>36) fname=fname.substring(0,36);
  //Remove illegal filename chars
  String illegals[]={"*","\\","\"","?",":","|",">","<"};
  for(byte count=0;count<8;count++) fname.replace(illegals[count],"");
  return fname;
}

void printDirectory(String dirname) {
  
  //Prints the selected directory listing to the file 'dirlist'
  //This file can then be sent to the QL as a directory listing
  //SD.remove("dirlist");
  File dir=SD.open(dirname);
  File output=SD.open("dirlist",O_WRITE | O_CREAT | O_TRUNC);
  if(!output) Serial.println("File error");
  while (true) {
    char tbuf[37];
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      output.print("\n");
      output.print("Current file: ");
      output.print(currentPath);
      if(currentPath!="/") output.print("/");
      output.print(FD.fname);
      output.print("\n\n");
      break;
    }
    entry.getName(tbuf,37);
    
    if (entry.isDirectory()) {
      output.print(tbuf);
      output.print("/\n");
    } else {
      if((String)tbuf!="dirlist"){ //Don't display dirlist file
        output.print(tbuf);
        output.print("  ");
        output.print(entry.size(), DEC);
        output.print("\n");
      }
    }
    entry.close();
  }
  output.close();
  dir.close();
  
}

void debugPin(){
  SET_DEBUG_PIN;
  delayMicroseconds(3);
  CLR_DEBUG_PIN;
}

/*
void viewFile(String fname){ //Debug only
//  if(SD.exists(fname)){
    if(FD.handle=SD.open(ffname,FILE_READ)){
    for(short count=0;count<FD.handle.size();count++){
      Serial.write(FD.handle.read());
    }
    FD.handle.close();
  } else {
    Serial.println("File not found");
  }
  Serial.println();
}
*/

