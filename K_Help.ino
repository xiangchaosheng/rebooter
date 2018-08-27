void helpPageLoad(AsyncWebServerRequest *request) {
  //无需用户登录，就可以看到Help页面
  Serial.println("\nShow help.html ");
  ESP.wdtFeed();
  request->send(SPIFFS, "/help.html");      
}

void helpPageSubmit(AsyncWebServerRequest *request) {      
  Serial.println("\nhelp page Submit ... nothing here");
}
