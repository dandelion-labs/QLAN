void printDirectory(String dirname) {
  //Prints the selected directory listing to the file 'dirlist'
  //This file can then be sent to the QL as a directory listing
  File dir=SD.open(dirname);
  File output=SD.open("dirlist",O_WRITE | O_CREAT | O_TRUNC);
  if(!output) Serial.println("File error");
  while (true) {
    char tbuf[33];
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      output.print("\n");
      output.print("Current file: ");
      output.print(fileName);
      output.print("\n\n");
      break;
    }
    entry.getName(tbuf,33);
    
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
