#include "wled.h"

class MyMod : public Usermod {
  void setup() override {
    // called once at boot, before WiFi connects
  }

  void connected() override {
    // called each time WiFi (re)connects
  }

  void loop() override {
    // called every main-loop iteration — never use delay() here!
    if (millis() - lastTime > 2000) {
      lastTime = millis();
      // do something every 2 seconds
    }
  }

  private:
    unsigned long lastTime = 0;
};

static MyMod my_mod;
REGISTER_USERMOD(my_mod);   // self-registers — no usermods_list.cpp edits needed
