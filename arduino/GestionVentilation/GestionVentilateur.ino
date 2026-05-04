#include <Arduino.h>
#include "credentials-vent.h"

// --- CONFIGURATION PINS ---
#define FAN_RELAY_PIN 23  // Broche S du relais
const int trigPin = 13;   // Ultrason Trig
const int echoPin = 14;   // Ultrason Echo

// --- VARIABLES GLOBALES ---
bool ventiloEstActif = false;
long distancePrecedente = 999;
unsigned long dernierToggle = 0;

// --- FONCTIONS AT ---
String envoyerAT(String commande, unsigned long timeoutMs, int maxRetries = 1) {
  for (int tentative = 0; tentative <= maxRetries; tentative++) {
    if (tentative > 0) {
      Serial.println("[AT] Busy error détectée, 5s puis relance");
      delay(5000); 
      Serial.println("[AT] Tentative " + String(tentative) + "...");
    }

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
    }

    // Si pas de "busy" dans la réponse, on retourne directement
    if (reponse.indexOf("BUSY") == -1) {
      return reponse;
    } 
  }

  // Retourne la dernière réponse même si toujours busy
  Serial.println("[AT] Échec après " + String(maxRetries + 1) + " tentative(s).");
  return "";
}

void configureLoRa() {
  Serial.println("\n--- Test module RAK ---");
  if (envoyerAT("AT", 1000).indexOf("OK") == -1) {
    Serial.println("❌ ÉCHEC : Le RAK ne répond pas ! Vérifiez le câblage TX/RX.");
    Serial.println("Restart dans 5s...");
    delay(5000);
    esp_restart();
  } else {
    Serial.println("✅ Module RAK fonctionnel");
  }

  Serial.println("\n--- CONFIGURATION LORA ---");
  envoyerAT("AT+NWM=1", 1000);   // Mode LoRaWAN
  delay(1000); // Attente la commande précédente peut redémarer l'antenne

  envoyerAT("AT+NJM=1", 500);    // Mode OTAA
  envoyerAT("AT+BAND=4", 500);   // Bande EU868
  envoyerAT("AT+DEVEUI=" + String(DEVEUI), 500);
  envoyerAT("AT+APPEUI=" + String(APPEUI), 500);
  envoyerAT("AT+APPKEY=" + String(APPKEY), 500);
  envoyerAT("AT+CLASS=C", 500);  // On force la Classe C dès le début
  
  Serial.println("✅ Configuration terminée\n");
}

// --- CAPTEUR ULTRASON ---
long getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duree = pulseIn(echoPin, HIGH, 30000);
  return (duree == 0) ? 999 : (duree / 2) * 0.034;
}

void setup() {
  Serial.begin(115200);
  // Initialisation Serial2 pour le RAK (TX=17, RX=16 sur l'ESP32)
  Serial2.begin(115200, SERIAL_8N1, 16, 17); 
  delay(1000);

  // Initialisation des états pour le capteur HC-SR04
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Gestion du relais (pour allumer éteindre les ventilateurs )
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(FAN_RELAY_PIN, LOW); // Éteint par défaut

  // Configuration de la connexion
  configureLoRa();

  Serial.println("Lancement du JOIN...");
  envoyerAT("AT+JOIN=1:1:10:8", 2000);

  unsigned long debutJoin = millis();
  bool joined = false;
  while (millis() - debutJoin < 30000 && !joined) {
    if (Serial2.available()) {
      String rep = Serial2.readStringUntil('\n');
      rep.trim();
      if (rep.indexOf("+EVT:JOINED") != -1) {
        joined = true;
        Serial.println("🚀 RÉSEAU REJOINT !");
        delay(500);
        String repClasse = envoyerAT("AT+CLASS=C", 1000);
        Serial.println("Passage Classe C : " + repClasse);
        String verif = envoyerAT("AT+CLASS=?", 1000);
        Serial.println("Classe active : " + verif);

        // Envoi au TTN pour synchronisation avec le backend 
        const String message = ventiloEstActif ? "01" : "00";
        envoyerAT("AT+SEND=16:"+message, 2000);

        // Message pour passer l'antenne en écoute)
        envoyerAT("AT+SEND=15:", 2000);
      }
    }
  }
}

void loop() {
  // 1. GESTION MANUELLE (ULTRASON)
  long distance = getDistance();
  if (distance < 20 && distancePrecedente >= 20 && millis() - dernierToggle > 1000) {
      ventiloEstActif = !ventiloEstActif;
      digitalWrite(FAN_RELAY_PIN, ventiloEstActif ? HIGH : LOW);
      Serial.print(">>> ACTION MANUELLE : Ventilateur ");
      Serial.println(ventiloEstActif ? "ALLUMÉ" : "ÉTEINT");

      // Envoi au TTN pour synchronisation avec le backend
      const String message = ventiloEstActif ? "01" : "00";
      envoyerAT("AT+SEND=16:"+message, 2000);
  }
  distancePrecedente = distance;

  // 2. GESTION RÉCEPTION LORA (RAK3172)
  if (Serial2.available()) {
    String message = Serial2.readString();
    message.trim();

    // Analyse de l'événement de réception
    // Format typique : +EVT:RX_1:-110:5:UNICAST:15:01
    if (message.indexOf("+EVT:RX") != -1) {
      
      int lastColon = message.lastIndexOf(':'); // Avant le Payload (01)
      int portColon = message.lastIndexOf(':', lastColon - 1); // Avant le Port (15)

      if (lastColon != -1 && portColon != -1) {
        String port = message.substring(portColon + 1, lastColon);
        String data = message.substring(lastColon + 1);
        data.trim();

        // Comparaison des valeurs Hexa reçues de Node-RED
        // 01 ou 31 (ASCII '1') = ON
        if (data == "01" || data == "31") {
          digitalWrite(FAN_RELAY_PIN, HIGH);
          ventiloEstActif = true;
          Serial.println(">>> Modification automatique : VENTILATEUR ALLUMÉ ✅");
        } 
        // 00 ou 30 (ASCII '0') = OFF
        else if (data == "00" || data == "30") {
          digitalWrite(FAN_RELAY_PIN, LOW);
          ventiloEstActif = false;
          Serial.println(">>> Modification automatique : VENTILATEUR ÉTEINT ❌");
        }
      }
    }
  }

  delay(100); 
}