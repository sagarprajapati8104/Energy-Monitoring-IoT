#include <WiFi.h>
#include <HTTPClient.h>
#include <PZEM004Tv30.h>

// ========== WiFi Credentials ==========
const char* ssid = "****";
const char* password = "****";

// ========== InfluxDB Cloud Settings ==========
const char* influxUrl   = "*****";
const char* influxToken = "*****";

// ========== PZEM Configuration ==========
#define RXD2 16
#define TXD2 17
PZEM004Tv30 pzem(Serial2, RXD2, TXD2);

// ======= Function to connect/reconnect Wi-Fi =======
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to WiFi ");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi connection failed!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 + PZEM-004T + InfluxDB Cloud");

  connectWiFi();
}

void loop() {
  connectWiFi();

  // Read all 6 PZEM parameters
  float voltage   = pzem.voltage();
  float current   = pzem.current();
  float power     = pzem.power();
  float energy    = pzem.energy();
  float frequency = pzem.frequency();
  float pf        = pzem.pf();

  // Show on Serial Monitor
  Serial.println("\n------------------------------------------");
  Serial.println("⚡ PZEM-004T Readings ⚡");
  Serial.printf("Voltage     : %.2f V\n", voltage);
  Serial.printf("Current     : %.3f A\n", current);
  Serial.printf("Power       : %.2f W\n", power);
  Serial.printf("Energy      : %.3f kWh\n", energy);
  Serial.printf("Frequency   : %.2f Hz\n", frequency);
  Serial.printf("Power Factor: %.2f\n", pf);
  Serial.println("------------------------------------------");

  // Prepare Line Protocol with a tag
  if (!isnan(voltage) && WiFi.status() == WL_CONNECTED) {
    String postData = "pzem_data,device=ESP32 "
                      "voltage=" + String(voltage,2) + "," +
                      "current=" + String(current,3) + "," +
                      "power=" + String(power,2) + "," +
                      "energy=" + String(energy,3) + "," +
                      "frequency=" + String(frequency,2) + "," +
                      "pf=" + String(pf,2);

    HTTPClient http;
    http.begin(influxUrl);
    http.addHeader("Authorization", String("Token ") + influxToken);
    http.addHeader("Content-Type", "text/plain");

    int httpCode = http.POST(postData);
    if (httpCode > 0) {
      Serial.printf("✅ Data sent! HTTP code: %d\n", httpCode);
    } else {
      Serial.printf("❌ Failed to send data! Error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("⚠️ WiFi not connected or invalid reading!");
  }

  delay(20000); // 20s delay to match Telegraf config
}
