/* Wi-Fi AP 的名字，Zoboot-ABCD 形式 */
void createAPprefix(){
  String tempStr; 
  Serial.println("Trying to create Zoboot-ABCD prefix");
  ESP.wdtFeed();
  if (AP_Prefix.length() == myHostName.length() + 1){              // AP_Prefix = "zoboot-", myHostName="zoboot", 两者长度就差1个字符-
    tempStr = WiFi.softAPmacAddress();                         // 如果长度差异>1, 就表明已经是 zoboot-ABCD 形式了，不用再添加变成 zoboot-ABCDABCD
    tempStr.toUpperCase();
    //mac address=5E:CF:7F:02:06:1C，从0开始，第12位是06的0
    tempStr = tempStr.substring(12);  //取子字符串，从第12位到末尾，即 06:1C
    tempStr.remove(2,1);              //去掉 06:1C 中的冒号:
    AP_Prefix = AP_Prefix + tempStr;        
  }  
  Serial.print("AP prefix is: ");
  Serial.println(AP_Prefix);
}

void startSoftAP_DNSCaptivePortal(){
  Serial.println("\r\nStart - softAP and DNS captive portal");
  ESP.wdtFeed();
  WiFi.softAPdisconnect(true);
  //WiFi.disconnect();
  createAPprefix();
  WiFi.hostname(myHostName.c_str());  
  WiFi.mode(WIFI_AP);              
  //只允许一个客户连接到AP热点上                           
  // WiFi.softAP(AP_Prefix.c_str(), NULL, 1, false, 1);  
  WiFi.softAP(AP_Prefix.c_str(), AP_password.c_str(), 1, false, 1);  
  Serial.println("Started: Soft AP with password");
  Serial.print(AP_Prefix); Serial.print("//"); Serial.println(AP_password);  
  //Captive Portal starts
  Serial.println("Starting DNS Captive Portal ...");
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53,"*", apIP);   
  dnsCaptivePortalReady = true;
/* 5/31/2018  
  Serial.println("Only updates status:0 in /router.js");
  updateWiFiStatusFile('0');                   // router.js 中 status 变更 0: 没有连接上Router
  checkFile("/scripts/router.js");     
*/  
  Serial.println("Started - softAP and DNS captive portal.\r\n");
}

/* 只允许 1 个Client连接到ESP的AP上  */
void setSingleAPclient(){
  struct softap_config conf;
  ESP.wdtFeed();
  wifi_softap_get_config(&conf);
  conf.max_connection = 1;
  ETS_UART_INTR_DISABLE();
  if(wifi_softap_set_config_current(&conf))
    Serial.println("Successfully changed to Current max AP client # = 1");
  else{
    Serial.println("Error: Fail to change Current max AP client # = 1");
  }
  ETS_UART_INTR_ENABLE();
}

/* 打印文件内容 */
bool checkFile (const char* FilePath){
  char c;
  Serial.print("Printing content of file: ");
  Serial.println(FilePath);
  
  File configFile = SPIFFS.open(FilePath, "r");
  if(configFile){
    while(configFile.available()){
      c = configFile.read();        
      Serial.print(c);
    }
    configFile.close();
    Serial.println();
    return true;
  }
  else{
    Serial.print("check fail Error. Failed to open the file ");
    Serial.print(FilePath);
    Serial.println(" for read.");
    return false;
  }
}

/*  扫描ESP周边的Wi-Fi热点，按照信号强度排序，
    并保存入 scripts/mySSID.js 文件中，
    并在串口打印显示出来 */
void scanSaveSSID(){
  int m = WiFi.scanNetworks();
  ESP.wdtFeed();
  Serial.println("Scan Wi-Fi you may want to connect to. ");
  if(m == 0){
    Serial.println("NO Wi-Fi networks found.");
    File ssidFile = SPIFFS.open("/scripts/mySSID.js", "w+");
    if (ssidFile){
      Serial.println("Open mySSID.js to Write success");
      ssidFile.println("loadSSID ([");  
      ssidFile.println("])");
      ssidFile.close();       
      Serial.println("Write 0 SSID to mySSID.js file.");
      /* ------------- 需要实地测试上述功能 -----------------*/      
    } 
    else
      Serial.println("Fail to write 0 SSIDs to mySSID.js file.");       
    return;
  }
  else{
    int n;
    if(m < 5){                                                    //如果仅找到 <5 个Wi-Fi热点，再尝试一次
      Serial.print("Only found ");
      Serial.print(n);
      Serial.println(" Wi-Fi networks. n < 5. Try again.");
      delay(500);                                                 // 延迟在此，试图发现更多的Wi-Fi
      ESP.wdtFeed();
      n = WiFi.scanNetworks();
      if(n == 0){
        Serial.println("NO Wi-Fi networks found.");
        File ssidFile = SPIFFS.open("/scripts/mySSID.js", "w+");
        if (ssidFile){
          Serial.println("Open mySSID.js to Write success");
          ssidFile.println("loadSSID ([");  
          ssidFile.println("])");
          ssidFile.close();       
          Serial.println("Write 0 SSID to mySSID.js file.");
          /* ------------- 需要实地测试上述功能 -----------------*/      
        } 
        else
          Serial.println("Fail to write 0 SSIDs to mySSID.js file.");       
        return;
      }
    }
    else{
      n = m;
    }
    int i, j, tempInt, ssidIndex[n];
    long rssiArray[n], tempLong;

    Serial.print("----------");
    Serial.print(n);
    Serial.println(" Wi-Fi networks found ---------- ");                
    memset(ssidIndex, 0, sizeof(ssidIndex));
    memset(rssiArray, 0, sizeof(rssiArray));
    for (i=0; i<n; i++){
      ssidIndex[i] = i; 
      rssiArray[i] = WiFi.RSSI(i);
    }
    //bubble sort based on RSSI 
    for(i=0; i<n; i++){
      for(j=i+1; j<n; j++){
        if(rssiArray[i] < rssiArray[j]){
          tempLong = rssiArray[i];
          rssiArray[i] = rssiArray[j];
          rssiArray[j] = tempLong;
          tempInt = ssidIndex[i];
          ssidIndex[i] = ssidIndex[j];
          ssidIndex[j] = tempInt;
        }
      }
    }

    // save info to mySSID.js file 
    Serial.println("check mySSID.js file existance ... ");
    if(SPIFFS.exists("/scripts/mySSID.js")){
      Serial.println("/scripts/mySSID.js exists. Delete its content. ");            
    }
    else
      Serial.println("ERROR: /scripts/mySSID.js does not exist. Create it.");  
    File ssidFile = SPIFFS.open("/scripts/mySSID.js", "w+");    
    if(ssidFile){
      Serial.println("Open mySSID.js to Write: success");
      ssidFile.println("loadSSID ([");         //必须匹配这个 loadSSID () 函数在 register.html 中的JS函数名
      for(i=0; i<n; i++){
        ssidFile.print("{");
        ssidFile.print("\"serial\":\"");      //增加了 serial# 
        ssidFile.print(ESP.getChipId());
        ssidFile.print("\",");
        ssidFile.print("\"ssid\":\"");        //都用小写字母，Server端的PHP是大小写敏感的
        ssidFile.print(WiFi.SSID(ssidIndex[i]));
        ssidFile.print("\",");
        ssidFile.print("\"rssi\":");
        ssidFile.print(WiFi.RSSI(ssidIndex[i]));
        ssidFile.print(",");
        ssidFile.print("\"bssid\":\"");
        ssidFile.print(WiFi.BSSIDstr(ssidIndex[i]));
        ssidFile.print("\",");
        ssidFile.print("\"channel\":");
        ssidFile.print(WiFi.channel(ssidIndex[i]));                
        if(i <= n-1)  
          ssidFile.print("},");  
        else        
          ssidFile.print("}");            
        ssidFile.print("\r\n");              //必须有这个 \n,否则在读取文件时会在 readUntil('\n')会出现溢出错误
      }            
      ssidFile.println("])");
      ssidFile.close();
      Serial.println("Write SSIDs to mySSID.js file success.");  
    }
    else 
      Serial.println("Open mySSID.js Failure");            
  }
  Serial.println("Completed scan and save SSID function.");        
}

/* 显示文件系统中的文件 */
void showSPIFFcontent(){
  Serial.println("Display SPIFFS structure ----------");
  if(SPIFFS.begin()){
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()){    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, Size: %d bytes\n", fileName.c_str(), fileSize);
    }
    Serial.printf("SPIFFS structure displayed.----------\n");
  }
}

// 闪烁 blue LED 的 callback 函数
void blueLEDblink(void*) {                                
  digitalWrite(BlueLEDpin, !digitalRead(BlueLEDpin));  
}

// 闪烁 red LED 的 callback 函数
void redLEDblink(void*) {                                
  digitalWrite(RedLEDpin, !digitalRead(RedLEDpin));  
}

/* Dec. 12/6/2017
 * This function is working. But I cannot notfiy /quick.html page that already showing on client phone when SSID.js has been updated
 * This function is NOT used. Safe to DELETE ! 
 */
void asyncScanSaveSSID(int networksFound) {
  Serial.println("Async Scan Wi-Fi you may want to connect to. ");
  Serial.print("n= "); Serial.println(networksFound);
  ESP.wdtFeed();
  int n=networksFound;
  if(n == 0){
    Serial.println("NO Wi-Fi networks found.");
    File ssidFile = SPIFFS.open("/scripts/mySSID.js", "w+");
    if(ssidFile){
      Serial.println("Open mySSID.js to Write success");
      ssidFile.println("loadSSID ([");  
      ssidFile.println("])");
      ssidFile.close();       
      Serial.println("Write 0 SSID to mySSID.js file.");                
    } 
    else
      Serial.println("Fail to write 0 SSIDs to mySSID.js file.");       
    return;
  }
  else{
    Serial.print("----------");
    Serial.print(n);
    Serial.println(" Wi-Fi networks found ---------- ");             
    int i, j, tempInt, ssidIndex[n];
    long rssiArray[n], tempLong;     
    for(i=0;i<n;i++)
      Serial.println(WiFi.SSID(i));
          
    memset(ssidIndex, 0, sizeof(ssidIndex));
    memset(rssiArray, 0, sizeof(rssiArray));
    for(i=0; i<n; i++){
      ssidIndex[i] = i; 
      rssiArray[i] = WiFi.RSSI(i);
    }
    //bubble sort based on RSSI 
    for (i=0; i<n; i++){
      for (j=i+1; j<n; j++){
        if (rssiArray[i] < rssiArray[j]){
          tempLong = rssiArray[i];
          rssiArray[i] = rssiArray[j];
          rssiArray[j] = tempLong;
          tempInt = ssidIndex[i];
          ssidIndex[i] = ssidIndex[j];
          ssidIndex[j] = tempInt;
        }
      }
    }
    // save info to mySSID.js file 
    Serial.println("check mySSID.js file existance ... ");
    if (SPIFFS.exists("/scripts/mySSID.js")){
      Serial.println("/scripts/mySSID.js exists. Delete its content. ");            
    }
    else
      Serial.println("ERROR: /scripts/mySSID.js does not exist. Create it.");  
    File ssidFile = SPIFFS.open("/scripts/mySSID.js", "w+");    
    if (ssidFile) {
      Serial.println("Open mySSID.js to Write: success");
      ssidFile.println("loadSSID ([");         //必须匹配这个 loadSSID () 函数在 register.html 中的JS函数名
      for (i=0; i<n; i++){
        ssidFile.print("{");
        ssidFile.print("\"serial\":\"");      //增加了 serial# 
        ssidFile.print(ESP.getChipId());
        ssidFile.print("\",");
        ssidFile.print("\"ssid\":\"");        //都用小写字母，Server端的PHP是大小写敏感的
        ssidFile.print(WiFi.SSID(ssidIndex[i]));
        ssidFile.print("\",");
        ssidFile.print("\"rssi\":");
        ssidFile.print(WiFi.RSSI(ssidIndex[i]));
        ssidFile.print(",");
        ssidFile.print("\"bssid\":\"");
        ssidFile.print(WiFi.BSSIDstr(ssidIndex[i]));
        ssidFile.print("\",");
        ssidFile.print("\"channel\":");
        ssidFile.print(WiFi.channel(ssidIndex[i]));                
        if(i <= n-1)  
          ssidFile.print("},");  
        else        
          ssidFile.print("}");            
        ssidFile.print("\r\n");              //必须有这个 \n,否则在读取文件时会在 readUntil('\n')会出现溢出错误
      }            
      ssidFile.println("])");
      ssidFile.close();
      Serial.println("Write SSIDs to mySSID.js file success.");       
    }
    else 
      Serial.println("Open mySSID.js Failure");            
  }     
  Serial.println("Completed scan and save SSID function.");        
}


