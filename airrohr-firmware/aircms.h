#include "Hash.h"
// Датчик iAQ-Core (VOC)
#include "iAQcore.h"
#define IAQCORE_I2CADDR 0x5A
iAQcore iaqcore;
bool iaqcore_is_present = false;
String AIRCMS_VERSION = "AIRCMS-2019-001";

void sha1Hex(const String& s, char hash[41]) {
    uint8_t buf[20];
    sha1(s, &buf[0]);
    for(int i = 0; i < 20; i++) {
        sprintf(&hash[i*2], "%02x", buf[i]);
    }
    hash[40] = (char)0;
}
void hmac1(const String& secret, const String& s, char hash[41]) {
    // usage as ptr
    Serial.print("Hashing string: ");
    Serial.println(s);
    char buf[41];
    sha1Hex(s, &buf[0]);
    String str = (char*)buf;
    Serial.println(secret + str);
    sha1Hex(secret + str, &hash[0]);
}
const char* host_our = "doiot.ru";
const char* url_our = "/php/sensors.php?h=";
int httpPort_our = 80;

void sendData2Us(const String& data, const int pin, const String& contentType) {
    unsigned long ts = millis() / 1000;
    String  login = esp_chipid,
            token = WiFi.macAddress();
    String our_data = "L=" + login + "&t=" + String(ts, DEC) + "&airrohr=" + data;
    char token_hash[41];
    sha1Hex(token, &token_hash[0]);
    char hash[41];
    hmac1(String(token_hash), our_data+token, &hash[0]);
    char char_full_url[100];
    sprintf(char_full_url, "%s%s", url_our, hash);

    wdt_reset();
    sendData(our_data, pin, host_our, httpPort_our, char_full_url, false, "", contentType);
    wdt_reset();
}

// Проверяет наличие датчика VOC
bool isIaqcorePresent() {
    byte error;
    Wire.beginTransmission(IAQCORE_I2CADDR);
    return Wire.endTransmission() == 0;
}

void initIaqcore() {
    iaqcore_is_present = isIaqcorePresent();
    if (iaqcore_is_present) {
        debug_out(F("iAQcore is present"), DEBUG_MIN_INFO, 1);
        Wire.setClockStretchLimit(2000);
    } else {
        debug_out(F("iAQcore is not present"), DEBUG_MIN_INFO, 1);
    }
    iaqcore.begin();
}

String sensorIaqcore() {
    String s = "";
    // Read
    uint16_t eco2;
    uint16_t stat;
    uint32_t resist;
    uint16_t etvoc;

    debug_out(F("Start reading iAQcore"), DEBUG_MIN_INFO, 1);

    iaqcore.read(&eco2, &stat, &resist, &etvoc);
    if(stat == 0x00 || stat == 0x80) {
        // Print
        debug_out(F("iAQcore: "), DEBUG_MIN_INFO, 0);
        debug_out(F("eco2="), DEBUG_MIN_INFO, 0);
            debug_out(String(eco2), DEBUG_MIN_INFO, 0);
            debug_out(F(" ppm,  "), DEBUG_MIN_INFO, 0);
        debug_out(F("stat=0x"), DEBUG_MIN_INFO, 0);
            debug_out(String(stat, HEX), DEBUG_MIN_INFO, 0);
            debug_out(F(",  "), DEBUG_MIN_INFO, 0);
        debug_out(F("resist="), DEBUG_MIN_INFO, 0);
            debug_out(String(resist), DEBUG_MIN_INFO, 0);
            debug_out(F(" ohm,  "), DEBUG_MIN_INFO, 0);
        debug_out(F("tvoc="), DEBUG_MIN_INFO, 0);
            debug_out(String(etvoc), DEBUG_MIN_INFO, 0);
            debug_out(F(" ppb"), DEBUG_MIN_INFO, 1);

        s += Value2Json(F("iaq_eco2"), String(eco2));
        s += Value2Json(F("iaq_stat"), String(stat));
        s += Value2Json(F("iaq_resist"), String(resist));
        s += Value2Json(F("iaq_tvoc"), String(etvoc));
    } else {
        // Прогревается или занят
    }

    debug_out(F("------"), DEBUG_MIN_INFO, 1);
    debug_out(F("End reading iAQcore"), DEBUG_MIN_INFO, 1);

    return s;
}
