
void loop(){
  //Feed watchDog
  ESP.wdtFeed();  
  if(dnsCaptivePortalReady)  
    dnsServer.processNextRequest();

  //如果还在连接中的状态
  if(connectedToRouter==false && connecting == true){   
    //Serial.println("in loop: connectedToRouter==false && connecting == true");
    if(millis() < (connStartTime + ConnectionTimeOut)){
      //Serial.println("Still trying to connect to Router Wi-Fi.");
      if(WiFi.status() == WL_CONNECTED){          
        Serial.println("\nConnect to Router Wi-Fi: Succss");
        Serial.println("Turn off Red LED and blink Blue LED");
        os_timer_disarm(&redLEDtimer);          //结束AP状态红色LED闪烁
        digitalWrite(RedLEDpin, HIGH);          // red LED: turn off 
        os_timer_disarm(&blueLEDtimer);         
        os_timer_arm(&blueLEDtimer, 500, 1 );   // blueLED 开始闪烁，当开始PING之后solid         
        Serial.print("Connected to Router with SSID:"); 
        Serial.println(WiFi.SSID());
        Serial.print("STA mode IP address:"); 
        Serial.println(WiFi.localIP());
        connectedToRouter = true; connecting = false;
        Serial.print("Soft AP mode IP address:"); Serial.println(WiFi.softAPIP());
        Serial.print("Reset connectAttemptCounter = ");
        connectAttemptCounter = 0;
        Serial.println(connectAttemptCounter);        
        //server.send(200, "text/plain", "connect to router wifi success"); 增加判断是否有Client连接                               
        Serial.println("Call saveRouterWiFi() function from within loop()"); 
        if (pingEnable)       // router.js 中的 "Status":"1" 表示 pingEnable
          saveRouterWiFi(routerSSID, routerPassword, '1');    
        else
          saveRouterWiFi(routerSSID, routerPassword, '0');           
        checkFile("/scripts/router.js");                               
        //captive portal service 停止
        //dnsServer.stop();            dnsCaptivePortalReady=false;
        //Serial.println("Captive Portal service stopped for STA mode.");                                
        Serial.println("Try to acquire time.");               
        ESP.wdtFeed();
        //没获得有效时间
        if (timeAcquired == false){
          timeZoneDSTacquired==false;         // 强制调用去URL获取时区和时间，6/20/2018
          acquireTime();    
          adjustTime();
          DisplayTimeToSerial();    
        }                              
        if (pingEnable){     
          // 开始监控          
          PTC=0;           // Ping Timeout Count
          pingCheck();     //内有判断是否连接上路由器Wi-Fi
          os_timer_disarm(&blueLEDtimer);         // 停止蓝色LED闪烁，更改为蓝色LED常亮
          digitalWrite(BlueLEDpin, LOW);          // 蓝色LED（监控路由器状态灯）：长亮  
        }                                                                    
      }
    }     
    else{         
      // connection to STA timeout, reverse to softAP mode
        Serial.println("Connect to Router Wi-Fi: Failure due to connection timeout.");
        Serial.print("connectAttemptCounter = ");
        Serial.println(connectAttemptCounter);
        if (connectAttemptCounter <= Max_Connection_Attempt){
            Serial.println("connectAttemptCounter <= Max_Connection_Attempt, try to run connecToRouter() again");
            connectToRouter(routerSSID, routerPassword);
        }
        else{
            Serial.println("connectAttemptCounter > Max_Connection_Attempt, going to Delete SSID/Password and Restart.");
            connectedToRouter = false;                //连接到路由器Wi-Fi错误，重新恢复 AP 模式，仅用captive portal即可，不用 mDNS
            connecting = false;
            os_timer_disarm(&blueLEDtimer);          //结束蓝色LED闪烁
            digitalWrite(BlueLEDpin, HIGH);          // 蓝色 LED 熄灭 
            os_timer_arm(&redLEDtimer, 500, 1 );     // 红色 LED 闪烁
            digitalWrite(RedLEDpin, LOW);            //       
            //saveRouterWiFi("", "", '1');           // Deletes current failure connection router credentials 
            Serial.println("Restart ESP, along with SPIFFS and AsyncWebServer setup ....");
            ESP.restart();
        }
    }            
  }

  /* 3/16/2018
      在既定时间后，关掉AP
  */
  if(connectedToRouter && turnoffAPSet && apTurnedOff == false){
    if((millis() > connStartTime + turnoffAPTime*1000)){
      WiFi.softAPdisconnect(true);   // 关掉AP,只留下 STA
      apTurnedOff = true;
      Serial.println("--- * ---");
      Serial.println("AP turned off.");
      Serial.print("STA IP is still:");
      Serial.println(WiFi.localIP());
      Serial.print("WiFi.getMode() [ 0=OFF / 1=STA / 2=AP / 3=AT_STA ]: ");
      Serial.println(WiFi.getMode()); 
      Serial.println("--- * ---");           
    }
  }
  // End of content added at 3/16/2018
               
  if(connectedToRouter && timeAcquired==false){
    //acquireTimezoneAndDaylightSavingSetting();
    acquireTime();
    adjustTime();
    DisplayTimeToSerial();                         
  }
    
  if (pingEnable){
    // if(pingEnable && connectedToRouter) 这个判断是不行的，因为有其他功能借用了这段程序
    switch (condition){
      case 0:                          // 没有连接上路由器，或者路由器中间故障了
        //Serial.println("condtion=0: not connected to Router or Router failure.");                          
      break;
      case 1: 
        //Serial.print("1");
        if(millis() > (pingTimer + PingInterval*1000)) {    // Ping 成功时，default间歇是30秒  
          Serial.println();
          pingCheck(); 
          Serial.println();                          
        }
        if(timeAcquired == false) 
          acquireTime();  //尝试再次获取时间                                                              
      break;
      case 2: 
        Serial.println("Loop(), condition=2");      // Ping 失败次数<3,立即Ping下一个DNS                              
        pingCheck();
      break;
      case 3:
        Serial.print("Loop(), condition=3, WiFi.Status[3:connected, 4:connection failed, 6:disconnected] = "); 
        Serial.println(WiFi.status());              
        Serial.print("PTC=");   Serial.print(PTC);      Serial.print(" ,RC=");      Serial.println(RC);
        if(millis() > (pingTimer + PowerInterval*1000)){     // 5 秒钟后恢复给路由器的供电，实现路由器重启
          digitalWrite(PowerControlGPIOA, LOW);             // 恢复路由器的电源
          digitalWrite(PowerControlGPIOB, LOW);             // 恢复路由器的电源                            
          Serial.println("Loop(): *** Power back ON Router ***");
          //os_timer_arm(&redLEDtimer, 500, 1 );              // Red LED flashes
          //write to history log
          saveToHistoryLogFile("","Router power restore");
          pingTimer = millis();                             // 重新开始计时，2 分钟， 等路由器恢复自身的上网功能
          condition = 4;
        }
        else{
          os_timer_disarm(&redLEDtimer);     
          digitalWrite(RedLEDpin,LOW);                      //点亮红色LED
        }
      break;
      case 4:
        Serial.print("Loop(), condition=4, WiFi.Status = ");  
        Serial.println(WiFi.status());                  
        Serial.print("PTC="); Serial.print(PTC);   Serial.print(" RC=");   Serial.println(RC);
        if(millis() > (pingTimer + RebootInterval*1000)){    // 给路由器RebootInterval=120秒钟=2分钟时间，完成自己的重新启动并连接到网络
            Serial.println("Reboot interval time is up. Now try to connect to Router Wi-Fi...");                        
            PTC=0;   
            // 根据
            if( presetRebootInProgress == true){
                Serial.println("restore pingEnable value");
                presetRebootInProgress = false;
                if (pingRestore == false){
                  pingEnable = false;
                  pingRestore = true;               // default                  
                }
                else{     
                  pingEnable = true;
                }
            }
            if(connectedToRouter==false && (routerSSID.length() > 0) && (routerPassword.length() >= 0)){
              Serial.println("2 minutes is up, start connectToRouter() again.");
              condition = 0;
              WiFi.disconnect();
              connectToRouter(routerSSID, routerPassword);  
            }              
          }
          else{
            os_timer_disarm(&redLEDtimer);     
            os_timer_arm(&redLEDtimer, 500, 1 );    //AP mode: 每500ms执行一次redLEDtimer的callback函数，即redLEDblink()                  
          }            
//            ESP.restart();
        
/*  Replaced the following code with ESP.restart() as David claims he could not connects to office Router      
        if(millis() > (pingTimer + RebootInterval*1000)){       // 给路由器RebootInterval=120秒钟=2分钟时间，完成自己的重新启动并连接到网络       
          if(connectedToRouter==false){
            WiFi.disconnect();
            connectToRouter(routerSSID, routerPassword);  
          }                        
          PTC=0;   
          if (pingRestore == false){
            pingEnable = false;
            pingRestore = true;               // default
            presetRebootInProgress = false;
          }
          else     
            pingCheck();    
        }
        else{
          os_timer_disarm(&redLEDtimer);     
          os_timer_arm(&redLEDtimer, 500, 1 );    //AP mode: 每500ms执行一次redLEDtimer的callback函数，即redLEDblink()                  
        }
*/      
      break; 
      case 5:
        Serial.println("Loop(), condition=5");
        acquireTime();
        Serial.print("PTC="); 
        Serial.print(PTC); 
        Serial.print(" RC="); 
        Serial.println(RC);
        if (millis() > (pingTimer + RestInterval*1000)){      //连续重启3次路由器后，暂停监控15分钟=900秒
          Serial.println("* * * Router rest time is over. Now try to connect to Router again. * * *"); 
          Serial.println("set condition=0; PTC=0; RC=0");
          condition = 0;         
          PTC=0;
          RC=0;
          WiFi.disconnect();
          if((routerSSID.length() > 0) && (routerPassword.length() >= 0)){
              connectToRouter(routerSSID, routerPassword);    
          }                                                 
        }
        else{
          os_timer_disarm(&blueLEDtimer); 
          digitalWrite(BlueLEDpin, HIGH);       //关闭蓝色LED
          os_timer_disarm(&redLEDtimer); 
          digitalWrite(RedLEDpin,LOW);          //点亮红色LED                        
        }
      break;
    }      
  } //end of pingEnable
  ESP.wdtFeed();
  /* Add reboot Router at defined time: 每天在固定时间重启路由器，无论 PING 是否工作
     userSetReboot: from config.js file when calling loadConfig()
     这是为了借用上文的 switch (pingEnable)的 condition=3 的重启功能，
     用 pingRestore 来记录当前的 pingEnable 的状态
  */
  if((userSetReboot == true) && (presetRebootInProgress == false)){     
    if(timeAcquired){
      adjustTime();
      if(checkTime4Reboot(currentHour, currentMinute, currentSecond, userSetRebootHour, userSetRebootMinute)){
        Serial.println("\r\nTime to R E B O O T\r\n");
        presetRebootInProgress = true;      // 避免重复执行本段代码
        if(pingEnable == false){
          pingRestore = false;
          pingEnable = true;
        }              
        Serial.println("Reboot router.... save reboot history log");
        saveToHistoryLogFile("", "Reboot router: Reboot Daily timer effect");            
        digitalWrite(PowerControlGPIOA, HIGH); // 切断路由器的电源
        digitalWrite(PowerControlGPIOB, HIGH); // 切断路由器的电源
        Serial.println("*** Power off Router ***");
        WiFi.disconnect();
        connectedToRouter = false;
        connecting = false;
        os_timer_disarm(&blueLEDtimer); 
        digitalWrite(BlueLEDpin,HIGH);         // turn off 蓝色LED    
        os_timer_disarm(&redLEDtimer); 
        digitalWrite(RedLEDpin, HIGH);         // turn off Red LED                  
        condition = 3;                         // 借用上文的 switch（pingEnable)来实现计时2分钟后恢复路由器供电
        pingTimer = millis();                 // 开始5秒钟的计时，5秒后把路由器的电源恢复                         
      }
    }     
  }   
   
  // OTA check
  if(connectedToRouter && needOTAcheck){
    Serial.println("in loop(), going to check OTA...");
    otaCheck();     // needOTAcheck 在 otaCheck() 函数中变更了状态为 false 
  }
  // OTA: 根据用户在 advanced.html 页面的选项，决定 userPermitOTA 变量，来是否进行OTA 
  if (connectedToRouter && (firmwareNeedUpdate||spiffsNeedUpdate) && userPermitOTA){        //只是连接到路由器，并不能保证能够访问Internet 
    if(firmwareNeedUpdate) {
      Serial.println("in loop(), going to update firmware ...");
      updateFirmware();
      Serial.print("Save the new version # ");
      Serial.print(newOtaVersion);
      Serial.println(" to config.js");
      saveVarToConfig("Version", newOtaVersion);
    }
    else
      Serial.println("No Firmware need to be updated.");
    if(spiffsNeedUpdate){
      Serial.println("in loop(), going to 1) save config to EEPROM; 2) update File System; 3) restore config from EEPROM ...");
      saveConfigToEEPROM();
      updateSPIFFS();
      restoreConfigFromEEPROM();
    }
    else
      Serial.println("No SPIFFS need to be updated.");
   
    if(firmwareUpdated || spiffsUpdated){
      Serial.println("OTA update completed. Going to reboot ......");
      ESP.restart();      
    }            
  }  
} // end of loop()


