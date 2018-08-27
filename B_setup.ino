
void setup() {
  pinMode(PowerOnGPIO, OUTPUT);
  digitalWrite(PowerOnGPIO, HIGH);      // GPIO-13 保持高电平，持续给ESP供电      
  Serial.begin(115200);
  Serial.println();
  Serial.println("------------------------------------->|<-------------------------------");
  Serial.println(" Zobox Rebooter program last reviewed on 6/21/2018");
  Serial.println(" - Async WebServer Lite v.1.1 -");
  Serial.println("------------------------------------->|<-------------------------------");  
  Serial.setDebugOutput(false);
  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);                 // GPIO-15 保持低电平    
  pinMode(PowerControlGPIOA, OUTPUT);   
  pinMode(PowerControlGPIOB, OUTPUT);
  pinMode(BlueLEDpin, OUTPUT);
  pinMode(RedLEDpin, OUTPUT);
  digitalWrite(PowerControlGPIOA, LOW);   //default:导通供电
  digitalWrite(PowerControlGPIOB, LOW);   //default:导通供电
  digitalWrite(BlueLEDpin,HIGH);          //熄灭蓝色LED
  digitalWrite(RedLEDpin, HIGH);          //熄灭红色LED   
  ESP.wdtEnable(WDTO_8S);                   // WatchDog timer is set to 8 seconds
  ESP.wdtFeed();
  delay(50); 
  digitalWrite(RedLEDpin, LOW);          //点亮红色LED  
  // 各GPIO控制端口状态初始化
  pinMode(ButtonPressGPIO, INPUT);      // 用中断方式读取 GPIO-5 的状态，判断是关机，还是 Reset
  //等待用户抬起按键完成开机
  while(1){
    ESP.wdtFeed();
    delay(20);
    Serial.println("Waiting power on end!");
    if(digitalRead(ButtonPressGPIO) == HIGH)
      button_power_on_cnt++;
    else
      button_power_on_cnt = 0;
    if(button_power_on_cnt >= 10)
      break;
  }
  attachInterrupt(digitalPinToInterrupt(ButtonPressGPIO), handleButtonPressInterrupt, FALLING);     //核心语句
  interrupt_trigger_mode = FALLING_MODE;       
  digitalWrite(BlueLEDpin,HIGH);          //熄灭蓝色LED
  digitalWrite(RedLEDpin, HIGH);          //熄灭红色LED   

  os_timer_setfn(&blueLEDtimer, (os_timer_func_t*) &blueLEDblink, 0); 
  os_timer_setfn(&redLEDtimer,  (os_timer_func_t*) &redLEDblink,  0);
  os_timer_arm(&redLEDtimer, 500, 1 );    //AP mode: 每500ms执行一次redLEDtimer的callback函数，即redLEDblink()       
  Serial.print("ESP Serial# :"); 
  Serial.println(ESP.getChipId());
  Serial.print("ESP core version :");
  Serial.println(ESP.getCoreVersion());
  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  if(SPIFFS.begin()){
    Serial.println("SPIFFS mount success.");            
    showSPIFFcontent();
  }
  else{
    Serial.println("Error(10): SPIFFS mount failure. Restart in 3 seconds");      
    ErrorNo = 10;
    // Flash error lights in the 3 seconds, 用指示灯显示错误
    //另一种处理方法：继续往下走，在login.html中提供SPIFFS错误，提醒用户重新更新固件
    //在 loop() 中检测，如果 开关按钮超过10秒钟，自动更新SPIFFS和Firmware
    delay(3000);
    ESP.restart();
  }   
  /*  6/6/2018 addition:
   *  Read data from EEPROM, check if OTA was performed and need to restore config.js and router.js from EEPROM
   *  EEPROM address 0 content=1: YES, need to restore data from EEPROM
   *  EEPROM address 0 content=0: NO,  no need to restore data from EEPROM
   */
  
  EEPROM.begin(128);
  byte otaFlag;
  otaFlag = EEPROM.read(0);
  EEPROM.end();
  if(char(otaFlag) == '1'){
    Serial.println("\nOTA performed. Need to restore config.js and router.js configuration from EEPROM\n");
    restoreConfigFromEEPROM();
  }
  else
    Serial.println("\nOTA check: otaFlag in EEPROM indicates restore configuration is NOT necessary.\n");
   
  //读取PING的参数, /scripts/config.js
  //如果读取失败，使用变量定义初始化时的Default值
  Serial.println("start loading config.js...");
  if (loadConfig())
    Serial.println("Load config from config.js success.");
  else
    Serial.println("Load config from config.js Failure. Use default values.");
  
  Serial.println();
    
  /* 为 QuickSetup 页面做准备*/    
  Serial.println("Scan for SSIDs & show quick.html");
  scanSaveSSID();
  checkFile("/scripts/mySSID.js"); 

  ESP.wdtFeed();
  
  // 载入 router.js 的信息，
  checkFile("/scripts/router.js");
  Serial.println("Set connectAttemptCounter = 0");
  connectAttemptCounter = 0;
  Serial.println("Load router.js.and ......");   
  loadRouterWiFi();
  if((routerSSID.length() > 0) && (routerPassword.length() >= 0)){
    // 如果用户之前有配置过路由器Wi-Fi，那么尝试连接，然后开始监控路由器
    Serial.println("Still in setup(), Router SSID and Password acquired. ConnectToRouter()....");    
    connectToRouter(routerSSID, routerPassword);   //acquire time included in this process 
  }
  else{
    Serial.println("Still in setup(), Router SSID and Password length = 0, startSoftAP_DNSCaptivePortal()...");
    startSoftAP_DNSCaptivePortal();     
  }
  Serial.print("WiFi.getMode() [ 0=OFF / 1=STA / 2=AP / 3=AP_STA ]: ");
  Serial.println(WiFi.getMode());
  
  Serial.println("\nAsync Web Server processing"); 
  server.serveStatic("/images", SPIFFS, "/images", "max-age=86400");     // 86400秒 = 24 小时
  server.serveStatic("/styles", SPIFFS, "/styles","max-age=86400");      // 客户端缓存CSS等网页格式信息
  server.serveStatic("/scripts", SPIFFS, "/scripts");                    // 客户端不缓存配置信息   

  switch (languageChoice){
      case  "English":
            Serial.println("Language selection from SPIFF: ENGLISH");
            server.serveStatic("/", SPIFFS, "/", "max-age=86400").setDefaultFile("welcome.html");
            break;
      case  "French":
            Serial.println("Language selection from SPIFF: French");
            server.serveStatic("/", SPIFFS, "/French/", "max-age=86400").setDefaultFile("welcome.html"); 
            break;
      caes  "German":
            Serial.println("Language selection from SPIFF: German");
            server.serveStatic("/", SPIFFS, "/German", "max-age=86400").setDefaultFile("welcome.html");
            break;
      default:
            Serial.println("Default Language selection from SPIFF: ENGLISH");
            server.serveStatic("/", SPIFFS, "/", "max-age=86400").setDefaultFile("welcome.html");            
  }
/*  
  if (languageChoice == "French"){
      Serial.println("Language selection from SPIFF: French");
      server.serveStatic("/", SPIFFS, "/French/", "max-age=86400").setDefaultFile("welcome.html");          
  }
  else {
      Serial.println("Language selection from SPIFF: ENGLISH");
      server.serveStatic("/", SPIFFS, "/", "max-age=86400").setDefaultFile("welcome.html");      
  }
  */
  server.on("/",           HTTP_POST,    welcomePageSubmit);
  server.on("/welcome",    HTTP_POST,    welcomePageSubmit);
  server.on("/security",   HTTP_POST,    securityPageSubmit);
  server.on("/quick",      HTTP_POST,    quickSetupSubmit);
  server.on("/dashboard",  HTTP_POST,    dashboardPageSubmit);
  server.on("/dashboard_load", HTTP_GET, dashboardLoadConfig);         //3/30/2018
  server.on("/history",    HTTP_POST,    historyPageSubmit);           //4/17/2018
  server.on("/help",       HTTP_POST,    helpPageSubmit);
  server.on("/advanced",   HTTP_POST,    advancedPageSubmit);
  server.on("/reboot",     HTTP_POST,    rebootPageSubmit);    
  server.on("/connect",    HTTP_POST,    connectPageSubmit);  
  server.on("/webOTA",     HTTP_POST,    webOTAPageSubmit); 
  
  server.onNotFound(welcomePageLoad);    
  server.begin();
  ESP.wdtFeed();
  Serial.println("Async Web Server started. ");          

  // 这个时间是 24小时格式，到23：59：59后，更新为 00：00：00 
  if(connectedToRouter){
    time(&currentTime);                     // 从 L_time.ino 中的 acquireTime()更改到这里
    Serial.println("Config Time offset parameter and shcedule time-update");
    configTime(timeZone * 3600, daylightSavingTime * 3600, "pool.ntp.org", "time.nist.gov");
    os_timer_setfn(&timeUpdateTimer, (os_timer_func_t*)&acquireTime, 0);
    os_timer_arm(&timeUpdateTimer, timeUpdateInterval, 1 );   
  }         
  // 初始化其他变量
  needOTAcheck = false;
  userPermitOTA = false; 
  Serial.println("\n-------------End of Setup------------\r\n");  
}
