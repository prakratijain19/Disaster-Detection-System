#define BLYNK_TEMPLATE_ID "TMPL3pQcwY-N2"
#define BLYNK_TEMPLATE_NAME "Smart Home System"
#define BLYNK_AUTH_TOKEN "nMjLlUxr7V8pimWZsy6Xg2CMxr45RQJu"

// ---------------------- Libraries ----------------------
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// ---------------------- WiFi Credentials ----------------------
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// ---------------------- OLED Setup ----------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------------- NeoPixel Ring ----------------------
#define NEOPIXEL_PIN 5
#define NUMPIXELS 16
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ---------------------- Sensor Pins ----------------------
#define DHTPIN 15
#define DHTTYPE DHT22
#define GAS_SENSOR_PIN 36
#define LDR_SENSOR_PIN 35
#define PIR_SENSOR_PIN 27
#define POTENTIOMETER_PIN 34
#define BUZZER_PIN 26
#define BUTTON_PIN 32

// ---------------------- Sensor Objects ---------------------
MPU6050 mpu;
DHT dht(DHTPIN, DHTTYPE);

// ---------------------- Threshold Values -------------------
int gasThreshold = 500;
int ldrThreshold = 2000;
int floodThreshold = 3000;
int earthquakeThreshold = 20000;

// ---------------------- Alert Flags -------------------
bool gasAlerted = false;
bool flameAlerted = false;
bool floodAlerted = false;
bool quakeAlerted = false;
bool motionAlerted = false;
bool panicAlerted = false;

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  dht.begin();
  pixels.begin();
  Wire.begin();
  mpu.initialize();

  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Disaster System Ready");
  display.display();
  delay(1000);
}

void loop() {
  Blynk.run();

  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  int gas = analogRead(GAS_SENSOR_PIN);
  int ldr = analogRead(LDR_SENSOR_PIN);
  int flood = analogRead(POTENTIOMETER_PIN);
  int motion = digitalRead(PIR_SENSOR_PIN);
  bool panicPressed = digitalRead(BUTTON_PIN) == LOW;

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float accel = sqrt(ax * ax + ay * ay + az * az);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Disaster Monitor");
  display.print("Temp: "); display.print(temp); display.println(" C");
  display.print("Hum : "); display.print(humidity); display.println(" %");
  display.print("Gas : "); display.println(gas);
  display.print("Flame: "); display.println(ldr);
  display.print("Flood: "); display.println(flood);
  display.display();

  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, gas);
  Blynk.virtualWrite(V3, flood);
  Blynk.virtualWrite(V4, ldr);
  Blynk.virtualWrite(V5, motion);

  pixels.clear();
  pixels.show();
  digitalWrite(BUZZER_PIN, LOW);

  // GAS ALERT
  if (gas > gasThreshold && !gasAlerted) {
    alert("GAS LEAK!", 255, 0, 0);
    Blynk.logEvent("gas_alert", "Gas leak detected!");
    gasAlerted = true;
  } else if (gas <= gasThreshold) gasAlerted = false;

  // FLAME ALERT
  if (ldr < ldrThreshold && !flameAlerted) {
    alert("FLAME DETECTED!", 255, 50, 0);
    Blynk.logEvent("flame_alert", "Flame or fire detected!");
    flameAlerted = true;
  } else if (ldr >= ldrThreshold) flameAlerted = false;

  // FLOOD ALERT
  if (flood > floodThreshold && !floodAlerted) {
    alert("FLOOD ALERT!", 0, 0, 255);
    Blynk.logEvent("flood_alert", "Flood level exceeded!");
    floodAlerted = true;
  } else if (flood <= floodThreshold) floodAlerted = false;

  // EARTHQUAKE ALERT
  if (accel > earthquakeThreshold && !quakeAlerted) {
    alert("EARTHQUAKE!", 255, 255, 0);
    Blynk.logEvent("quake_alert", "Earthquake detected!");
    quakeAlerted = true;
  } else if (accel <= earthquakeThreshold) quakeAlerted = false;

  // MOTION ALERT
  if (motion && !motionAlerted) {
    alert("INTRUSION!", 255, 0, 255);
    Blynk.logEvent("motion_alert", "Motion detected - Possible intrusion!");
    motionAlerted = true;
  } else if (!motion) motionAlerted = false;

  // PANIC BUTTON ALERT
  if (panicPressed && !panicAlerted) {
    alert("PANIC BUTTON!", 0, 255, 0);
    Blynk.logEvent("panic_button", "Panic button was pressed!");
    panicAlerted = true;
  } else if (!panicPressed) panicAlerted = false;

  delay(500);
}

// ---------------- ALERT FUNCTION ----------------
void alert(String message, int r, int g, int b) {
  Serial.println("ALERT: " + message);

  // OLED Alert Message
  display.fillRect(0, 55, SCREEN_WIDTH, 10, BLACK);
  display.setCursor(0, 55);
  display.setTextColor(SSD1306_WHITE);
  display.print("ALERT: ");
  display.println(message);
  display.display();

  // NeoPixel Light Effect
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();

  // Buzzer
  digitalWrite(BUZZER_PIN, HIGH);
}
