#include <Arduino.h>
#include "credentials.h"

// -- CONFIGURATION BUZZER
#define BUZZER_PIN 22
const int BUTTON_PIN = 5;

// --- CONFIGURATION PINS ---
#define FAN_RELAY_PIN 23  // Broche S du relais
const int trigPin = 13;   // Ultrason Trig
const int echoPin = 14;   // Ultrason Echo

// --- VARIABLES GLOBALES ---
bool ventiloEstActif = false;
long distancePrecedente = 999;
bool buzzerEstActif = false;
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

void makeBuzzerAlarm() {
  // Fréquences des notes pour les accords
  int Do4 = 262, Mi4 = 330, Sol4 = 392; // Accord de Do (C)
  int Fa4 = 349, La4 = 440, Do5 = 523;  // Accord de Fa (F)
  int Sol4_bas = 392, Si4 = 494, Re5 = 587; // Accord de Sol (G)
  int Do5_aigu = 523, Mi5 = 659, Sol5 = 784; // Accord de Do aigu (C)

  // 1. Accord de Do Majeur 
  playChord(Do4, Mi4, Sol4, 500); // Joue pendant 500ms
  delay(50); // Petit silence pour détacher les accords

  // 2. Accord de Fa Majeur 
  playChord(Fa4, La4, Do5, 500);
  delay(50);

  // 3. Accord de Sol Majeur 
  playChord(Sol4_bas, Si4, Re5, 500);
  delay(50);

  // 4. RÉSOLUTION FINALE : Accord de Do Majeur Aigu 
  playChord(Do5_aigu, Mi5, Sol5, 800); // Dure un peu plus longtemps (800ms)
}

// Fonction qui simule un accord en jouant 3 notes très vite
void playChord(int note1, int note2, int note3, int dureeTotale) {
  // Chaque boucle prend 60 ms (3 notes x 20 ms)
  int nombreDeBoucles = dureeTotale / 60; 
  
  for (int i = 0; i < nombreDeBoucles; i++) {
    tone(BUZZER_PIN, note1); delay(20);
    tone(BUZZER_PIN, note2); delay(20);
    tone(BUZZER_PIN, note3); delay(20);
  }
  noTone(BUZZER_PIN); // Coupe le son proprement à la fin
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
      envoyerAT("AT+NWM=1", 1000); //  Network Working Mode : LoRaWAN (0 = P2P)
      delay(3000); // Attendre le reboot du RAK
      while (Serial2.available()) Serial2.read(); // Vider le message de boot
  }

  // Vérifier NJM avant d'écrire (évite AT_ERROR si déjà jointé)
  if (envoyerAT("AT+NJM=?", 500).indexOf("AT+NJM=1") == -1) {
      envoyerAT("AT+NJM=1", 500); // Network Join Mode : OTAA (Over-The-Air Activation)
  }
  envoyerAT("AT+BAND=4", 500);   // Norme EU868
  envoyerAT("AT+DEVEUI=" + String(DEVEUI), 500);  // Identifiant unique du module
  envoyerAT("AT+APPEUI=" + String(APPEUI), 500); // Identifiant de l'application
  envoyerAT("AT+APPKEY=" + String(APPKEY), 500); // Configuration OTAA 
  envoyerAT("AT+CLASS=A", 500);  // Class a pour le join
  
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
  // Gestion alarme
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
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

  // Vide le buffer pour éviter les buzy error
  while (Serial2.available()) Serial2.read(); 
  Serial.println("Lancement du JOIN...");
  // AT+JOIN=1:0 → join une fois, PAS d'auto-rejoin
  envoyerAT("AT+JOIN=1:0:10:8", 2000);

  unsigned long debutJoin = millis();
  bool joined = false;
  while (millis() - debutJoin < 30000 && !joined) {
    if (Serial2.available()) {
      String rep = Serial2.readStringUntil('\n');
      rep.trim();
      if (rep.indexOf("+EVT:JOINED") != -1 && rep.indexOf("+EVT:JOINED_FAIL") == -1) {
        joined = true;
        Serial.println("🚀 RÉSEAU REJOINT !");
        delay(500);
        String repClasse = envoyerAT("AT+CLASS=C", 1000);
        Serial.println("Passage Classe C : " + repClasse);
        String verif = envoyerAT("AT+CLASS=?", 1000);
        Serial.println("Classe active : " + verif);
        delay(5000);

        // Envoi au TTN pour synchronisation avec le backend 
        const String message = ventiloEstActif ? "01" : "00";
        envoyerAT("AT+SEND=16:"+message, 2000);

        Serial.println("✅ Le module ventilateur est prêt");
      }
    }
  }
}

void loop() {
  // Extinction de l'alarme
  if (digitalRead(BUTTON_PIN) == LOW && buzzerEstActif) {
    Serial.println("Alarme désactivée");
    buzzerEstActif = false;
  }

  // Lancement de l'alarme
  if (buzzerEstActif) {
    makeBuzzerAlarm();
  }


  // GESTION MANUELLE (ULTRASON)
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

  // GESTION RÉCEPTION LORA (RAK3172)
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
        // O2 = ON + ALARME
        if ( data == "01" || data == "31" || data=="02") {
          digitalWrite(FAN_RELAY_PIN, HIGH);
          ventiloEstActif = true;
          Serial.println(">>> Modification automatique : VENTILATEUR ALLUMÉ ✅");
          if (data=="02") {
            buzzerEstActif = true;
          }
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