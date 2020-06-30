#include "cleanbox.h"
#include "hal.h"
#include "radio.h"
#include "ui.h"
#include "power.h"
#include "opentracev2.h"
#include "serial_cmd.h"
#include "storage.h"

// Notes:
// - look at mods/boards.diff.txt -- set CPU to 80mhz instead of 240mhz
//

#ifndef WIFI_SSID
#define WIFI_SSID "test"       // Enter your SSID here
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "password"    // Enter your WiFi password here
#endif

void setup() {
  TS_HAL.begin();

  TS_Storage.begin();
  log_w("Storage free: %d, %d%", TS_Storage.freespace_get(), TS_Storage.freespace_get_pct());

  TS_Settings* settings = TS_Storage.settings_get();

  if (strcmp(settings->userId, "") == 0)
  {
    memset(settings->userId, '\0', sizeof(settings->userId));
    strcpy(settings->userId, "0123456789");
  }

  if (strcmp(settings->wifiSsid, "") == 0)
  {
    memset(settings->wifiSsid, '\0', sizeof(settings->wifiSsid));
    strcpy(settings->wifiSsid, WIFI_SSID);
  }

  if (strcmp(settings->wifiPass, "") == 0)
  {
    memset(settings->wifiPass, '\0', sizeof(settings->wifiPass));
    strcpy(settings->wifiPass, WIFI_PASS);
  }

  log_i("Stored settings:");
  log_i("Version: %d", settings->settingsVersion);
  log_i("UID: %.32s", settings->userId);
  log_i("WIFI_SSID: %.32s", settings->wifiSsid);
  log_i("WIFI_PASS: %.32s", settings->wifiPass);

  TS_Storage.settings_save();

  // load settings from storage
  TS_RADIO.init();

  OT_ProtocolV2.begin();

  TS_POWER.init();
  
  // This starts a new task
  TS_UI.begin();

  // Start serial command console
  TS_SerialCmd.init();
  TS_SerialCmd.begin();

  log_w("Crash count: %d", TS_PersistMem.crashCount);
  log_i("Setup completed free heap: %d", ESP.getFreeHeap());
}

int skips = 0;

void loop() {
  TS_HAL.update();
  TS_POWER.update();

  // blink once a second
  TS_HAL.led_set(TS_Led::Red, true);
  TS_HAL.sleep(TS_SleepMode::Default, 1);
  TS_HAL.led_set(TS_Led::Red, false);

  // Wifi 
  TS_RADIO.wifi_enable(TS_POWER.get_state() == TS_PowerState::HIGH_POWER);
  TS_RADIO.wifi_update();

  if (TS_HAL.ble_is_init())
  {
    // don't turn off radio if we have connected clients
    uint16_t connectedCount = OT_ProtocolV2.get_connected_count();
    uint16_t sleepDuration = TS_HAL.random_get(1000, 3000);

    log_i("Devices connected: %d", connectedCount);
    
    if(connectedCount > 0) {
      TS_HAL.sleep(TS_SleepMode::Task, sleepDuration);
    } else {
      // TODO: figure out how to sleep deeper
      // TS_HAL.sleep(TS_SleepMode::Light, sleepDuration);
      TS_HAL.sleep(TS_SleepMode::Task, sleepDuration);
    }

    if(skips >= 5) { // vary the interval between scans here
      skips = 0;
      
      // spend up to 1s scanning, lowest acceptable rssi: -95
      OT_ProtocolV2.scan_and_connect(1, -95);
    }
    else
    {
      ++skips;
    }

    // enable advertising
    OT_ProtocolV2.advertising_start();
    
    // just advertise for 1s
    TS_HAL.sleep(TS_SleepMode::Task, 1000);
    
    // disable advertising, get back to sleep
    OT_ProtocolV2.advertising_stop();

  }

  // Give some time for comms after broadcasts
  // TODO: by right should wait T time after last uncompleted handshake before going back to sleep
  TS_HAL.sleep(TS_SleepMode::Task, 100);

  // TODO: call OT update_characteristic_cache at least once every 15 mins
}
