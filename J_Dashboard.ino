
void dashboardPageLoad(AsyncWebServerRequest *request) {
  Serial.println("\nDashboard.html page-load()");   
  ESP.wdtFeed();   
  request->send(SPIFFS, "/dashboard.html");                     
  Serial.println("End of dashboard page-load");
}

void dashboardLoadConfig(AsyncWebServerRequest *request) {
  String tempStr;
  Serial.println();
  Serial.println("In dashboardLoadConfig...");  
  ESP.wdtFeed();  
  if(connectedToRouter){
    IPAddress ip;
    ip = WiFi.localIP();
    if (pingEnable)
      tempStr = "{\"code\":\"1\",\"data\":{\"status\":\"1\",\"routerSSID\":\"" + routerSSID + "\", \"localIP\":\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\",";       
    else
      tempStr = "{\"code\":\"1\",\"data\":{\"status\":\"0\",\"routerSSID\":\"" + routerSSID + "\", \"localIP\":\"" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]) + "\",";     
  }  
  else
    tempStr = "{\"code\":\"0\",\"data\":{\"status\":\"0\",\"routerSSID\":\"\",\"localIP\":\"\","; 
  // loadConfig() 从 config.js 中获取各个参数的数字
  if(loadConfig()){
    tempStr = tempStr + "\"rebootTime\":\"";
    if(userSetRebootHour < 10)
      tempStr = tempStr + "0" + String(userSetRebootHour);
    else
      tempStr = tempStr + String(userSetRebootHour);
    tempStr = tempStr + ":";
    if(userSetRebootMinute < 10)
      tempStr = tempStr + "0" + String(userSetRebootMinute);
    else
      tempStr = tempStr + String(userSetRebootMinute);
    tempStr = tempStr + "\",\"rebootSet\":\"";
    if(userSetReboot == true)
      tempStr = tempStr + "1\",\"turnoffAPTime\":\"";
    else
      tempStr = tempStr + "0\",\"turnoffAPTime\":\"";
    /**********************************************************    
    if(int(turnoffAPTime/3600) < 10)                                         // hours
      tempStr = tempStr + "0" + String(int(turnoffAPTime/3600));
    else
      tempStr = tempStr + String(int(turnoffAPTime/3600));
    tempStr = tempStr + ":";
    if(int((turnoffAPTime%3600)/60) < 10)                                   // minutes
      tempStr = tempStr + "0" + String(((turnoffAPTime%3600)/60));
    else
      tempStr = tempStr + String(((turnoffAPTime%3600)/60));   
    **********************************************************/
    tempStr = tempStr + turnoffAPTime/60;         
    tempStr = tempStr + "\",\"turnoffAPSet\":\"";
    if(turnoffAPSet == true)
      tempStr = tempStr + "1\"}}";
    else
      tempStr = tempStr + "0\"}}";
  }

  Serial.print("Data string prepared:");
  Serial.println(tempStr);
  request->send(200,"application/json", tempStr);
  Serial.println("Exit dashboardLoadConfig...");
}

void dashboardPageSubmit(AsyncWebServerRequest *request) {
  IPAddress ip;
  Serial.println("\nDashboard page Submit()");
  ESP.wdtFeed();
  if (request->hasArg("status")){
    Serial.print("Server founds Status switch action");
    if (request->arg("status")=="0") {                               // dashboard显示没有连接上Router状态
      request->send(200, "text/plain", "{\"code\":\"0\"}"); 
      Serial.println("\nStop ping check. Turn off Wi-Fi monitoring action.");
      //stop Ping check   
      pingEnable = false;
      condition = 0;
      Serial.println("Blue LED blink");
      os_timer_disarm(&blueLEDtimer); 
      os_timer_arm(&blueLEDtimer, 500, 1 );       // Blue LED blink starts      
      //write to router.js file to save status
      updateWiFiStatusFile('0');  
      //write to /history.js log;
      saveToHistoryLogFile("","User switch Off monitoring");            
    }
    else{  //开启Ping监控
      Serial.println("\nEnable ping check. Turn on Wi-Fi monitoring action.");
      // 1: 成功登录，在 dashboard.html 中的 AJAX 中定义的,跳转到 /quick.html
      if (connectedToRouter){
        ip = WiFi.localIP();
        request->send(200, "text/plain", "{\"code\":\"1\", \"routerSSID\":\"" + routerSSID + "\"}");  
        //bring IP address to UI
        //request->send(200,"text/plain","{\"code\":\"1\", \"routerSSID\":\"" + routerSSID + "\",\"acquiredIP\":\" + String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3])+ "\"}");
        Serial.println("Blue LED solid on");
        os_timer_disarm(&blueLEDtimer);
        digitalWrite(BlueLEDpin, LOW);       // Blue LED constantly lights up  
        os_timer_disarm(&redLEDtimer);
        digitalWrite(RedLEDpin, HIGH);       // turn off Red LED
        pingEnable = true;
        dnsCounter=0; PTC=0; RC=0;
        pingCheck();    
        //write to router.js file to save status
        updateWiFiStatusFile('1');       
        //write to history.js log file;
        saveToHistoryLogFile("","User switch On monitoring");                                                                                  
      }
      else{
        Serial.println("Error: please select a Wi-Fi SSID and enter a valid password.");   
        request->redirect("/quick.html");  
      }
    }
  }
  
  if(request->hasArg("time")){
    Serial.print("Server founds Daily Reboot Time setting data.");
    String userInputTime;
    userInputTime = request->arg("time");
    Serial.printf("User input time is: %s\r\n", userInputTime.c_str());
    if(userInputTime == "") 
      userInputTime = "00:00";                               // need to change this 
    userSetRebootHour = (userInputTime[0] - char('0'))* 10 + (userInputTime[1] - char('0'));   
    userSetRebootMinute = (userInputTime[3] - char('0')) * 10 + (userInputTime[4] - char('0'));
    Serial.printf("Converted time is: %d:%d\r\n",userSetRebootHour,userSetRebootMinute);  
    // save to config.js
    File configFile = SPIFFS.open("/scripts/config.js", "r+");                                
    if(!configFile){
      Serial.println();
      Serial.print("Failed to open /scripts/config.js file to Write: ");
      Serial.println(configFile);                
    }
    else{
      configFile.seek(0,SeekSet); // beginning of file
      int i=0; 
      while(configFile.available()){
        configFile.seek(1,SeekCur);
        if(configFile.peek() == ',') 
          i++; 
        if(i == 4) 
          break;                   
      }
      while(configFile.available()){
        configFile.seek(1,SeekCur);
        if(configFile.peek() == ':') 
          break;   
      }
      configFile.seek(2,SeekCur);
      configFile.print(request->arg("time"));                
      configFile.close();
      Serial.println("Save Dashboard/user set reboot Time to /scripts/config.js completed.");  
      checkFile("/scripts/config.js");                
    }          
  }

  if(request->hasArg("isRebootOpen")){
    Serial.print("Server founds Reboot at Time switch condition.");
    if(request->arg("isRebootOpen") == "1"){
      Serial.println("User set reboot at above time: ON");
      userSetReboot = true;
    }
    else{
      Serial.println("User set reboot at above time: OFF");  
      userSetReboot = false;
    }      
    // Save to  config.js 
    File configFile = SPIFFS.open("/scripts/config.js", "r+");                                
    if(!configFile){
      Serial.println();
      Serial.print("Failed to open /scripts/config.js file to Write: ");
      Serial.println(configFile);                
    }
    else{
      configFile.seek(0,SeekSet); // beginning of file
      int i=0; 
      while (configFile.available()){
        configFile.seek(1,SeekCur);
        if(configFile.peek() ==',') 
          i++; 
        if(i==4) 
          break;                   
      }   
      configFile.seek(-2,SeekCur);
      configFile.print(request->arg("isRebootOpen"));                
      configFile.close();
      Serial.println("Save Dashboard/user set Reboot configuration to /scripts/config.js completed.");  
      checkFile("/scripts/config.js");                
    }              
  }    
  
  if(request->hasArg("turnoffAPTime")){
    Serial.print("Server founds turnoffAPTime setting data.");
    String userInputTime;
    userInputTime = request->arg("turnoffAPTime");
    Serial.printf("User input turnoff AP time in minutes is: %s\r\n", userInputTime.c_str());
    turnoffAPTime = userInputTime.toInt() * 60;                
    Serial.printf("Converted turnoffAPTime in seconds is: %d\r\n",turnoffAPTime);  
    //save to config.js
    saveVarToConfig("turnoffAPTime", userInputTime);
/*    
    File configFile = SPIFFS.open("/scripts/config.js", "r+");                                
    if(!configFile){
      Serial.println();
      Serial.print("Failed to open /scripts/config.js file to Write: ");
      Serial.println(configFile);                
    }
    else{
      configFile.seek(0, SeekSet); // beginning of file
      int i = 0; 
      while(configFile.available()){
        configFile.seek(1,SeekCur);
        if(configFile.peek() == ',') i++; 
        if(i == 6) break;                   
      }
      while(configFile.available()){
        configFile.seek(1, SeekCur);
        if(configFile.peek() == ':') 
          break;   
      }
      configFile.seek(2, SeekCur);
      configFile.print(request->arg("turnoffAPTime"));                
      configFile.close();
      Serial.println("Save Dashboard/user set turnoffAPTime to /scripts/config.js completed.");  
      checkFile("/scripts/config.js");                
    }
    */          
  }
        
  if (request->hasArg("turnoffAPSet")){
    Serial.println("Server found turnoffAPSet switch condition");       
    if(request->arg("turnoffAPSet") == "1"){
      Serial.println("turnoffAPSet = 1");                
      turnoffAPSet = true;         
    }
    else{
      turnoffAPSet = false;
      Serial.println("turnoffAPSet = 0");
    }
    // Save to the config.js file
    File configFile = SPIFFS.open("/scripts/config.js", "r+");                                
    if(!configFile){
      Serial.println();
      Serial.print("Failed to open /scripts/config.js file to Write: ");
      Serial.println(configFile);                
    }
    else{
      configFile.seek(0, SeekSet); // beginning of file
      int i=0; 
      while (configFile.available() ){
        configFile.seek(1, SeekCur);
        if(configFile.peek() ==',') i++; 
        if(i == 6) break;                   
      }   
      configFile.seek(-2, SeekCur);
      configFile.print(request->arg("turnoffAPSet"));                
      configFile.close();
      Serial.println("Save Dashboard/user set turnoffAPSet switch configuration to /scripts/config.js completed.");  
      checkFile("/scripts/config.js");                
    }      
  }

  /*  5/15/2018
   *  增加如下的OTA检查，在这里设置标志位，在 loop() 中执行检查，保存结果到变量里，
   *  这样用户在进入 /advanced 页面后，点击 OTA Update 时，可以直接看到检查的结果，不用等待
   *  这也是被AsyncWebServer逼的，因为在 AsyncWebServer 运行时，WiFiClient 无法运行
   */
  if (request->hasArg("otaCheck")){
      Serial.print("Server find otaCheck = "); 
      Serial.println(request->arg("otaCheck"));
      Serial.println("Set needOTACheck = true to enable OTA check in loop()");
      request->send(200,"text/json", "{\"code\":\"0\",\"data\":\"\"}");
      needOTAcheck = true;
  }
  else{
      Serial.print("Server does NOT find otaCheck value "); 
      Serial.println("Set needOTACheck = false to disable OTA check in loop()");
      request->send(200,"text/json", "{\"code\":\"0\",\"data\":\"\"}");
      needOTAcheck = false;      
  }
    
  Serial.println("END of handling Dashboard page Submit()\n");
}


