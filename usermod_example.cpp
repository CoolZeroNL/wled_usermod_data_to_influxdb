#pragma once

#include "wled.h"
#include <Arduino.h>
#include <Wire.h>


#if defined ( ESP8266 )

  #include <WiFiUdp.h>
  #include <ESP8266mDNS.h>
  #include <mDNSResolver.h>

  // loaded trought the usermods_list.cpp
  // #include <ESP8266HTTPClient.h>
  // #include <WiFiClient.h>

  using namespace mDNSResolver;
  // WiFiClient wifiClient;
  WiFiUDP udp;
  Resolver resolver(udp);

#else
  #include <ESPmDNS.h>
#endif

// FOR NOW ONLY HTTP ENDPOINTS !

// V4:
// - usermods - settings page
// - influxdb2 -> token  -> 
// - influxdb2 -> host   -> 
// - influxdb2 -> port   -> 
// - influxdb2 -> bucket -> 
// - influxdb2 -> org    -> 
// - add field: strip.currentMilliamps

// this version works with the older version of wled (0.13.3b) sr

// V3:
// - esp and 8266 

// V2:
// - added vars

// V1:
// - init version

#ifdef __cplusplus
  extern "C" {
#endif

  uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

uint8_t temprature_sens_read();

static AsyncClient * aClient = NULL;
static AsyncClient * bClient = NULL;

// INFLUXDB SETTINGS
String grafanaIP;                           // variable holder
bool debugthis = true;                      // turn on/off the debugging to serial

// These config variables have defaults set inside readFromConfig()
String _host;
String _bucket;
String _org;
String _token = "myesptoken";
String _error = "";
int _forcecfg = 1;
int _port;
int _interval;
int _version = 1004;


// #if defined ( ESP8266 )
//   // const char* find_mdns_host = "grafana";         // hostname where the influxdb is runnig
//   const char* find_mdns_host = "tempdata.local";     // hostname where the influxdb is runnig
// #else
//   const char* find_mdns_host = "grafana";           // hostname where the influxdb is runnig
// #endif



// class UserMod_DataToInfluxDB : public Usermod
class MyExampleUsermod : public Usermod {

private:
  bool initialized = false;
  bool resolved = false;
  bool forceConfig = false;
  unsigned long nextMeasure = 0;  
 
  bool usermodActive = false;
  

  //initialize mDNS service
  #if defined ( ESP8266 )
    String IpAddress2String(const IPAddress& ipAddress)
    {
      return String(ipAddress[0]) + String(".") +\
      String(ipAddress[1]) + String(".") +\
      String(ipAddress[2]) + String(".") +\
      String(ipAddress[3])  ; 
    }

    void resolve_mdns_host(const char * host_name){

        MDNS.update();
        Serial.printf("Query A: %s", host_name);
        Serial.println("");
        resolver.setLocalIP(WiFi.localIP());
        IPAddress ip = resolver.search(host_name);
        if (ip != INADDR_NONE)
        {
            // addr = ip;
            Serial.print("Host is resolved: ");
            Serial.println(ip);
            grafanaIP = IpAddress2String(ip);
            Serial.println("grafanaIP: " + grafanaIP);            
            resolved = true;
        } else {
            Serial.println("Host was not found!");
            return;
        }
    }

  #else
    void _initialize_mDNS()
    {
      esp_err_t err = mdns_init();
      if (err) {
          Serial.printf("mDNS Init failed: %d\n", err);
          return;
      }else{
        Serial.println("started mDNS...");
        initialized = true;
      }

    }

    // void resolve_mdns_host(const char * host_name)
    // {
    //     Serial.printf("Query A: %s.local", host_name);

    //     struct ip4_addr addr;
    //     addr.addr = 0;

    //     esp_err_t err = mdns_query_a(host_name, 2000,  &addr);
    //     if(err){
    //         if(err == ESP_ERR_NOT_FOUND){
    //             //Serial.println("Host was not found!");
    //             return;
    //         }
    //         Serial.println("Query Failed");
    //         return;
    //     }else{
    //       //Serial.println("Host is resolved");
    //       //Serial.printf(IPSTR, IP2STR(&addr));
    //       grafanaIP = String(ip4_addr1(&addr)) + "." + ip4_addr2(&addr) + "." + ip4_addr3(&addr) + "." + ip4_addr4(&addr);
    //       resolved = true;
    //     }
    // }
    void resolve_mdns_host(const char * host_name)
    {
        Serial.printf("Query A: %s.local\n", host_name);
    
        esp_ip4_addr_t addr;
        addr.addr = 0;
    
        esp_err_t err = mdns_query_a(host_name, 2000, &addr);
        if(err){
            if(err == ESP_ERR_NOT_FOUND){
                Serial.println("Host was not found!");
                return;
            }
            Serial.println("Query Failed");
            return;
        }else{
            Serial.println("Host is resolved");
            // Convert esp_ip4_addr_t to String IP
            // grafanaIP = String(IP4_ADDR_TO_UINT8(addr.addr, 0)) + "." +
            //             String(IP4_ADDR_TO_UINT8(addr.addr, 1)) + "." +
            //             String(IP4_ADDR_TO_UINT8(addr.addr, 2)) + "." +
            //             String(IP4_ADDR_TO_UINT8(addr.addr, 3));
            grafanaIP = String(ip4_addr1(&addr)) + "." + ip4_addr2(&addr) + "." + ip4_addr3(&addr) + "." + ip4_addr4(&addr);
            Serial.println("grafanaIP: " + grafanaIP);
            resolved = true;
        }
    }


  #endif  

   void runAsyncClient(){
    if(bClient)//client already exists
      return;

    bClient = new AsyncClient();
    if(!bClient)//could not allocate client
      return;

    bClient->onError([](void * arg, AsyncClient * client, int error){
      Serial.println("GetConfig: Connect Error");
      bClient = NULL;
      delete client;
    }, NULL);

    bClient->onConnect([](void * arg, AsyncClient * client){
      Serial.println("Connected");
      bClient->onError(NULL, NULL);

      client->onDisconnect([](void * arg, AsyncClient * c){
        Serial.println("Disconnected");
        bClient = NULL;
        delete c;
      }, NULL);

      

      client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
        
        //// DEBUG:
        // Serial.print("\r\nData: ");
        // Serial.println(len);
        // uint8_t * d = (uint8_t*)data;
        // // String aa;
        // for(size_t i=0; i<len;i++){
        //   // aa += d[i];
        //   Serial.write(d[i]);
        // }

        // Convert incoming data to a string
        String response = String((char*)data).substring(0, len);
        // Find the end of the headers
        int headersEnd = response.indexOf("\r\n\r\n");
        // Check if headers end was found
        if (headersEnd != -1) {
          // Extract the body from the response
          String body = response.substring(headersEnd + 4); // Skip "\r\n\r\n"
          // Now you can use `body` for further processing
          //Serial.println("Response body:");
          //Serial.println(body);

          // get org
          StaticJsonDocument<1024> doc;
          deserializeJson(doc, body);
          const char* org = doc["org"];

          //Serial.println(org);
          //Serial.println(_org);

          if (_org == org) {
              Serial.println("- [OK] ORG == ORG");
              // reconfig_initialized = true;
          }
          else 
          {
              Serial.println("- [NOK] ORG != ORG");
              _org = org;
              //serializeConfig();

                  // .pio/libdeps/esp32_eth_mine/MyMod/usermod_example.cpp:264:31: error: too few arguments to function 'void serializeConfig(ArduinoJson::JsonObject)'
                  //  serializeConfig();

                  // wled00/fcn_declare.h:39:6: note: declared here
                  //  void serializeConfig(JsonObject doc);

                        // does it now need a json arg ??


            
              // reboot to submit to eeprom, and read the new name as var
              // ESP.restart();
          }


        } else {
          Serial.println("No headers found in response");
        }
        
        // Serial.println("-------------------------------");
        // Serial.println(str);
        // Serial.println("-------------------------------");

      }, NULL);

      //send the request
      String a = "GET /firmware/configuration.h HTTP/1.0\r\n\r\n";
      //Serial.println(a);
      const char *C = a.c_str();
      client->write(C);

    }, NULL);

    const char *CgrafanaIP = grafanaIP.c_str();
    if(!bClient->connect(CgrafanaIP, 80)){
      //Serial.println("Connect Fail");
      AsyncClient * client = bClient;
      bClient = NULL;
      delete client;
    }

    forceConfig = true;

  }

  // AsyncClient
  // HTTP works
  // HTTPS does not work... without verify=false
  // https://github.com/me-no-dev/ESPAsyncTCP/issues/18
  void _updateData(){
    
    if(aClient)//client already exists
      // aClient->close(true);
      return;

    aClient = new AsyncClient();
    if(!aClient)//could not allocate client
      return;

    aClient->onError([](void * arg, AsyncClient * client, int error){
      //Serial.println("Connect Error");
      aClient = NULL;
      delete client;
    }, NULL);

    aClient->onConnect([](void * arg, AsyncClient * client){
      //Serial.println("Connected");
      aClient->onError(NULL, NULL);

      client->onDisconnect([](void * arg, AsyncClient * c){
        //Serial.println("Disconnected");
        aClient = NULL;
        delete c;
      }, NULL);

      client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
        //Serial.print("\r\nData: ");
        //Serial.println(len);

        
        // String aa;

        uint8_t * d = (uint8_t*)data;
        {
          for(size_t i=0; i<len;i++){
            // aa += d[i];
            Serial.write(d[i]);
          }

        }

        // Serial.println("-------------------------------");
        // Serial.println(aa);
        // Serial.println("-------------------------------");

      }, NULL);

      // we dont send timestamp, the receiving side will stamp it with a time.
      // herefore we need to make sure the last item in the line protocal is not been seend as a timestamp value
      
      // table,tagname=aaa field1=aa,field2=bb
      
      String Table = "wled_measurement";                                                               // should end with a , and notspace

      // ==================================================================================================================================
      String wledWLED_Version = ",WLED_Version=\"" + String(versionString) + "\"";                      // str
      String wledUMVersion = ",FW_Version=\"" + String(_version) + "\"";                                // str
      // ==================================================================================================================================
      String wledWIFIip = " Wifi_ClientIP=\"" + String(WiFi.localIP().toString().c_str()) + "\"";       // str // first field, should be seperated with a space from table, and tag
      String wledETHip = ",ETH_ClientIP=\"" + String(ETH.localIP().toString().c_str()) + "\"";          // str
      String wledWifi_Signal = ",Wifi_Signal=" + String(WiFi.RSSI());                                   // int
      String wledclientSSID = ",Wifi_SSID=\"" + String(WiFi.SSID()) + "\"";                              // str
      // ==================================================================================================================================
      String wledMqttHost = ",mqtt_Host=\"" + String(mqttServer) + "\"";                                // str
      String wledMqttPort = ",mqtt_Port=\"" + String(mqttPort) + "\"";                                  // str
      String wledMqttEnabled = ",mqtt_Enabled=\"" + String(mqttEnabled) + "\"";                         // str
      String wledMqttClientID = ",mqtt_ClientId=\"" + String(mqttClientID) + "\"";                      // str
      String wledMqttGroupTopic = ",mqtt_GroupTopic=\"" + String(mqttGroupTopic) + "\"";                // str
      // ==================================================================================================================================
      String wledCount = ",ledCount=" + String(int(strip.getLengthTotal())) + "";                                                         // int
      String wledPwr = ",Pwr=" + String(int(BusManager::currentMilliamps())) + "";                                                   // int
      String wledMaxPwr = ",MaxPwr=" + String(int(BusManager::currentMilliamps()>0 ? BusManager::ablMilliampsMax() : 0)) + "";       // int
      String wledFPS = ",fps=" + String(int(strip.getFps())) + "";                                                                   // int
      // ==================================================================================================================================
      String wledInDBHost = ",InDB_Host=\"" + String(_host) + "\"";                                     // str
      String wledInDBPort = ",InDB_Port=\"" + String(_port) + "\"";                                     // str
      String wledInDBOrg = ",InDB_Org=\"" + String(_org) + "\"";                                        // str
      String wledInDBBucket = ",InDB_Bucket=\"" + String(_bucket) + "\"";                               // str
      String wledInDBInterval = ",InDB_Interval=" + String(_interval) + "";                             // int
      // ==================================================================================================================================
      // String wledlastKnownApSsid = ",wledlastKnownApSsid=\"" + String(apSSID) + "\"";                  // last AP ssid
      // String wledlastKnownApPass = ",wledlastKnownApPass=\"" + String(apPass) + "\"";                  // last ssid pass
      // String wledclientPass = ",wledClientPass=\"" + String(clientPass) + "\"";                        
      // String wledRuntime = ",wledRuntime=" + String(millis());
      // String wledFree_heap = ",wledFreeheap=" + String(ESP.getFreeHeap());
      // String wledWifi_State = ",wledWifiState=" + String(WiFi.status());
      // String wledNetworkisConnected = ",wledNetworkisConnected=" + String(Network.isConnected());
      // String wledlastKnownWiFiConnected = ",wledlastKnownWiFiConnected=" + String(WiFi.isConnected());
      // String wledlastNTPsync = ",wledlastNTPsync=" + String(ntpLastSyncTime);  
      // String wledlastKnownApChannel = ",wledlastKnownApChannel=" + String(apChannel);
      // ==================================================================================================================================
      String wledServerDescription = ",Hostname=" + String(serverDescription) + "";                       // tag is attached to table splitted with , no space
      // ==================================================================================================================================

      
      #if defined ( ESP8266 )
        String wledMDNSHostname = ",mdnsHostname=" + String(WiFi.hostname()) + "";                                 // str // tag is attached to table splitted with , no space  --> 8266

        // String wledtemperatureRead = ",wledtemperatureRead=" + "-1";  // not availible for esp8266 
        String wledtemperatureRead = ",wledtemperatureRead=" + String(temperatureRead());
      
        // String wledPwr = ",Pwr=" + String(int(strip.currentMilliamps)) + "";                                                        // int
        // String wledMaxPwr = ",MaxPwr=" + String(int(strip.ablMilliampsMax)) + "";                                                   // int
      
      #else
        // ========================================================================================
        String wledMDNSHostname = ",mdnsHostname=" + String(WiFi.getHostname()) + "";                       // tag is attached to table splitted with , no space
        // ========================================================================================
        String wledTotal_PSRAM = ",wledTotalPSRAM=" + String(ESP.getPsramSize()/1024);
        String wledFree_PSRAM = ",wledFreePSRAM=" + String(ESP.getFreePsram()/1024);
        // ========================================================================================  
        String wledtemperatureRead = ",wledtemperatureRead=" + String(temperatureRead());
        // ========================================================================================

      #endif

      String postdata = Table + wledMDNSHostname + wledServerDescription + wledWIFIip + wledETHip + wledWifi_Signal + wledclientSSID + wledtemperatureRead + wledMqttHost + wledMqttPort + wledMqttEnabled + wledMqttClientID + wledMqttGroupTopic + wledWLED_Version + wledPwr + wledMaxPwr + wledFPS + wledInDBHost + wledInDBPort + wledInDBOrg + wledInDBBucket + wledInDBInterval;
      
      //Serial.println(postdata);
      int len=postdata.length();
     
      //Serial.print("Send data to influxdb: ");
      //Serial.println(grafanaIP);

      // URL ENCODE (Main Org.) = (Main%20Org.)
      // URL ENCODE (bbq)       = (bbq)

      _org.replace(" ","%20");

          // mDNS:
          String a = "POST /influxdb/api/v2/write?org=" + String(_org) + "&bucket=" + String(_bucket) + "&precision=s HTTP/1.0\r\nContent-Length:" + String(len) + "\r\nAuthorization: Token " + String(_token) + "\r\n\r\n" + postdata + "\r\n";
          Serial.println(a);

          // /influxdb/api/v2/write?org=$org&bucket=$bucket&precision=s"

          // HTTPS
          // if on ip then + host header ?
          // fqdn ?
          // String a = "POST /api/v2/write?org=" + String(_org) + "&bucket=" + String(_bucket) + "&precision=s HTTP/1.0\r\nContent-Length:" + String(len) + "\r\nAuthorization: Token " + String(_token) + "\r\n\r\n" + postdata + "\r\n";

      const char *C = a.c_str();
      client->write(C);
      
    }, NULL);

    // if(!aClient->connect("0.0.0.0", 8086)){
    const char *CgrafanaIP = grafanaIP.c_str();
    if(!aClient->connect(CgrafanaIP, _port)){
      //Serial.println("Connect Fail");
      AsyncClient * client = aClient;
      aClient = NULL;
      delete client;
    }
  }

public:
  void setup()
  {
    Serial.println("Starting!");
  }

  // gets called every time WiFi is (re-)connected.
  void connected()
  {
    nextMeasure = millis() + 5000; // Schedule next measure in 5 seconds
  }

  void loop()
  {

    if (!usermodActive || strip.isUpdating()) return;
    
    unsigned long tempTimer = millis();

    if (tempTimer > nextMeasure)
    {
      nextMeasure = tempTimer + _interval; // Schedule next measure in 60 seconds

      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("Error! Wifi is not connected!");
        return; // lets try again next loop
      }

      if (!resolved)
      {
        Serial.println("Error! mDNS could not resolve in loop()!");
        // resolve_mdns_host(find_mdns_host);
        Serial.println(_host.c_str());
        resolve_mdns_host(_host.c_str());

        return; // lets try again next loop
      }

      if (_forcecfg == 1)
      {
        if (!forceConfig)
        {
          Serial.println("enforcing config from url");
          // resolve_mdns_host(find_mdns_host);
          // resolve_mdns_host(_host.c_str());
          runAsyncClient();

          return; // lets try again next loop
        }      
      }
      // Update sensor data
      _updateData();
      
    }
  }

  // void readFromJsonState(JsonObject& root)
  // bool readFromConfig(JsonObject& root)
  // {
  //   _pwr = root["leds"]["pwr"] | _pwr; //if "user0" key exists in JSON, update, else keep old value
  //   _maxpwr = root["leds"]["maxpwr"] | _maxpwr; //if "user0" key exists in JSON, update, else keep old value
  //   //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
  // }

  void addToConfig(JsonObject& root)
  {

    Serial.println("addToConfig:");

    JsonObject top = root.createNestedObject("InfluxDB2");
    //save these vars persistently whenever settings are save
    top[F("active")] = usermodActive;
    top["host"] = _host;
    top["port"] = _port;
    top["interval"] = _interval;
    // top["token"] = _token;
    top["bucket"] = _bucket;
    top["org"] = _org;
    top["error"] = _error;
    top["version"] = _version;
    top["forceConfig"] = _forcecfg;
    DEBUG_PRINTLN(F("Autosave config saved."));
  }

  bool readFromConfig(JsonObject& root)
  {

    // default settings values could be set here (or below using the 3-argument getJsonValue()) instead of in the class definition or constructor
    // setting them inside readFromConfig() is slightly more robust, handling the rare but plausible use case of single value being missing after boot (e.g. if the cfg.json was manually edited and a value was removed)

    Serial.println("readFromConfig:");
    
    JsonObject top = root["InfluxDB2"];

    bool configComplete = !top.isNull();
    // if (top.isNull()) {
    //   DEBUG_PRINTLN(F("No config found. (Using defaults.)"));
    //   return false;
    // }

    // A 3-argument getJsonValue() assigns the 3rd argument as a default value if the Json value is missing
    configComplete &= getJsonValue(top[F("active")], usermodActive);
    configComplete &= getJsonValue(top["host"], _host, "tempdata");
    configComplete &= getJsonValue(top["port"], _port, 80); 
    configComplete &= getJsonValue(top["interval"], _interval, 10000); 
    // configComplete &= getJsonValue(top["token"], _token, "myesptoken");
    configComplete &= getJsonValue(top["bucket"], _bucket, "data");
    configComplete &= getJsonValue(top["org"], _org, "Main Org.");
    configComplete &= getJsonValue(top["error"], _error, "");
    configComplete &= getJsonValue(top["forceConfig"], _forcecfg, 1);
    // configComplete &= getJsonValue(top["version"], _version, 1001);
    
    DEBUG_PRINTLN(F("config (re)loaded."));
    
    return configComplete;
  }


};

static MyExampleUsermod example_usermod;
REGISTER_USERMOD(example_usermod);
