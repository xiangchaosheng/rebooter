/* functions for advanced.html */
void advancedPageLoad(AsyncWebServerRequest *request) {
  Serial.println("\nAdvanced page-load()");
  ESP.wdtFeed();
//  checkFile("/scripts/config.js");                      //检查文件内容
//  request->send(SPIFFS, "/advanced.html");          
  Serial.println("End of Advanced page-load");
}

void advancedPageSubmit(AsyncWebServerRequest *request) {               // save to /scripts/config.js
  
/* 将 /advanced 页面中的参数，保存入 /scripts/config.js 文件中，替代缺省值   
 *  参数的检验在HTML代码中 JS 来实现的，这里就不再检验用户输入数据的有效性了
*/
  String inputRebootInterval, inputPingInterval, inputAPSurvivalTime, inputEmail, otaCheckRequest; 
  int ipPart1, ipPart2, ipPart3, ipPart4, index1, index2, index3;      
  String inputIP;
  Serial.println("\nAdvanced page Submit()");
  ESP.wdtFeed();
  if (request->hasArg("RebootInterval")){
    inputRebootInterval = request->arg("RebootInterval");
    inputRebootInterval.trim();
    Serial.print("RebootInterval entered is : "); 
    Serial.println( inputRebootInterval); 
    saveVarToConfig("RebootInterval",inputRebootInterval);               
  }
  else 
    Serial.println("RebootInterval variable was not changed.");

  if (request->hasArg("PingInterval")){
    inputPingInterval = request->arg("PingInterval");
    inputPingInterval.trim();
    Serial.print("PingInterval entered is : "); 
    Serial.println(inputPingInterval);   
    saveVarToConfig("PingInterval", inputPingInterval);              
  }
  else 
    Serial.println("PingInterval variable was not changed.");

  if(request->hasArg("IP1")){
    inputIP = request->arg("IP1");
    inputIP.trim();
    Serial.print("Received user input IP-1: "); 
    Serial.println(inputIP);
    if(inputIP.length()>15)                                     // 15: 是255.255.255.255的长度
      Serial.println("IP-1 input format error");
    else{
      convertStringtoIP(inputIP, 0);
      saveVarToConfig("IP1",inputIP);
    }
  }
  else 
    Serial.println("IP1 variable was not changed.");

  if(request->hasArg("IP2")){
    inputIP = request->arg("IP2");
    inputIP.trim();
    Serial.print("Received user input IP-2: "); 
    Serial.println(inputIP);
    if(inputIP.length()>15)                                     // 15: 是255.255.255.255的长度
      Serial.println("IP-2 input format error");
    else{
      convertStringtoIP(inputIP, 1);
      saveVarToConfig("IP2",inputIP);
    }
  }
  else 
    Serial.println("IP2 variable was not changed.");

  if(request->hasArg("IP3")){
    inputIP = request->arg("IP3");
    inputIP.trim();
    Serial.print("Received user input IP-3:"); 
    Serial.println(inputIP);
    if(inputIP.length()>15)                                     // 15: 是255.255.255.255的长度
      Serial.println("IP-3 input format error");
    else{
      convertStringtoIP(inputIP, 2);
      saveVarToConfig("IP3",inputIP);
    }
  }
  else 
    Serial.println("IP3 variable was not changed.");

  if(request->hasArg("IP4")){
    inputIP = request->arg("IP4");
    inputIP.trim();
    Serial.print("Received user input IP-4:"); 
    Serial.println(inputIP);
    if(inputIP.length()>15)                                     // 15: 是255.255.255.255的长度
      Serial.println("IP-4 input format error");
    else{
      convertStringtoIP(inputIP, 3);
      saveVarToConfig("IP4",inputIP);
    }
  }
  else 
    Serial.println("IP4 variable was not changed.");

  if(request->hasArg("IP5")){
    inputIP = request->arg("IP5");
    inputIP.trim();
    Serial.print("Received user input IP-5:"); 
    Serial.println(inputIP);
    if(inputIP.length()>15)                                     // 15: 是255.255.255.255的长度
      Serial.println("IP-5 input format error");
    else{
      convertStringtoIP(inputIP, 4);
      saveVarToConfig("IP5",inputIP);
    }
  }
  else 
    Serial.println("IP5 variable was not changed.");   

  /*  6/4/2018
   *  insert the following code for Language setting 
   */
  if(request->hasArg("Language")){
    languageChoice = request->arg("Language");
    Serial.print("Received user input Language:");
    Serial.println(languageChoice);
    saveVarToConfig("Language", languageChoice);
    Serial.println("Restart ESP to make the Language setting in effect >>>");
    ESP.restart();
  }
  else
    Serial.println("Language variable was not changed.");  
     
  Serial.println("Reload changes just saved and assign to varablies by calling loadConfig()");
  if (loadConfig())
    Serial.println("Load config from config.js success.");
  else
    Serial.println("Load config from config.js Failure. Continue using current values.");

 
  // check OTA
  if(request->hasArg("otaCheck")){
     if(firmwareNeedUpdate || spiffsNeedUpdate){
        Serial.println("inform /advanced.html that new firmware update is avaiable.");
        request->send(200,"text/json", "{\"code\":\"1\",\"data\":\"New firmware available for update.\"}"); 
    }      
    else{
        Serial.println("inform /advanced.html that NO firmware update is avaiable.");
        request->send(200,"text/json", "{\"code\":\"0\",\"data\":\"No updates available.\"}"); 
    }    
  }
  else{
      Serial.println("otaCheck variable was not changed."); 
      Serial.println("Set needOTACheck = false to disable OTA check in loop()");
      needOTAcheck = false;           
  }

  Serial.println("Leave Advanced page Submit()");
}

/* 载入 /scripts/config.js 里的参数给全局变量 */
bool loadConfig() {  
  String tempStr;
  char c;     
  File configFile = SPIFFS.open("/scripts/config.js", "r");  
  ESP.wdtFeed();
  Serial.println("\nRead and assign Global variable value from config.js ");
  if(configFile){
    //reboot interval 
    while(configFile.available()){
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }
    c = configFile.read();           // "RebootInterval":'00:00:10', hh:mm:ss changes to a number between 1～30
    while(configFile.available()){
      c = configFile.read();
      if (c != ',')
        tempStr = tempStr + c;    // tempStr = 00:00:10' changes to a number between 1～30
      else
        break;
    }
    //RebootInterval = ((tempStr[0]-'0')*10 + (tempStr[1]-'0'))*3600 + ((tempStr[3]-'0')*10+(tempStr[4]-'0'))*60 + ((tempStr[6]-'0')*10+(tempStr[7]-'0'));
    RebootInterval = tempStr.toInt()*60;
    Serial.printf("Reboot Interval = %d seconds\r\n", RebootInterval);
    //ping interval 
    tempStr = "";
    while(configFile.available()){    //"PingInterval":'00:00:30' changes to 00:30
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }
    c = configFile.read();           //changed from 00:00:30', to 00:30
    while(configFile.available()){
      c = configFile.read();
      if (c != ',')
        tempStr = tempStr + c;    // tempStr = 00:00:10'
      else
        break;
    }
    //PingInterval = ((tempStr[0]-'0')*10 + (tempStr[1]-'0'))*3600 + ((tempStr[3]-'0')*10+(tempStr[4]-'0'))*60 + ((tempStr[6]-'0')*10+(tempStr[7]-'0'));
    PingInterval = ((tempStr[0]-'0')*10 + (tempStr[1]-'0'))*60 + ((tempStr[3]-'0')*10+(tempStr[4]-'0'));
    Serial.printf("Ping Interval = %d seconds\r\n", PingInterval);  
    // Version
    tempStr = "";
    while(configFile.available()){    // "Version":"1.1",
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }       
    c = configFile.read();           // 1.1", 
    while(configFile.available()){
      c = configFile.read();
      if (c != ',')
        tempStr = tempStr + c;    // tempStr = 1.1"
      else
        break;
    }
    Version = tempStr.substring(0, tempStr.length()-1);
    Serial.print("Version = "); 
    Serial.println(Version); 
    // userSetReboot
    tempStr = "";
    while(configFile.available()){    // "userSetReboot":'0',
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }       
    c = configFile.read();
    c = configFile.read();
    if (c == '1'){
      userSetReboot = true;
      Serial.println("UserSetReboot = True");
    }
    if (c =='0'){
      userSetReboot = false;
      Serial.println("UserSetReboot = False");
    }
    // "userSetRebootTime":'',
    tempStr = "";
    while(configFile.available()){    // "userSetRebootTime":"", 或者  "userSetRebootTime":"03:30",
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }       
    c = configFile.read();
    while(configFile.available()){
      c = configFile.read();
      if (c != ',')
        tempStr = tempStr + c;    // tempStr = " 或者  03:30"
      else
        break;
    }       
    userSetRebootHour = (tempStr[0]-'0')*10 + (tempStr[1]-'0');
    userSetRebootMinute = (tempStr[3]-'0')*10 + (tempStr[4]-'0');
    Serial.print("User set reboot time:");
    Serial.println(tempStr);
    // turnOffAPSet
    tempStr = "";
    while(configFile.available()){    // "turnoffAPSet":"0",
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }       
    c = configFile.read();    
    c = configFile.read();
    if (c == '1'){
      turnoffAPSet = true;
      Serial.println("turnOffAPSet = True");
    }
    if (c =='0'){
      turnoffAPSet = false;
      Serial.println("turnOffAPSet = False");
    }             
    
    // turnoffAPTime               "不再是turnoffAPTime":"00:20", HH:MM，更改为一个以分钟为单位的数字 0 ～60
    tempStr = "";
    while(configFile.available()){    // 不再是"turnoffAPTime":"00:20"
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }       
    c = configFile.read();
    while(configFile.available()){
      c = configFile.read();
      if (c != ',')
        tempStr = tempStr + c;    // tempStr = " 或者  00:20"
      else
        break;
    }       
    //convert HH:MM to seconds and assign to turnoffAPTime variable    
    /* turnoffAPTime: 变更为一个单个分钟单位的数字 0～60. 4/19/2018
      *  Range: 0～60 minutes
    */
    Serial.print("Turn off AP time in minutes (read raw):");
    Serial.println(tempStr);
    tempStr.remove(tempStr.indexOf("\""));
    Serial.print("Turn off AP time in minutes (remove \"):");
    Serial.println(tempStr);        
    turnoffAPTime = tempStr.toInt()*60;        
    Serial.print("Convert to seconds:");
    Serial.println(turnoffAPTime);  
    
    // AP Password: AP_password
    tempStr = "";
    while(configFile.available()){                              // 
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }       
    c = configFile.read();
    while(configFile.available()){
      c = configFile.read();
      if (c != ',')
        tempStr = tempStr + c;                                  // 
      else
        break;
    }    
    Serial.print("AP_password: (read raw):");
    Serial.println(tempStr);  
    if(tempStr.indexOf("\"")>0)           
      tempStr.remove(tempStr.indexOf("\""));
    Serial.print("AP_password: (cleaned up):");
    Serial.println(tempStr);         
    AP_password = tempStr;
    Serial.print("Read AP_Password is:");
    Serial.println(AP_password);   
    //IP-0 ~ IP-4, loop repeat 5 times 
    for (int i=0; i<=4; i++){
      tempStr = "";
      while(configFile.available()){    // "IP1": "8.8.8.8",
        c = configFile.read();        
        if (c == ':'){
          break;             
        }
      }       
      c = configFile.read();          // skip :       
      while(configFile.available()){
        c = configFile.read();
        if (c == ',' || c == '\n')
          break;
        else
          tempStr = tempStr + c;    // tempStr = 8.8.8.8"
      }  
      tempStr.trim();                 // remove blanks
      tempStr = tempStr.substring(0, tempStr.indexOf("\""));  // 去掉尾部的" 
      Serial.print("IP string to be converted: ");
      Serial.println(tempStr);           
      convertStringtoIP(tempStr, i);      // arrayDNS[i]
    }
    // language setting : 6/4/2018
    tempStr = "";
    while(configFile.available()){                              // language setting 
      c = configFile.read();        
      if (c == ':'){
        break;             
      }
    }       
    c = configFile.read();    
    while(configFile.available()){
      c = configFile.read();
      if (c != ',' || c=='}')                                   // } means end of file
        tempStr = tempStr + c;                                  
      else
        break;
    }    
    Serial.print("Language setting (raw): ");
    Serial.println(tempStr);  
    if(tempStr.indexOf("\"")>0)           
      tempStr.remove(tempStr.indexOf("\""));
    Serial.print("Language Setting: (cleaned up):");
    Serial.println(tempStr);         
    languageChoice = tempStr;
    Serial.print("Read Language and set global variable languageChoice to:");
    Serial.println(languageChoice);   
                               
    configFile.close();
    Serial.println("Load config success");
    Serial.println();
    return true;
  }
  else{
    Serial.println("Failed to open the /config.js file to load global variables.");
    Serial.println();
    return false;
  }
  /*
    arrayDNS[0] = IPAddress(115,239,211,112);            // www.baidu.com 
    arrayDNS[1] = IPAddress(115,238,190,239);            // www.sina.com.cn
    arrayDNS[2] = IPAddress(119,29,29,29);               //  从网页 http://jingyan.baidu.com/article/a3a3f811d00f348da3eb8a6f.html
    arrayDNS[3] = IPAddress(114,114,114,114);            // 114 DNS    
    arrayDNS[4] = IPAddress(13,107,21,200);              // cn.bing.com
  */     
}

void convertStringtoIP(String inputIP, int iDNS){
  int ipPart1, ipPart2, ipPart3, ipPart4, index1, index2, index3;      
          
  Serial.println("Convert string to IP address");        
  Serial.print("String to be converted to IP address: ");
  Serial.println(inputIP);
  if(inputIP.length()>15)                                     // 15: 是255.255.255.255的长度
    Serial.println("IP input format error");
  else{                                                       //inputIP = " 192.16 . 42 .255"; 前后的空格都可以在toInt()时忽略
    index1 = inputIP.indexOf(".", 0);
    ipPart1 = inputIP.substring(0, index1).toInt();
    index2 = inputIP.indexOf(".", index1+1);
    ipPart2 = inputIP.substring(index1+1, index2).toInt();
    index3 = inputIP.indexOf(".",index2+1);
    ipPart3 = inputIP.substring(index2+1,index3).toInt();
    ipPart4 = inputIP.substring(index3+1,inputIP.length()).toInt();
    if(ipPart1<0 || ipPart1>255 || ipPart2<0 || ipPart2>255 || ipPart3<0 || ipPart3>255 || ipPart4<0 || ipPart4>255)
      Serial.printf("Invalid IP-%d\r\n", iDNS);      
    else { 
      arrayDNS[iDNS] = IPAddress(ipPart1,ipPart2,ipPart3,ipPart4); 
      Serial.printf("Saved IP to arrayDNS[%d]\r\n", iDNS);
    }  
  }
  Serial.println("Convert string to IP address completed");              
}


/* save variable to config.js file
 *  First round: Read and save the config.js file into 2 Strings: tempStr + tailStr
 *  tempStr contains the content (variable name) to be modified.
 *  tailStr contains the rest of the config.js file content. 
 *  ReOpen the config.js file as Write mode and then 
 *  write modeified content and tailStr. 
*/
void saveVarToConfig (String varName, String varValue){
  String tempStr, tailStr;
  char c;
  int i = 0, j = 0;
  Serial.println("Save variable to /scripts/config.js - BEGIN -");
  Serial.print(varName); 
  Serial.print(","); 
  Serial.println(varValue);
  File configFile = SPIFFS.open("/scripts/config.js", "r");                                
  if(!configFile){
    Serial.println();
    Serial.println("Failed to open /scripts/config.js file to Write.");
  }
  else{   // extract content to tempStr 
    configFile.seek(0,SeekSet); // beginning of file
    tempStr = "";
    while(configFile.available()){
      c = configFile.read(); 
      tempStr = tempStr + c;       
      if (c == ','){
        if(tempStr.indexOf(varName) > 0)
          break;   
      }
    } 
    Serial.print("tempStr =");
    Serial.println(tempStr); 
    Serial.print("index of variable is:");
    Serial.println(tempStr.indexOf(varName));
    // modify tempStr with varValue
    i = tempStr.indexOf(varName);
    j = tempStr.indexOf(':', i);              
    tempStr.remove(j+1);          // delete tempStr content after :
    tempStr = tempStr + "\"" + varValue + "\",";              
    // save the rest of config.js to tailStr
    tailStr = "";
    while(configFile.available()){
      c = configFile.read();
      tailStr = tailStr + c;        
    }
    Serial.print("tailStr =");
    Serial.println(tailStr);
    configFile.close();      
    // re-open config.js for Write only
    configFile = SPIFFS.open("/scripts/config.js", "w"); 
    configFile.print(tempStr);            
    configFile.print(tailStr);    // this should starts with a new line \n               
    configFile.close();
    Serial.println("Saved variable to /scripts/config.js");  
    checkFile("/scripts/config.js");                
  } 
  Serial.println("Save variable to /scripts/config.js - END -");         
}


