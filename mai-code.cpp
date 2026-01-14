#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/RTDBHelper.h>

/* ================= WIFI ================= */
#define WIFI_SSID     "xxx"
#define WIFI_PASSWORD "xxx"

/* ================= FIREBASE ================= */
#define API_KEY "xxx"
#define DATABASE_URL "xxx"
#define DATABASE_SECRET "xxx"

/* ================= PIN ESP32 ================= */
#define TRIG_PIN 5
#define ECHO_PIN 18
#define RELAY_PIN 23

/* ================= THRESHOLD ================= */
#define WATER_LIMIT_CM 10

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/* ================= HC-SR04 ================= */
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  /* ===== WIFI ===== */
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected");

  /* ===== FIREBASE ===== */
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase connected");
}

void loop() {

  /* ===== BACA SENSOR ===== */
  long distance = readDistanceCM();

  if (distance > 0) {
    Firebase.RTDB.setInt(&fbdo, "/sensor/distance_cm", distance);

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  /* ===== SAFETY THRESHOLD ===== */
  if (distance > 0 && distance < WATER_LIMIT_CM) {
    digitalWrite(RELAY_PIN, HIGH);

    Serial.println("Water level HIGH → Pump FORCED OFF");

    delay(2000);
    return;
  }

  /* ===== KONTROL MANUAL DARI FIREBASE ===== */
  if (Firebase.RTDB.getString(&fbdo, "/pump/status")) {
    String status = fbdo.stringData();

    Serial.print("Pump status (Firebase): ");
    Serial.println(status);

    if (status == "ON") {
      digitalWrite(RELAY_PIN, LOW);   // Pump ON
    } else {
      digitalWrite(RELAY_PIN, HIGH);  // Pump OFF
    }
  } else {
    Serial.print("Firebase read error: ");
    Serial.println(fbdo.errorReason());
  }

  delay(2000);
}
