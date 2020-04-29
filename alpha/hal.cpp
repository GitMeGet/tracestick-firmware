#include "hal.h"
#include "esp_pm.h"



// Lower level library include decisions go here

#ifdef HAL_M5STICK_C
#include <M5StickC.h>

#elif HAL_M5STACK
#include <M5Stack.h>

#endif


// Your one and only
_TS_HAL TS_HAL;
_TS_HAL::_TS_HAL() {}

void _TS_HAL::begin()
{
  log_init();

#ifdef HAL_M5STICK_C
  M5.begin();
  
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);

  // Set LED pins
  pinMode(10, OUTPUT);
#endif
}

void _TS_HAL::update()
{
#ifdef HAL_M5STICK_C
  M5.update();
#endif
}



//
// LCD
//

void _TS_HAL::lcd_brightness(uint8_t level)
{
  if(level > 100) level = 100;
  else if(level < 0) level = 0;

#ifdef HAL_M5STICK_C
  // m5stickc valid levels are 7-15 for some reason
  // level / 12 -> (0-8), +7 -> 7-15
  M5.Axp.ScreenBreath(7 + (level / 12));
#endif
}

void _TS_HAL::lcd_backlight(bool on)
{
#ifdef HAL_M5STICK_C
  M5.Axp.SetLDO2(on);
#endif  
}

void _TS_HAL::lcd_sleep(bool enabled)
{
  // does not exist on m5stick-c
}

void _TS_HAL::lcd_cursor(uint16_t x, uint16_t y)
{
#ifdef HAL_M5STICK_C
  // x, y, font
  // We only use font 2 for now
  M5.Lcd.setCursor(x, y, 2);
#endif
}

void _TS_HAL::lcd_printf(const char* t)
{
#ifdef HAL_M5STICK_C
  M5.Lcd.printf(t);
#endif
}

void _TS_HAL::lcd_printf(const char* t, int a, int b, int c)
{
#ifdef HAL_M5STICK_C
  M5.Lcd.printf(t, a, b, c);
#endif  
}
    


//
// RTC
//

void _TS_HAL::rtc_get(TS_DateTime &dt)
{
#ifdef HAL_M5STICK_C
  RTC_TimeTypeDef RTC_TimeStruct;
  RTC_DateTypeDef RTC_DateStruct;
  M5.Rtc.GetTime(&RTC_TimeStruct);
  M5.Rtc.GetData(&RTC_DateStruct);
  dt.hour   = RTC_TimeStruct.Hours;
  dt.minute = RTC_TimeStruct.Minutes;
  dt.second = RTC_TimeStruct.Seconds;
  dt.day    = RTC_DateStruct.Date;
  dt.month  = RTC_DateStruct.Month;
  dt.year   = RTC_DateStruct.Year;
#endif
}
   
void _TS_HAL::rtc_set(TS_DateTime &dt)
{
#ifdef HAL_M5STICK_C
  RTC_TimeTypeDef TimeStruct;
  RTC_DateTypeDef DateStruct;
  TimeStruct.Hours   = dt.hour;
  TimeStruct.Minutes = dt.minute;
  TimeStruct.Seconds = dt.second;
  // DateStruct.WeekDay = 3; // TODO: do we even need this
  DateStruct.Month  = dt.month;
  DateStruct.Date   = dt.day;
  DateStruct.Year   = dt.year;
  M5.Rtc.SetTime(&TimeStruct);
  M5.Rtc.SetData(&DateStruct); // NOTE: apparently their lib has a typo
#endif
}



//
// Misc IO
//

void _TS_HAL::setLed(TS_Led led, bool enable)
{
#ifdef HAL_M5STICK_C
  switch(led) {
    case TS_Led::Red:
      digitalWrite(10, enable ? 0 : 1); // logic is reversed probably due to pulldown
      break;
  }
#endif
}


//
// BLE
//
BLEScan* pBLEScan;

void _TS_HAL::ble_init()
{
  BLEDevice::init(DEVICE_NAME);

  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setActiveScan(true);    //active scan uses more power, but get results faster
  // pBLEScan->setInterval(100);    // in ms , left to defaults
  // pBLEScan->setWindow(99);
}

BLEScanResults _TS_HAL::ble_scan(uint8_t seconds)
{
  pBLEScan->clearResults();
  return pBLEScan->start(seconds, false);
}





//
// Power management
//

void _TS_HAL::sleep(TS_SleepMode sleepMode, uint32_t ms)
{
  switch(sleepMode)
  {
    case TS_SleepMode::Light:
#ifdef HAL_M5STICK_C
      M5.Axp.LightSleep(SLEEP_MSEC(ms));
#endif
      break;
      
    case TS_SleepMode::Deep:
#ifdef HAL_M5STICK_C
      M5.Axp.DeepSleep(SLEEP_MSEC(ms));
#endif
      break;

    default:
      delay(ms);
  }
}

void _TS_HAL::power_off()
{
#ifdef HAL_M5STICK_C
  M5.Axp.PowerOff();
#endif
}

void _TS_HAL::reset()
{
  ESP.restart();
}


//
// Common logging functions
//

void _TS_HAL::log_init()
{
#ifdef HAL_SERIAL_LOG
  Serial.begin(115200);
#endif
}



//
// Debug
//

// logs a failure message and reboots
void _TS_HAL::fail_reboot(const char *msg)
{
  log(msg);
  delay(3000);
  reset();
}

