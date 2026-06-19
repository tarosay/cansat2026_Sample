#include <Arduino.h>
#include <TinyGPSPlus.h>

// The TinyGPSPlus object
TinyGPSPlus gps;
unsigned long lastRead = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 44, 43);  //G44->RXD, G43->TXD
  delay(1500);

  Serial.println(F("GPS V1.1 Sample2"));
}

void loop() {
  if (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }
  unsigned long now = millis();
  if (now - lastRead > 1000)  // 毎秒更新
  {
    lastRead = now;

    if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid())
    {
      int utcHour = gps.time.hour();
      int utcMinute = gps.time.minute();
      int utcSecond = gps.time.second();
      int jstHour = utcHour + 9;
      int jstDay = gps.date.day();
      int jstMonth = gps.date.month();
      int jstYear = gps.date.year();

      if (jstHour >= 24) {
        jstHour -= 24;
        jstDay += 1;
        // 月末処理（略式）
        if ((jstMonth == 4 || jstMonth == 6 || jstMonth == 9 || jstMonth == 11) && jstDay > 30) {
          jstDay = 1;
          jstMonth++;
        } else if (jstMonth == 2 && jstDay > 28) {
          jstDay = 1;
          jstMonth++;
        } else if (jstDay > 31) {
          jstDay = 1;
          jstMonth++;
        }
        if (jstMonth > 12) {
          jstMonth = 1;
          jstYear++;
        }
      }

      String log = "";
      log += "------ GNSS Data ------\n";
      log += "UTC: " + String(gps.date.year()) + "/" + String(gps.date.month()) + "/" + String(gps.date.day());
      log += " " + String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + "\n";
      log += "JST: " + String(jstYear) + "/" + String(jstMonth) + "/" + String(jstDay);
      log += " " + String(jstHour) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + "\n";

      log += "Latitude: " + String(gps.location.lat(), 6) + ", Longitude: " + String(gps.location.lng(), 6) + "\n";
      log += "Speed (km/h): " + String(gps.speed.kmph()) + "\n";
      log += "Course (deg): " + String(gps.course.deg()) + "\n";
      log += "Satellites: " + String(gps.satellites.value()) + "\n";
      log += "Altitude (m): " + String(gps.altitude.meters()) + "\n";
      log += "HDOP: " + String(gps.hdop.value()) + "\n";
      Serial.print(log);
    }
  }

  delay(10);
}
