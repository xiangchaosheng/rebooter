void connectToRouter(String strSSID, String strPassword){
  IPAddress ip;
  unsigned long timeout;
  ESP.wdtFeed();
  Serial.println("--- connectToRouter() starts ---");
  routerSSID = strSSID;
  routerPassword = strPassword;
  //判断目前是否已经连接到用户指定的路由器上
  ip = WiFi.localIP();
  if(ip[0]!=0 && WiFi.SSID() == routerSSID){          //ie. IP !=0, 说明现在已经连接在用户指定的路由器上了，否则，IP=0.0.0.0, 没有成功连接到路由器WiFi上
    Serial.println("Already connected to target router Wi-Fi.");
    connectedToRouter = true;
    connecting = false;
  }
  else{                                            // 连接到用户指定的路由器Wi-Fi上
    connectedToRouter = false;
    if((routerSSID.length()>0) && (routerPassword.length()>=0)){
      Serial.println("Attempt to connect to user defined Wi-Fi.");
      Serial.print("connectAttemptCounter = ");
      connectAttemptCounter ++;
      Serial.println(connectAttemptCounter);
      WiFi.softAPdisconnect(true);
      WiFi.disconnect();
      dnsCaptivePortalReady = false;
      // turnoffAPTime = 0 means turn off AP immediately
      if(turnoffAPSet==true && turnoffAPTime==0){
        Serial.println("turnoffAPset = true && turnoffAPTime =0. Connect to Router as STA mode only.");
        WiFi.mode(WIFI_STA);
        apTurnedOff = true;
      }
      else{
        Serial.println("Connect to Router as AP_STA mode.");
        WiFi.mode(WIFI_AP_STA);                // 连接到Router上，AP_STA模式，一段时间后，关掉AP，只做STA模式：3/16/2018
        createAPprefix();            
        WiFi.softAP(AP_Prefix.c_str(), AP_password.c_str(),1, false, 1);   // Zobox-ABCD     或者 wifi_station_set_hostname("Zobox-ABCD");
        Serial.println("Started: Soft AP with password");
        Serial.print(AP_Prefix); Serial.print("//"); Serial.println(AP_password);
        apTurnedOff = false;  
        //Captive Portal starts for AP
        Serial.println("Starting DNS Captive Portal for AP...");
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(53,"*", apIP);   
        dnsCaptivePortalReady = true;
      }
      Serial.println("Connecting to user defined Router with SSID//password...");
      Serial.print(strSSID);Serial.print("//"); Serial.println(strPassword);    
      WiFi.hostname(myHostName.c_str());       // 在局域网内，启用mDNS后通过http://zobox.local 方式访问 
      WiFi.begin(strSSID.c_str(), strPassword.c_str());
      connStartTime = millis();                             //启动计时器
      connecting = true;   
      ESP.wdtFeed();    
      //mDNS 服务开启
      Serial.println("Start mDNS service.");
      Serial.print("mDNS name setting - myHostName:"); Serial.println(myHostName);
      if(MDNS.begin(myHostName.c_str())){      // http://zobox.local/
        MDNS.addService("http", "tcp", 80);
        //MDNS.notifyAPChange();
        //MDNS.update();
        Serial.println("mDNS started");           
      }
      else   
        Serial.println("mDNS fail to start.");                                       
    } 
    else
      connecting = false;     
  }
  Serial.println("Status is:");
  Serial.print("connectedToRouter = ");
  if(connectedToRouter)
    Serial.print(" true; ");
  else
    Serial.print(" false; ");
  Serial.print("connecting = ");
  if(connecting)
    Serial.println("true;");
  else
    Serial.println("false;");
  Serial.println("--- connectToRouter() END ---");
}

void saveRouterWiFi(String strSSID, String strPassword, char state){        // 保存路由器Wi-Fi信息
  Serial.println("Save Router Wi-Fi credentials and status ...");
  File routerFile = SPIFFS.open("/scripts/router.js", "w");
  if(!routerFile){
    Serial.println();
    Serial.print("Failed to open Router Wi-Fi credential file to Write: ");
    Serial.println(routerFile);  
  } 
  else{
    routerFile.print("var myRouter= {\n");                                  //    var myRouter = {
    routerFile.print("\"routerSSID\":\"");                                  //    "routerSSID":"router-wifi-ssid",
    routerFile.print(strSSID);                                              //      "routerPassword":"topsecret",
    routerFile.print("\",\n");                                              //     "Status": 1
    routerFile.print("\"routerPassword\":\"");                              //      }
    routerFile.print(strPassword);
    routerFile.print("\",\n");                                              // 这个println产生的换行符就是读取字符串时产生的不可见字符
    routerFile.print("\"Status\":\"");                                      //  connected：1， NOT connected：0
    routerFile.print(state);                                                //  pingEnable=true: state=1;  otherwise, pingEnable=false: state=0;
    routerFile.print("\"\n}");   
    Serial.println("Saved Router credentials and status to /scripts/router.js");
  }
  routerFile.close();
  Serial.println("Save Router Wi-Fi credential file completed.");          
}

void disconnectWiFi(){                                        // 断开与路由器Wi-Fi的连接，更新 router.js 中的status
  Serial.println("Disconnect with Router Wi-Fi");
  WiFi.disconnect();                                     // AP 也不服务了
  connectedToRouter = false;
  updateWiFiStatusFile('0');                             // router.js 中 status 变更
  Serial.println("Disconnect Router Wi-Fi procedure completed.");          
}

void updateWiFiStatusFile(char state){      
  Serial.println("Only updates Status in /scripts/router.js ...");
  //Serial.println("Before Update");
  //checkFile("/scripts/router.js");      
  File routerFile = SPIFFS.open("/scripts/router.js", "r+");    //仅改变 status
  if(!routerFile){
    Serial.println();
    Serial.print("Failed to open Router Wi-Fi credential file to Write: ");
    Serial.println(routerFile);  
  } 
  else{
    routerFile.seek(0, SeekEnd);
    while(routerFile.position()>0 && routerFile.peek()!=':'){
      routerFile.seek(-1,SeekCur);
    }
    routerFile.seek(2,SeekCur);
    //routerFile.print(":\"");
    routerFile.print(state);
    //routerFile.print("\n}");
    Serial.println("Changed Router status in /scripts/router.js");
  }
  routerFile.close();
  Serial.println("After update");
  checkFile("/scripts/router.js");  
  Serial.println("Only updates Status in /scripts/router.js END");                              
}

bool loadRouterWiFi(){
  String tempStr; 
  int i,j;
  Serial.println("Load Router Wi-Fi credentials ...");
  File routerFile = SPIFFS.open("/scripts/router.js", "r");
  if(!routerFile){
    Serial.println();
    Serial.print("Failed to open Router Wi-Fi credential file to Read: ");
    Serial.println(routerFile);  
    return false;
  } 
  else{
    while(routerFile.available()){
      tempStr += char(routerFile.read());         // 不能用 readStringUntil(), 与 AsyncWebServer冲突
    }
    routerFile.close();          
    tempStr.trim();
    Serial.print("Read file content:");
    Serial.println(tempStr);
    i = tempStr.indexOf(":");                   //定位到第1个：,注意：在字符串中有不可见的\n存在
    tempStr = tempStr.substring(i+2);           //跳过双引号 tempStr = ChinaNet_tz4i","Password":"secretPassword", "Status":0 }  
    j = tempStr.indexOf('\"');                  // 找到双引号
    routerSSID = tempStr.substring(0, j);       // routerSSID = ChinaNet_tz4i
    tempStr = tempStr.substring(j+1);           // tempStr = ,"Password":"secretPassword","Status":0 }
    i = tempStr.indexOf(":");                   
    tempStr = tempStr.substring(i+2);           // tempStr = secretPassword", "Status":0}
    j = tempStr.indexOf('\"');
    routerPassword = tempStr.substring(0, j);   // routerPassword = secretPassword  
    tempStr = tempStr.substring(j+2);
    i = tempStr.indexOf(":");                   // tempStr = "Status":"0" }
    tempStr = tempStr.substring(i+2, i+3);            
    if(tempStr=="0")                            // Status 记录 pingEnable 的状态                         
      pingEnable = false;
    else
      pingEnable = true;
    Serial.print("Acquired Router credentials: ");
    Serial.print(routerSSID);
    Serial.print("//");
    Serial.println(routerPassword);  
    Serial.print("Read pingEnable setting to be:");
    Serial.println(tempStr);                    
  }
  Serial.println("Leave loadRouterWiFi()");
  if (routerSSID.length()>0 && routerPassword.length()>0)
    return true;
  else
    return false;
}

void pingCheck(){
  IPAddress ip, tempIP;
  Serial.println("--- pingCheck() starts ---");
  Serial.print("ESP Free Heap Size in bytes:");
  Serial.println(ESP.getFreeHeap());
  ESP.wdtFeed();
  //判断目前是否已经连接到用户指定的路由器上
  ip = WiFi.localIP();
  Serial.print("dnsCounter = "); 
  Serial.println(dnsCounter);
  tempIP = arrayDNS[dnsCounter];
  Serial.println("Current local time:"); 
  DisplayTimeToSerial();
  Serial.println("Current local IP:" + String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3]));
  Serial.println("Try to Ping DNS:" + String(tempIP[0])+"."+String(tempIP[1])+"."+String(tempIP[2])+"."+String(tempIP[3]));
  /*  功能改进：6/21/2018
   *  如果路由器的Wi-Fi信号消失时，也能够重启路由器
   */
  if(ip[0]!=0 || (routerSSID.length()>0)){         // ip[0]!=0 说明已经连接到路由器Wi-Fi上, connectedToRouter 变量的结果
    if(ip[0]!=0)  
        Serial.println("Connected to Router Wi-Fi");
    if(ip[0] == 0 && (routerSSID.length()>0))
        Serial.println("Not connected to router Wi-Fi. But router SSID exists. Follow the same ping failure routine.");        
    os_timer_disarm(&blueLEDtimer); 
    digitalWrite(BlueLEDpin, LOW);               
    Serial.print("Ping DNS: " + String(tempIP[0])+"."+String(tempIP[1])+"."+String(tempIP[2])+"."+String(tempIP[3]) + " ");          
    if (Ping.ping(tempIP, 3)){                  // PING 3 次都没有超时, 正常，等30秒后，再PING下一个DNS              
      Serial.println("Success.");
      Serial.print("Average Time:");
      Serial.println(Ping.averageTime());
      PTC=0;
      RC=0;
      os_timer_disarm(&redLEDtimer);           // Red LED stops blinking  
      digitalWrite(RedLEDpin, HIGH);           //熄灭红色LED
      condition = 1;
      pingTimer = millis();
      dnsCounter++;                             // dnsCounter=0~4, which are 5 IPs. 
      if(dnsCounter>4 || dnsCounter>(4+userDefinedDNSqty)) 
        dnsCounter = 0;
    }                                           
    else{                                       // PING 超时了
      Serial.println("Failure.");
      Serial.print("Average Time:");
      Serial.println(Ping.averageTime());              
      saveToHistoryLogFile(String(tempIP[0])+"."+String(tempIP[1])+"."+String(tempIP[2])+"."+String(tempIP[3]), "Ping fail");
      Serial.println("Blink Red LED");
      os_timer_arm(&redLEDtimer,500,1);       //Red LED blinks      
      PTC++;                                  // 记录Ping超时次数的计数器 PTC + 1
      Serial.print("PingTimeoutCounter = ");
      Serial.println(PTC);
      if (PTC <= 3){                            // Ping 超时3次以内，立即Ping下一个DNS即可
        Serial.print("BEFORE  dnsCounter = "); 
        Serial.println(dnsCounter);
        dnsCounter++;
        if (dnsCounter>4 || dnsCounter>(4+userDefinedDNSqty)) 
          dnsCounter = 0;
        Serial.print("AFTER   dnsCounter = "); 
        Serial.println(dnsCounter);
        condition = 2;
      }
      else{
        RC++;
        Serial.print("Reboot Counter = ");
        Serial.println(RC);                  
        if(RC <= 3){                                  // 路由器连续重启次数 <= 3 次，继续重启路由器
          Serial.println("Reboot router.... save reboot history log");
          saveToHistoryLogFile("", "Reboot router");                          //上文已经记录Ping的Failure，这里只记录Reboot，不再记录DNS的IP
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
          condition = 3;
          pingTimer = millis();                 // 开始5秒钟的计时，5秒后把路由器的电源恢复
        }
        else{                                       // 路由器连续重启次数 3 次，不再监控，休息15分钟
          //PTC=0;
          //RC=0;
          Serial.println("* * * Rebooted router 3 consecutive times. Now starts giving Router 15 minutes recovery time and then try again.* * *");
          Serial.print("Assign condition = 5");
          condition = 5;
          pingTimer = millis();
        }
      }            
    }
  }
  else{
    condition = 0;                          //没有连接上路由器
    connectedToRouter = false;   
    os_timer_disarm(&blueLEDtimer); 
    digitalWrite(BlueLEDpin, HIGH);         //熄灭蓝色LED   
    os_timer_disarm(&redLEDtimer);      
    digitalWrite(RedLEDpin, LOW);           // 红色LED 亮
    Serial.println("No connection to Router. No Ping.");
    if ((routerSSID.length() > 0) && (routerPassword.length() >= 0)){
      // 如果用户之前有配置过路由器Wi-Fi，那么尝试连接，然后开始监控路由器 
      if(connectedToRouter==false && connecting == false)    
        connectToRouter(routerSSID, routerPassword);   //acquire time included in this process 
    }
  }   
  Serial.println("---- pingCheck() completed ---- ");      
}


