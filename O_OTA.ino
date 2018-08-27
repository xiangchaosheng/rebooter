/* Web_OTA update */
void webOTAPageLoad(AsyncWebServerRequest *request){
  Serial.println("\nwebOTA page-Load");  
  ESP.wdtFeed();
  if(connectedToRouter){
    request->send(SPIFFS, "/webOTA.html");           //必须有这个 /
  }
  else{
    Serial.println("Cannot get onlin to check OTA because not connected to Router."); 
    request->send(200, "text/json", "{\"code\":\"0\",\"data\":\"Error: Not connected to Router. Cannot access Internet.\"}");    
    request->send(SPIFFS, "/quick.html");           //必须有这个 /
  }          
  Serial.println("End of webOTA page-Load");
}


void webOTAPageSubmit(AsyncWebServerRequest *request){
  Serial.println("\n...Web OTA page submit() Starts...");
  if (connectedToRouter){                                  //只是连接到路由器，并不能保证能够访问Internet
      Serial.println("Set userPermitOTA = true. ");
      userPermitOTA = true;                
  }
  else{
      Serial.println("Not connected to router. Set userPermitOTA = false."); 
      userPermitOTA = false;
  }
  Serial.println("...Web OTA page submit() Ends...\n");
}

/*  OTA check:
 *  False: not successfully connect to Server or No updates
 *  True:  updates available
 */
bool otaCheck(){
  WiFiClient client;        //check web OTA server URL to figure out whether web ota available 
  String result, tempStr;
  char c;
  int i,j;
  Serial.println("otaCheck() : Check to see if OTA updates available from Server.");
  if(connectedToRouter){
    //check to identify whether there is updates available
    //work with Ethan to fill in code here to check Update availability
    Serial.print("Connecting to ");
    Serial.println(otaURL);
    if (!client.connect(otaURL, httpPort)) {                              // convert String to char *
      Serial.println("connection failed. Cannot check OTA.");
      return false;
    }
    client.print(String("GET ") + otaCheckUpdateFile + " HTTP/1.1\r\n" +
                 "Host: " + otaURL + "\r\n" + 
                 "Connection: close\r\n\r\n" );                                 
    Serial.print("check update request sent to OTA Server:");
    Serial.println(otaURL);
    while (client.connected()) {                              // ignore HTTP header content
      if(client.available()){
        c = char(client.read());
        if ( c == '{') 
          break;
      }
    }
    result = "";
    result = result + c;                                      // result = "{ ... }"
    while (client.connected()) {                              
      if(client.available()){
        c = char(client.read());
        result = result + c;              //如果没有char()转换，出来的都是HEX
        if (c == '}') 
          break;
      }
    }     
    Serial.println(result);
    Serial.println("closing connection");
    client.stop();
    //change Global Variable : needOTAcheck = false, 和Server检查成功，只需要检查一次OTA就好，不用在loop()里循环做多次
    needOTAcheck = false;
    // analyze and extract data from "result" string
    tempStr = "";
    i = result.indexOf("otaVersion");                   //extract otaVersion
    if( i < 0){
      Serial.println("Error: Cannot find otaVersion -- new firmware version.Exit otaCheck().");
      return false;
    }
    else{
      j = result.indexOf(':', i);           
      i = result.indexOf('"', j)+1;
      j = result.indexOf('"', i);
      tempStr=result.substring(i,j);
      tempStr.trim();
      newOtaVersion = tempStr;
      Serial.print("newOtaVersion=");
      Serial.println(newOtaVersion);
      if (Version == newOtaVersion){
        Serial.println("Version numbers match. No OTA updates available");
        return false;
      }
      else{
        Serial.println("New version avaiable to update.");
        ESPhttpUpdate.rebootOnUpdate(false);
        Serial.println("Set ESPhttpUpdate.rebootOnUpdate = false");  
      }            
    }
    tempStr="";                                        //extract firmware update availability
    i = result.indexOf("otaFirmware");
    if (i < 0){
      Serial.println("Error: Not finding otaFirmware.Exit otaCheck().");
      return false;
    }
    else{
      j = result.indexOf(':', i);           
      i = result.indexOf('"', j) + 1;
      j = result.indexOf('"', i);
      tempStr=result.substring(i,j);
      tempStr.trim();
      Serial.print("otaFirmware=");
      Serial.println(tempStr);
      if (tempStr == "0"){
        Serial.println("No firmware available for update.");        
      }
      if (tempStr == "1"){                                    // 只更新 Firmware
        Serial.println("Make firmware update available");
        firmwareNeedUpdate = true;
        firmwareUpdated = false;        
      }      
    }

    tempStr="";                                        //extract FileSystem update availability
    i = result.indexOf("otaFileSys");
    if (i < 0){
      Serial.println("Error: Not finding otaFileSys.Exit otaCheck().");
    }
    else{
      j = result.indexOf(':', i);           
      i = result.indexOf('"', j) + 1;
      j = result.indexOf('"', i);
      tempStr=result.substring(i,j);
      tempStr.trim();
      Serial.print("otaFileSys=");
      Serial.println(tempStr);
      if (tempStr == "0"){
        Serial.println("No File System available for update.");        
      }
      if (tempStr == "1"){                                    // 只更新 File System
        Serial.println("Make SPIFFS:File System update available");
        spiffsNeedUpdate = true;          // 只更新 SPIFFS   
        spiffsUpdated = false;       
      }      
    }

    tempStr="";                                        //extract otaURL availability
    i = result.indexOf("otaURL");
    if (i < 0){
      Serial.println("Error: Not finding otaURL.Exit otaCheck().");
    }
    else{
      j = result.indexOf(':', i);           
      i = result.indexOf('"', j) + 1;
      j = result.indexOf('"', i);
      tempStr=result.substring(i,j);
      tempStr.trim();
      otaURL = tempStr;
      Serial.print("otaURL=");
      Serial.println(otaURL);
    }

    tempStr="";                                        //extract otaFirmwareFile
    i = result.indexOf("otaFirmwareFile");
    if (i < 0){
      Serial.println("Error: Not finding otaFirmwareFile name.Exit otaCheck().");
    }
    else{
      j = result.indexOf(':', i);           
      i = result.indexOf('"', j) + 1;
      j = result.indexOf('"', i);
      tempStr=result.substring(i,j);
      tempStr.trim();
      otaFirmwareName = tempStr;
      Serial.print("otaFirmware=");
      Serial.println(otaFirmwareName);
    }

    tempStr="";                                        //extract otaFileSysFile 
    i = result.indexOf("otaFileSysFile");
    if (i < 0){
      Serial.println("Error: Not finding otaFileSysFile.Exit otaCheck().");
    }
    else{
      j = result.indexOf(':', i);           
      i = result.indexOf('"', j) + 1;
      j = result.indexOf('"', i);
      tempStr=result.substring(i,j);
      tempStr.trim();
      otaFileSysName = tempStr;
      Serial.print("otaFileSystem =");
      Serial.println(otaFileSysName);
    }        

    tempStr="";                                        //extract otaPortNumber 
    i = result.indexOf("otaPortNum");
    if (i < 0){
      Serial.println("Error: Not finding otaPortNum.Exit otaCheck().");
    }
    else{
      j = result.indexOf(':', i);           
      i = result.indexOf('"', j) + 1;
      j = result.indexOf('"', i);
      tempStr=result.substring(i,j);
      tempStr.trim();
      otaPortNum = tempStr.toInt();
      Serial.print("otaPortNum =");                   // blank is converted to 0
      Serial.println(otaPortNum);            
    } 
  } 
  else
    Serial.println("Not connected to Router to access Internet. Cannot conduct OTA check.");

  Serial.println("otaCheck(): OTA availability check ends");
  
}

void updateFirmware(){
  String otaFileLocation;
  if(firmwareNeedUpdate==true && firmwareUpdated==false){
    ESP.wdtFeed();
    Serial.println("Red and Blue LEDs blinking");
    os_timer_disarm(&blueLEDtimer); 
    os_timer_disarm(&redLEDtimer); 
    digitalWrite(BlueLEDpin,LOW);          // 亮蓝色LED
    digitalWrite(RedLEDpin, LOW);          // 亮红色LED
    os_timer_arm(&redLEDtimer, 500, 1 );   //每500ms执行一次redLEDtimer的callback函数，即redLEDblink(); 
    os_timer_arm(&blueLEDtimer,500, 1 );   //每500ms执行一次blueLEDtimer的callback函数，即blueLEDblink();
    unsigned long startTime = millis();
    Serial.println("Start to update firmware");
    ESP.wdtDisable(); //下载时间比较长，需要暂时关闭 watch dog
    otaFileLocation = otaURL + "/" + otaFirmwareName;
    Serial.print("OTA file online address: ");
    Serial.println(otaFileLocation);    
    t_httpUpdate_return ret = ESPhttpUpdate.update(otaFileLocation);
    //t_httpUpdate_return ret = ESPhttpUpdate.update("http://dev.met-light.com/A_Global_Async_OTA.bin");
    //t_httpUpdate_return ret = ESPhttpUpdate.update("http://update.zobox.co/A_Global_v2.bin");
    //如下是侯名的新加坡服务器地址：
//    Serial.println("Download firmware from HouMing's server ...");
//    t_httpUpdate_return ret = ESPhttpUpdate.update("http://192.168.1.16", "7000", "/A_Global_Async_OTA.bin");    
//    Serial.println("Download firmware from http://47.91.90.170:8080/A_Global_v2.bin ...");
//    t_httpUpdate_return ret = ESPhttpUpdate.update("http://47.91.90.170", "8080", "/A_Global_v2.bin"); 
    ESP.wdtEnable(WDTO_8S);  
    ESP.wdtFeed();
    switch(ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\r\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        Serial.println(">Firmware updated");
        Serial.printf("Time (in seconds) used: %ld\r\n", (millis()-startTime)%1000);
        firmwareUpdated = true;   
        firmwareNeedUpdate=false;              
       break;
    } 
    os_timer_disarm(&redLEDtimer);    //取消每500ms执行一次redLEDtimer的callback函数，即redLEDblink(); 
    os_timer_disarm(&blueLEDtimer);   //取消每500ms执行一次blueLEDtimer的callback函数，即blueLEDblink();  
    digitalWrite(BlueLEDpin,HIGH);    // Turn off Blue LED
    digitalWrite(RedLEDpin, HIGH);    // Turn off Red LED                 
  }
}

void updateSPIFFS(){
  String otaFileLocation;
  if(spiffsNeedUpdate==true && spiffsUpdated==false){
    ESP.wdtFeed();
    Serial.println("Terminate SPIFF via SPIFFS.end()");
    SPIFFS.end();
    Serial.println("Blue and Red LED blinking");
    os_timer_disarm(&blueLEDtimer); 
    os_timer_disarm(&redLEDtimer); 
    digitalWrite(BlueLEDpin,LOW);          // 亮蓝色LED
    digitalWrite(RedLEDpin, LOW);          // 亮红色LED
    os_timer_arm(&redLEDtimer, 500, 1 );    //每500ms执行一次redLEDtimer的callback函数，即redLEDblink(); 
    os_timer_arm(&blueLEDtimer,500, 1 );    //每500ms执行一次blueLEDtimer的callback函数，即blueLEDblink();      
    unsigned long startTime = millis();
    Serial.println("Start to update SPIFFS");
    ESP.wdtDisable(); //下载时间比较长，暂时关闭 watch dog 
    otaFileLocation = otaURL + "/" + otaFileSysName;
    Serial.print("OTA file online address: ");
    Serial.println(otaFileLocation);
    t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(otaFileLocation); 
    //t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs("http://dev.met-light.com/A_Global_Async_v3_FS.bin"); 
    //t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs("http://192.168.1.16","7000", "/A_Global_Async_spiffs.bin");
    ESP.wdtEnable(WDTO_8S);                   // WatchDog timer is set to 8 seconds
    ESP.wdtFeed();
    Serial.println(">SPIFFS updated");
    Serial.printf("Time (in seconds) used: %ld\r\n", (millis()-startTime)%1000);
    switch(ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\r\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        spiffsUpdated = true;     
        spiffsNeedUpdate = false;           
      break;
    } 
    os_timer_disarm(&redLEDtimer);    //取消每500ms执行一次redLEDtimer的callback函数，即redLEDblink(); 
    os_timer_disarm(&blueLEDtimer);   //取消每500ms执行一次blueLEDtimer的callback函数，即blueLEDblink();  
    digitalWrite(BlueLEDpin,HIGH);    // Turn off Blue LED
    digitalWrite(RedLEDpin, HIGH);    // Turn off Red LED                    
  }  
}

/* Save config.js and router.js content to EEPROM
 * After OTA, rebuild config.js and reouter.js, user do not need to re-configure Rebooter
 */
bool saveConfigToEEPROM (){
  int address = 0;
  byte c; 
  Serial.println("Save config.js and router.js entire file to EEPROM");
  Serial.println("Allocate 2048 bytes (2KB) for two files in EEPROM sector");
  Serial.println("config.js : 1K bytes, router.js : 1K bytes");
  EEPROM.begin(2048); 
  Serial.println("Begin Erasing EEPROM ...");
  for(address=0; address<2047; address++){
    EEPROM.write(address,0); 
  }
  EEPROM.commit();                    //这句话将变化写回EEPROM中    
  Serial.println("Erase process done.");   
  Serial.println("Set EEPROM address 0 content = 1: mark data saved and needs to be extracted when boot");
  Serial.println("Set EEPROM address 1 content = *: seperator");
  address = 0;
  EEPROM.write(address, '1');         //标志位是'1'，HEX = 49
  address ++;
  EEPROM.write(address, '*');         //分隔符
  address ++;                         //
  Serial.println("Begin saving config.js to EEPROM from address=2");   
  File configFile = SPIFFS.open("/scripts/config.js", "r");  
  if(configFile){
    if(configFile.size() > 1023){
      Serial.println("ERROR: config.js file size > 1024 bytes. Exit.");
      return false;
    }    
    while(configFile.available()){
      c = configFile.read(); 
      EEPROM.write(address,c);
      address ++;    
      if (address >= 1023) break;   
    } 
    configFile.close();
    Serial.println("config.js file saved to EEPROM and closed.");      
  }
  else{
    Serial.println("Failure: cannot open config.js file.");
    return false;
  }
  EEPROM.write(address,'*');                  // File seperator 
  address ++;
  File routerFile = SPIFFS.open("/scripts/router.js", "r");           //Save router.js to EEPROM
  if(routerFile){
    if(routerFile.size()> 1023){
      Serial.println("ERROR: router.js file size > 1023. Exit.");
      return false;
    }
    while(routerFile.available()){
      c = routerFile.read(); 
      EEPROM.write(address,c);
      address ++;    
      if (address >= 2047 ) break;               
    } 
    routerFile.close();
    Serial.println("router.js file saved to EEPROM and closed.");  
  }
  else{
    Serial.print("Failed to open router.js file to Read");
    return false;
  }
  EEPROM.write(address,'*');  
  EEPROM.commit();
  EEPROM.end();                         //结束 EEPROM 的操作
  return true; 
  Serial.println("Successfully saved config.js and router.js variable value to EEPROM");
}

bool restoreConfigFromEEPROM(){
  int address = 2;                        
  byte c; 
  Serial.println("Read value from EEPROM and rebuild config.js and router.js files");
  Serial.println("EEPROM address = 0 content: 0/1 indicates whether there is data");
  Serial.println("EEPROM address = 1 content: *   indicates it is a content seperator");
  Serial.println("EEPROM config content starts from address = 2");
  Serial.println("Rebuild config.js ...");
  EEPROM.begin(2048); 
  File configFile = SPIFFS.open("/scripts/config.js", "w");         // rebuild this file, 'w'
  address = 2;                    // EEPROM ruturns to beginning location
  if(configFile){    
    while(address <= 1023){
      c = EEPROM.read(address); 
      if (char(c)=='*') break;
      configFile.print(char(c));
      address ++;       
    } 
    configFile.close();
    Serial.println("config.js file re-created.");      
  }
  else{
    Serial.println("Failure: cannot create config.js file.");
    return false;
  }   
  address = address + 1;                      // skip this file seperator '*'
  File routerFile = SPIFFS.open("/scripts/router.js", "w");
  if(routerFile){
    while(address<=2047){
      c = EEPROM.read(address); 
      if (char(c)=='*') break;              // stop at file terminator '*'
      routerFile.print(char(c));
      address ++;                  
    } 
    routerFile.close();
    Serial.println("router.js file re-created.");  
  }
  else{
    Serial.print("Failure: cannot create router.js file.");
    return false;
  }    
  Serial.println("EEPROM: set address 0 content to 0: avoid repeatitive restoration");
  EEPROM.write(0,0);
  EEPROM.commit();
  EEPROM.end();
  return true;
  Serial.println("Extracted data from EEPROM and successfully rebuild config.js and router.js files");
}


/* 这个函数虽然编译没有问题，但是不可以工作，仍旧出现设备重启, 这是因为不能和AsyncWebServer一同使用，SSL占用的内存太多了！
bool sendEmail(){
  const char* user_base64 = "aHVnb0BtZXQtbGlnaHQuY29t"; // exmail.qq.com  uername hugo@met-light.com
  const char* user_password_base64 = "cHVmYTIwMTM="; // exmail.qq.com  password
  const char* from_email = "MAIL From:<hugo@met-light.com>";  
  const char* to_email = "RCPT To:<hugo@met-light.com>";  
  byte respCode;
  byte thisByte;

  WiFiClientSecure client;        // This definition is very Critical for using base64 SSL login,不能和AsynWebServer同时使用

  Serial.println("Executing sendEmail...");
  if (client.connect("smtp.exmail.qq.com", 465) == 1) {        
    Serial.println("connected to smtp.exmail.qq.com");
  } 
  else {
    Serial.println("connection to smtp.exmail.qq.com failed");
    return false;
  }
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }
    
  Serial.println("--- Sending EHLO---"); 
  client.println("EHLO smtp.exmail.qq.com");  // 下面一行等同的IP地址也可以的
  //client.println("EHLO 182.254.34.125");   // smtp.exmail.qq.com, port #465
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }

  Serial.println("--- Sending login request AUTH---"); 
  client.println("AUTH LOGIN"); 
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }

  Serial.println("--- Sending Username base64---"); 
  client.println(user_base64); 
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }   

  Serial.println("--- Sending Password base64---"); 
  client.println(user_password_base64); 
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  } 
  
  Serial.println("--- Sending From---"); 
  client.println(from_email); 
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }
  
  Serial.println("--- Sending To---"); 
  client.println(to_email); 
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }

  Serial.println("--- Sending DATA---"); 
  client.println("DATA"); 
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }
  
  client.println("Subject: Esp8266 email test\r\n");   
  client.println("Congratulations! Hugo, You can now send email via ESP8266.  \r\nWiFisecureClient \n"); 
  client.println(".");                    //结束DATA的方式是 354 End data with <CR><LF>.<CR><LF>
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }
  Serial.println("--- Sending QUIT---"); 
  client.println("QUIT");                                   // = client.write("QUIT\r\n"); 
  //if (!eRcv()) return false;
  respCode = client.peek();
  while (client.available()) {
    thisByte = client.read();
    Serial.write(thisByte);
  }
  client.stop();
  Serial.println("disconnected");
  return 1;
}
*/
