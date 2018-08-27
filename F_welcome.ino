
void welcomePageLoad(AsyncWebServerRequest *request) {
  ESP.wdtFeed();
  Serial.println("\nShow welcome.html ");
  request->send(SPIFFS, "welcome.html");  
}

void welcomePageSubmit(AsyncWebServerRequest *request) {      
  String tempStr;
  ESP.wdtFeed();
  Serial.println("\nwelomePageSubmit()");
  if(connectedToRouter)
    tempStr = "{\"code\":\"1\",\"data\":\"1\"}";                 
  else
    tempStr = "{\"code\":\"0\",\"data\":\"0\"}";  
  Serial.print("tempStr = ");
  Serial.println(tempStr);
  request->send(200,"text/json", tempStr);
  Serial.println("welcomePageSubmit() End");           
}
