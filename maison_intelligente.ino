#include <DHT.h>
#include <LiquidCrystal.h>


#define DHT_PIN      2
#define DHT_TYPE     DHT11
#define PIR_PIN      3
#define LED_PIN      4
#define LED_ALARM    5
#define BTN_PIN      13


LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
DHT dht(DHT_PIN, DHT_TYPE);


bool  lightOn        = false;
bool  alarmActive    = false;
bool  autoMode       = true;
float temperature    = 0.0;
float humidity       = 0.0;
bool  motionDetected = false;


unsigned long lastDHTRead    = 0;
unsigned long lastLCDUpdate  = 0;
unsigned long motionTimeout  = 0;
unsigned long lastBtnPress   = 0;
unsigned long lastSerialSend = 0;

const unsigned long DHT_INTERVAL   = 2000;
const unsigned long LCD_INTERVAL   = 1000;
const unsigned long MOTION_DELAY   = 10000;
const unsigned long DEBOUNCE_MS    = 200;
const unsigned long SERIAL_PERIOD  = 5000;
const float TEMP_THRESHOLD = 30.0;


void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16, 2);

  pinMode(PIR_PIN,    INPUT);
  pinMode(LED_PIN,    OUTPUT);
  pinMode(LED_ALARM,  OUTPUT);
  pinMode(BTN_PIN,    INPUT_PULLUP);

  lcd.setCursor(0, 0); lcd.print("  Maison Smart  ");
  lcd.setCursor(0, 1); lcd.print(" Initialisation ");
  delay(2000);
  lcd.clear();

  Serial.println("=========================================");
  Serial.println("  Systeme Maison Intelligente - Pret !  ");
  Serial.println("  Commandes serie :                     ");
  Serial.println("   L  -> Basculer lumiere               ");
  Serial.println("   A  -> Basculer mode Auto/Manuel      ");
  Serial.println("   R  -> Reset alarme                   ");
  Serial.println("=========================================");
}


void loop() {
  unsigned long now = millis();

  handleSerial();
  handleButton(now);
  readDHT(now);
  handlePIR(now);
  controlLight();
  controlAlarm();
  updateLCD(now);
  sendSerialStatus(now);
}


void handleSerial() {
  if (!Serial.available()) return;
  char cmd = Serial.read();
  switch (cmd) {
    case 'L': case 'l':
      lightOn = !lightOn;
      autoMode = false;
      Serial.print("[CMD] Lumiere -> ");
      Serial.println(lightOn ? "ON" : "OFF");
      break;
    case 'A': case 'a':
      autoMode = !autoMode;
      Serial.print("[CMD] Mode -> ");
      Serial.println(autoMode ? "AUTO" : "MANUEL");
      break;
    case 'R': case 'r':
      alarmActive = false;
      Serial.println("[CMD] Alarme resetee");
      break;
    default:
      break;
  }
}


void handleButton(unsigned long now) {
  if (digitalRead(BTN_PIN) == LOW && (now - lastBtnPress > DEBOUNCE_MS)) {
    lastBtnPress = now;
    if (autoMode) {
      autoMode = false;
      lightOn  = !lightOn;
      Serial.print("[BTN] Mode Manuel - Lumiere ");
      Serial.println(lightOn ? "ON" : "OFF");
    } else {
      lightOn = !lightOn;
      Serial.print("[BTN] Lumiere ");
      Serial.println(lightOn ? "ON" : "OFF");
      if (!lightOn) autoMode = true;
    }
  }
}

void readDHT(unsigned long now) {
  if (now - lastDHTRead < DHT_INTERVAL) return;
  lastDHTRead = now;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity    = h;
  } else {
    Serial.println("[DHT] Erreur de lecture !");
  }
}


void handlePIR(unsigned long now) {
  motionDetected = digitalRead(PIR_PIN);

  if (motionDetected) {
    motionTimeout = now;
    if (autoMode) {
      if (!lightOn) Serial.println("[PIR] Mouvement -> Lumiere ON");
      lightOn = true;
    }
    if (!alarmActive) {
      alarmActive = true;
      Serial.println("[PIR] Mouvement detecte -> Alarme active");
    }
  }


  if (autoMode && lightOn && (now - motionTimeout > MOTION_DELAY)) {
    lightOn = false;
    Serial.println("[PIR] Timeout -> Lumiere OFF");
  }
}


void controlLight() {
  digitalWrite(LED_PIN, lightOn ? HIGH : LOW);
}


unsigned long lastBlinkTime = 0;
bool blinkState = false;

void controlAlarm() {
  bool tempAlarm = (temperature >= TEMP_THRESHOLD);
  unsigned long now = millis();

  if (tempAlarm || alarmActive) {
   unsigned long interval = tempAlarm ? 200 : 400;
    if (now - lastBlinkTime >= interval) {
      lastBlinkTime = now;
      blinkState = !blinkState;
      digitalWrite(LED_ALARM, blinkState ? HIGH : LOW);
    }

    if (tempAlarm) {
 
    }
    if (!tempAlarm) alarmActive = false;
  } else {
    digitalWrite(LED_ALARM, LOW);
    blinkState = false;
  }
}


void updateLCD(unsigned long now) {
  if (now - lastLCDUpdate < LCD_INTERVAL) return;
  lastLCDUpdate = now;

 
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temperature, 1);
  lcd.print((char)223); lcd.print("C H:");
  lcd.print((int)humidity); lcd.print("%   ");

  
  lcd.setCursor(0, 1);
  lcd.print("L:"); lcd.print(lightOn ? "ON " : "OFF");
  lcd.print(" M:"); lcd.print(motionDetected ? "OUI" : "NON");
  lcd.print(" "); lcd.print(autoMode ? "A" : "M");
}


void sendSerialStatus(unsigned long now) {
  if (now - lastSerialSend < SERIAL_PERIOD) return;
  lastSerialSend = now;

  Serial.println("----- Etat systeme -----");
  Serial.print("Temp     : "); Serial.print(temperature); Serial.println(" C");
  Serial.print("Humidite : "); Serial.print(humidity);    Serial.println(" %");
  Serial.print("Lumiere  : "); Serial.println(lightOn ? "ON" : "OFF");
  Serial.print("Mouvement: "); Serial.println(motionDetected ? "OUI" : "NON");
  Serial.print("Mode     : "); Serial.println(autoMode ? "AUTO" : "MANUEL");
  Serial.print("Alarme   : "); Serial.println(alarmActive ? "ACTIVE" : "OK");
  Serial.println("------------------------");
}
