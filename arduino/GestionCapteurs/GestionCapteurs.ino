#include <DHT.h>
#include "credentials.h"
// Pont série ESP32-D <-> RAK3272S
// UART0 (Serial)  = communication PC via USB-C
// UART2 (Serial2) = communication RAK3272S via GPIO16/17

#define RXD2 16  // GPIO16 -> branché sur TX du RAK3272S
#define TXD2 17  // GPIO17 -> branché sur RX du RAK3272S
#define LED1 18
#define LED2 19
#define LED3 21
#define DHTPIN 5
#define LDR_PIN 27

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

unsigned long dernierEnvoi = 0;
const unsigned long intervalleEnvoi = 30000;

String envoyerAT(String commande, unsigned long timeoutMs) {
  Serial.println("[AT] → " + commande);
  Serial2.println(commande);

  String reponse = "";
  unsigned long debut = millis();
  while (millis() - debut < timeoutMs) {
    while (Serial2.available()) {
      String ligne = Serial2.readStringUntil('\n');
      ligne.trim();
      if (ligne.length() > 0) {
        Serial.println("[AT] ← " + ligne);
        reponse += ligne + "\n";
      }
    }
    delay(20);
  }
  return reponse;
}

void configureLoRa() {
  Serial.println("\n--- Test module RAK ---");
  // Vider le buffer résiduel du boot précédent
  delay(2000);
  while (Serial2.available()) Serial2.read();
  if (envoyerAT("AT", 1000).indexOf("OK") == -1) {
    Serial.println("❌ ÉCHEC : Le RAK ne répond pas ! Vérifiez le câblage TX/RX.");
    Serial.println("Restart dans 5s...");
    delay(5000);
    esp_restart();
  } else {
    Serial.println("✅ Module RAK fonctionnel");
  }

  Serial.println("\n--- CONFIGURATION LORA ---");
  // Stopper tout join en cours AVANT de configurer
  envoyerAT("AT+JOIN=0", 1000);
  delay(500);
  // Vérifier NWM avant d'écrire (évite le reboot inutile)
  if (envoyerAT("AT+NWM=?", 500).indexOf("AT+NWM=1") == -1) {
      envoyerAT("AT+NWM=1", 1000);
      delay(3000); // Attendre le reboot du RAK
      while (Serial2.available()) Serial2.read(); // Vider le message de boot
  }

  // Vérifier NJM avant d'écrire (évite AT_ERROR si déjà jointé)
  if (envoyerAT("AT+NJM=?", 500).indexOf("AT+NJM=1") == -1) {
      envoyerAT("AT+NJM=1", 500);
  }
  envoyerAT("AT+BAND=4", 500);   // Bande EU868
  envoyerAT("AT+DEVEUI=" + String(DEVEUI), 500);
  envoyerAT("AT+APPEUI=" + String(APPEUI), 500);
  envoyerAT("AT+APPKEY=" + String(APPKEY), 500);
  
  Serial.println("✅ Configuration terminée\n");
}

// Fonction pour encoder Température, Humidité ET Luminosité en hexadécimal
String encodePayload(float temp, float hum, int lum) {
  // 1. Température : * 100 et conversion en int16_t (2 octets)
  int16_t tempInt = (int16_t)(temp * 100.0);
  char hexTemp[5];
  sprintf(hexTemp, "%04X", (uint16_t)tempInt);  // 4 caractères hexa

  // 2. Humidité : conversion directe en uint8_t (1 octet)
  uint8_t humInt = (uint8_t)hum;
  char hexHum[3];
  sprintf(hexHum, "%02X", humInt);  // 2 caractères hexa

  // 3. Luminosité : conversion en uint16_t (2 octets)
  uint16_t lumInt = (uint16_t)lum;
  char hexLum[3];
  sprintf(hexLum, "%02X", lumInt);

  // Combine les trois (ex: 09293208FC) -> Temp (2 octets) + Hum (1 octet) + Lum (2 octets)
  return String(hexTemp) + String(hexHum) + String(hexLum);
}

void sendLoRaMessage(int port, String hexPayload) {
  String command = "AT+SEND=" + String(port) + ":" + hexPayload;
  Serial.print("Envoi au RAK : ");
  Serial.println(command);
  Serial2.print(command + "\r\n");
}

void makeLedAnimation() {
  digitalWrite(LED1, HIGH);
  delay(500);
  digitalWrite(LED1, LOW);

  digitalWrite(LED2, HIGH);
  delay(500);
  digitalWrite(LED2, LOW);

  digitalWrite(LED3, HIGH);
  delay(500);
  digitalWrite(LED3, LOW);

  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
  delay(500);
  
  // On éteint tout
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
}

void manageSensors() {
  dernierEnvoi = millis();

  // Lecture des capteurs
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int lum = analogRead(LDR_PIN);

  // Vérification que la lecture du DHT a fonctionné
  if (isnan(h) || isnan(t)) {
    Serial.println("Erreur de lecture du DHT11 !");
  } else {
    lum = map(lum, 4095, 0, 0, 100);
    Serial.print("Temp: ");
    Serial.print(t);
    Serial.print("°C, ");
    Serial.print("Hum: ");
    Serial.print(h);
    Serial.print("%, ");
    Serial.print("Lum: ");
    Serial.println(lum);


    // Encoder et envoyer avec le nouveau paramètre
    String payload = encodePayload(t, h, lum);
    sendLoRaMessage(1, payload);
  }
}

// méthodes principales
void setup() {
  Serial.begin(115200);                           // Port USB vers PC
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);  // Port vers RAK3272S

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  delay(500);

  Serial.println("=== Passerelle Serie ESP32 - RAK3272S ===");
  Serial.println("Tapez AT puis Entree pour tester");

  dht.begin();
  configureLoRa();
  Serial.println("Lancement de la connexion LoRaWAN (Join)...");
  envoyerAT("AT+JOIN=1:0:10:8", 1000);

  unsigned long debutJoin = millis();
  bool joined = false;
  while (millis() - debutJoin < 30000 && !joined) {
    if (Serial2.available()) {
      String rep = Serial2.readStringUntil('\n');
      rep.trim();
      if (rep.indexOf("+EVT:JOINED") != -1 && rep.indexOf("+EVT:JOINED_FAIL") == -1) {
        joined = true;
        Serial.println("🚀 RÉSEAU REJOINT !");
      }
    }
  }
  delay(2000);
}

void loop() {
  if (millis() - dernierEnvoi >= intervalleEnvoi) {
    manageSensors();
    makeLedAnimation();
  }

  // --- PARTIE PONT SÉRIE ---
  while (Serial.available()) {
    Serial2.write(Serial.read());
  }
  while (Serial2.available()) {
    Serial.write(Serial2.read());
  }
}