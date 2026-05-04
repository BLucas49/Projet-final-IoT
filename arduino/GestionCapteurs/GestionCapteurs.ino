#include <DHT.h>
#include "credentials.h"
#include "esp_sleep.h"

#define RXD2 16    // GPIO16 -> branché sur TX du RAK3272S
#define TXD2 17    // GPIO17 -> branché sur RX du RAK3272S
#define LED1 18    // LED Verte
#define LED2 19    // LED Jaune
#define LED3 21    // LED Bleue
#define DHTPIN 5   // port du capteur de température et d'humidité
#define BUZZPIN 4  // port du buzzer pour l'envoie
#define LDR_PIN 27 // port du capteur de luminiosité

#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const uint32_t intervalleEnvoi = 30000;

// permet d'envoyer une commande lora
String envoyerAT(String commande, unsigned long timeoutMs)
{
    Serial.println("[AT] → " + commande);
    Serial2.println(commande);

    String reponse = "";
    unsigned long debut = millis();
    while (millis() - debut < timeoutMs)
    {
        while (Serial2.available())
        {
            String ligne = Serial2.readStringUntil('\n');
            ligne.trim();
            if (ligne.length() > 0)
            {
                Serial.println("[AT] ← " + ligne);
                reponse += ligne + "\n";
            }
        }
        delay(20);
    }
    return reponse;
}

void configureLoRa()
{
    envoyerAT("AT+NWM=1", 2500); // Mode LoRaWAN (déclenche un reboot du module)
    envoyerAT("AT+NJM=1", 500);  // Mode OTAA
    envoyerAT("AT+BAND=4", 500); // Bande EU868

    envoyerAT("AT+DEVEUI=" + String(DEVEUI), 500);
    envoyerAT("AT+APPEUI=" + String(APPEUI), 500);
    envoyerAT("AT+APPKEY=" + String(APPKEY), 500);

    // Vérification (lecture des valeurs configurées)
    Serial.println("\nVérification configuration :");
    envoyerAT("AT+DEVEUI=?", 500);
    envoyerAT("AT+NJM=?", 500);
    envoyerAT("AT+BAND=?", 500);
    Serial.println("✅ Configuration terminée\n");
}

// Fonction pour encoder Température, Humidité ET Luminosité en hexadécimal
String encodePayload(float temp, float hum, int lum)
{
    // 1. Température : * 100 et conversion en int16_t (2 octets)
    int16_t tempInt = (int16_t)(temp * 100.0);
    char hexTemp[5];
    sprintf(hexTemp, "%04X", (uint16_t)tempInt); // 4 caractères hexa

    // 2. Humidité : conversion directe en uint8_t (1 octet)
    uint8_t humInt = (uint8_t)hum;
    char hexHum[3];
    sprintf(hexHum, "%02X", humInt); // 2 caractères hexa

    // 3. Luminosité : conversion en uint16_t (2 octets)
    uint16_t lumInt = (uint16_t)lum;
    char hexLum[3];
    sprintf(hexLum, "%02X", lumInt);

    // Combine les trois (ex: 09293208FC) -> Temp (2 octets) + Hum (1 octet) + Lum (2 octets)
    return String(hexTemp) + String(hexHum) + String(hexLum);
}

void sendLoRaMessage(int port, String hexPayload)
{
    String command = "AT+SEND=" + String(port) + ":" + hexPayload;
    Serial.print("Envoi au RAK : ");
    Serial.println(command);
    Serial2.print(command + "\r\n");
}

void makeLedAnimation()
{
    // Fréquences des notes pour nos accords
    int Do4 = 262, Mi4 = 330, Sol4 = 392;      // Accord de Do (C)
    int Fa4 = 349, La4 = 440, Do5 = 523;       // Accord de Fa (F)
    int Sol4_bas = 392, Si4 = 494, Re5 = 587;  // Accord de Sol (G)
    int Do5_aigu = 523, Mi5 = 659, Sol5 = 784; // Accord de Do aigu (C)
    // jouerFurElise();
    // 1. Accord de Do Majeur (LED 1)
    digitalWrite(LED1, HIGH);
    playChord(Do4, Mi4, Sol4, 500); // Joue pendant 500ms
    digitalWrite(LED1, LOW);
    delay(50); // Petit silence pour détacher les accords

    digitalWrite(LED2, HIGH);
    digitalWrite(LED2, LOW);
    delay(50);

    digitalWrite(LED3, HIGH);
    playChord(Sol4_bas, Si4, Re5, 500);
    digitalWrite(LED3, LOW);
    delay(50);

    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    playChord(Do5_aigu, Mi5, Sol5, 800);

    // On éteint tout
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
}

// Fonction qui simule un accord en jouant 3 notes très vite
void playChord(int note1, int note2, int note3, int dureeTotale)
{
    // Chaque boucle prend 60 ms (3 notes x 20 ms)
    int nombreDeBoucles = dureeTotale / 60;

    for (int i = 0; i < nombreDeBoucles; i++)
    {
        tone(BUZZPIN, note1);
        delay(20);
        tone(BUZZPIN, note2);
        delay(20);
        tone(BUZZPIN, note3);
        delay(20);
    }
    noTone(BUZZPIN); // Coupe le son proprement à la fin
}

void manageSensors()
{
    // Lecture des capteurs
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int lum = analogRead(LDR_PIN);

    // Vérification que la lecture du DHT a fonctionné
    if (isnan(h) || isnan(t))
    {
        Serial.println("Erreur de lecture du DHT11 !");
    }
    else
    {
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
void setup()
{
    Serial.begin(115200);                          // Port USB vers PC
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); // Port vers RAK3272S

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LDR_PIN, INPUT);
    pinMode(BUZZPIN, OUTPUT);

    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(BUZZPIN, LOW);
    delay(500);

    Serial.println("=== Passerelle Serie ESP32 - RAK3272S ===");
    Serial.println("Tapez AT puis Entree pour tester");

    dht.begin();
    configureLoRa();
    Serial.println("Lancement de la connexion LoRaWAN (Join)...");
    envoyerAT("AT+JOIN=1:0:10:8", 1000);
    delay(2000);
}

void loop()
{
    manageSensors();
    makeLedAnimation();

    // Mise en veille légère pendant ~30 secondes
    Serial.println("💤 Entrée en light sleep...");
    Serial.flush(); // Important : vider le buffer avant de dormir

    esp_sleep_enable_timer_wakeup((uint64_t)(intervalleEnvoi - 500) * 1000ULL);
    esp_light_sleep_start();

    Serial.println("⏰ Réveil !");
}
