/* 用户在通电情况下，长按开关超过了3秒钟，触发了Reset */

void handleReset(){
  File file;
  Serial.println("\n-----*-----Handling RESET process -----*-----");
  Serial.println("Fast blink red and blue LED together");
  ESP.wdtFeed();
  os_timer_disarm(&redLEDtimer);
  os_timer_disarm(&blueLEDtimer);
  digitalWrite(BlueLEDpin,LOW);          // 亮蓝色LED
  digitalWrite(RedLEDpin, LOW);          // 亮红色LED
  os_timer_arm(&redLEDtimer, 300, 1 );   //每300ms执行一次redLEDtimer的callback函数， 即redLEDblink(); 
  os_timer_arm(&blueLEDtimer,300, 1 );   //每300ms执行一次blueLEDtimer的callback函数，即blueLEDblink();
  digitalWrite(PowerOnGPIO, HIGH);       // GPIO 保持高电平，持续给ESP供电 
  Serial.println("clean SPIFFS .... ");  
  Serial.println("Restore config.js defaults");
  file = SPIFFS.open("/scripts/config.js", "w");
  file.print("loadMyConfig({\n");
  file.print("\"RebootInterval\":\"3\",\n");            //用户看到的是分钟, 6/19/2018: JP asked to change to 3 minutes
  file.print("\"PingInterval\":\"00:30\",\n");          //Ping 的时间间隔，30秒
  file.print("\"Version\":\"");                 
  file.print(Version);
  file.print("\",\n");
  file.print("\"userSetReboot\":\"0\",\n");
  file.print("\"userSetRebootTime\":\"00:00\",\n");
  file.print("\"turnoffAPSet\":\"1\",\n");              // turnoff AP ater 20 minutes
  // 4/19/2018: 变更为 20，不再是 00:20 分钟; 
  // file.print("\"turnoffAPTime\":\"00:20\",\n"); 
  file.print("\"turnoffAPTime\":\"30\",\n");  
  file.print("\"apPassword\":\"connectme\",\n");         // AP的密码        
  file.print("\"IP1\":\"8.8.8.8\",\n");
  file.print("\"IP2\":\"8.8.4.4\",\n");
  file.print("\"IP3\":\"139.130.4.5\",\n");
  file.print("\"IP4\":\"141.1.1.1\",\n");                 
  file.print("\"IP5\":\"199.7.91.13\",\n");
  file.print("\"Language\":\"English\",\n");             // language setting: default English
  file.print("})");
  file.close();
  Serial.println("Restore config.js completed.");
  checkFile("/scripts/config.js");

  Serial.println("Delete history.js content");
  file = SPIFFS.open("/scripts/history.js", "w");
  file.print("var histories = []");
  file.close();
  Serial.println("Delete history.js content completed.");
  checkFile("/scripts/history.js");

  Serial.println("Delete mySSID.js content");
  file = SPIFFS.open("/scripts/mySSID.js", "w");
  file.print("loadSSID([\n");
  file.print("])");
  file.close();
  Serial.println("Delete mySSID.js content completed.");
  checkFile("/scripts/mySSID.js");      

  Serial.println("Delete router.js content");
  file = SPIFFS.open("/scripts/router.js", "w");
  file.print("var myRouter ={\n");
  file.print("\"routerSSID\":\"\",\n");
  file.print("\"routerPassword\":\"\",\n");
  file.print("\"Status\":\"1\"\n");                 // once connects to router, start ping right away
  file.print("}");
  file.close();
  Serial.println("Delete router.js content completed.");
  checkFile("/scripts/router.js");      

  Serial.println("Reset completed. Restart system.");
  ESP.restart();
}

