#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println("RTC Error");
    while (1);
  }
}

void loop() {
  DateTime raw = rtc.now();
  DateTime thai = rtc.now() + TimeSpan(7 * 3600);

  Serial.print("RTC RAW: ");
  Serial.print(raw.year()); Serial.print("/");
  Serial.print(raw.month()); Serial.print("/");
  Serial.print(raw.day()); Serial.print(" ");
  Serial.print(raw.hour()); Serial.print(":");
  Serial.print(raw.minute()); Serial.print(":");
  Serial.println(raw.second());

  Serial.print("RTC +7H: ");
  Serial.print(thai.year()); Serial.print("/");
  Serial.print(thai.month()); Serial.print("/");
  Serial.print(thai.day()); Serial.print(" ");
  Serial.print(thai.hour()); Serial.print(":");
  Serial.print(thai.minute()); Serial.print(":");
  Serial.println(thai.second());

  Serial.println("----------------------");

  delay(1000);
}
