
void securityPageLoad (AsyncWebServerRequest *request) {
  ESP.wdtFeed();
  Serial.println("\nStart security.html page-load ");
  request->send(SPIFFS, "/security.html");
  Serial.println("End of register.html page-load");
}

void securityPageSubmit(AsyncWebServerRequest *request) {           //如果账户存在，登录不成功，仍旧不能再次注册，需要进入RESET过程
  String inputPassword1, inputPassword2;    
  Serial.println("\nSecurity page submit()");
  ESP.wdtFeed();
  /* Check for (inputPassword1 == inputPassword2) is done in the HTML page
  *  Only when (inputPassword1 == inputPassword2), HTML page sends back inputPassword_1 value
  */
  if (request->hasArg("inputPassword1")){
    inputPassword1 = request->arg("inputPassword1");  
    inputPassword1.trim();        
    Serial.print("Password_1 is: "); Serial.print(inputPassword1); Serial.println("[End]");
    //保存zobox-ABCD Wi-Fi AP password到 /scripts/config.js 中 
    if(inputPassword1.length() < 8) {   
      request->send(200, "text/json", "{\"code\":\"0\", \"msg\":\"Password must be longer than 8 characters. Please try again.\"}");  
      Serial.println("Password is shorter than 8 chars. User need to re-enter password.");         
    } 
    else {
      saveVarToConfig("apPassword",inputPassword1);
      Serial.println("Saved Zobox-ABCD Wi-Fi Password to config.js file.");  
      if(connectedToRouter == false){      // not connected to Router, webPage jumps to /quick.html
          request->send(200, "text/json", "{\"code\":\"1\", \"msg\":\"Zobox Wi-Fi Access Point password saved. It will be effective when Zobox restarts again.\"}"); 
      }
      else{                               // webPage jumps to /dashboard.html  
          request->send(200, "text/json", "{\"code\":\"2\", \"msg\":\"Zobox Wi-Fi Access Point password saved. It will be effective when Zobox restarts again.\"}");                   
      }
    }
  }
  else {
    Serial.println("Server does not found - inputPassword_1 arg.");                   
  }
/*       
  // 保存zobox-ABCD Wi-Fi AP password到 /scripts/config.js 中
  if (inputPassword1==inputPassword2){
    saveVarToConfig("AP_password",inputPassword1);
    request->send(200, "text/plain", "{\"code\":\"1\", \"msg\":\"Zobox Wi-Fi Access Point password saved.\"}"); 
    Serial.println("Saved Zobox-ABCD Wi-Fi Password to config.js file.");                 
    checkFile("/scripts/config.js");            
  }
  else{
    request->send(200, "text/plain", "{\"code\":\"0\", \"msg\":\"Error: Passwords entered must be the same. Please try again.\"}"); 
    Serial.println("Error: Passwords entered must be the same. Please try again.");     
  }
*/      
  Serial.println("Exit securityPageSubmit()");      
}


