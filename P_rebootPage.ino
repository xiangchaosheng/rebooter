/* reboot Router, not reboot ESP8266 */
void rebootPageLoad(AsyncWebServerRequest *request){
  Serial.println("\nReboot page-Load()");  
  ESP.wdtFeed();
  request->send(SPIFFS, "/reboot.html");
  Serial.println("End: reboot page-Load");
}

void rebootPageSubmit(AsyncWebServerRequest *request){
  Serial.println("\nReboot page Submit()");  
  Serial.println("Reboot router.... save reboot history log");
  ESP.wdtFeed();
  saveToHistoryLogFile("", "Router power off");                         //上文已经记录Ping的Failure，这里只记录Reboot，不再记录DNS的IP
  digitalWrite(PowerControlGPIOA, HIGH); // 切断路由器的电源
  digitalWrite(PowerControlGPIOB, HIGH); // 切断路由器的电源
  Serial.println("*** Power off Router ***");
  os_timer_disarm(&blueLEDtimer); 
  os_timer_disarm(&redLEDtimer); 
  digitalWrite(BlueLEDpin,HIGH);         // turn off 蓝色LED    
  digitalWrite(RedLEDpin, HIGH);         // turn off Red LED  
  os_timer_arm(&redLEDtimer, 500, 1 );    //AP mode: 每500ms执行一次redLEDtimer的callback函数，即redLEDblink() 
  WiFi.disconnect(); 
  connectedToRouter = false;
  connecting = false;
  pingRestore = pingEnable;             // Save pingEnable status to pingRestore
  pingEnable = true;                    // this is to borrow the code from PING 
  condition = 3;
  pingTimer = millis();
  Serial.println("End: reboot page Submit()");
}

