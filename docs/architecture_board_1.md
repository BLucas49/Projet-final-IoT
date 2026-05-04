# Architecture de la board de gestion des capteurs

> Remarque : cette architecture fonctionne avec le code du fichier GestionCapteurs.ino (/code/...)

## Objectifs

L'objectif de ce montage est de gérer les capteurs et de retransmettre les données au serveur TTN.
Il capte trois métriques :

- La luminosité (captée par le capteur LDR)
- La température (captée par le capteur DHT11)
- L'humidité (captée par le capteur DHT11)

L'envoi de données est signalé par une animation led et un buzzer.

## Configuration générale

Sur l'esp32, la PIN VIN (5v) est reliée à un rail + de la board (appelé **5v** dans la suite de cette documentation). La PIN GND est reliée à un rail - de la board (appelé **GND** dans la suite de cette documentation).

> Il est nécessaire de faire le branchement du module LoRa (RAK3172) avant de continuer (voir /docs/architecture.md)

## Paramètre LORAWAN

| Parametre        | Valeur                      | Justification                                                                           |
| ---------------- | --------------------------- | --------------------------------------------------------------------------------------- |
| Mode             | OTAA                        | Plus sécurisé qu'ABP (re-négociation des clés à chaque join)                            |
| Classe           | C                           | Écoute permanente, nécessaire pour recevoir des commandes depuis Node-RED à tout moment |
| Spreading Factor | SF7                         | Portée courte, débit maximal, latence minimale                                          |
| Bande            | EU868                       | Europe (8 canaux, 125 kHz BW)                                                           |
| Duty cycle       | 1%                          | Réglementation ETSI EU868                                                               |
| DevEUI           | Défini dans `credentials.h` | Identifiant unique du device, fourni par TTN                                            |
| AppEUI           | Défini dans `credentials.h` | Identifiant de l'application TTN                                                        |
| AppKey           | Défini dans `credentials.h` | Clé de chiffrement OTAA, à ne pas partager                                              |

## Configuration capteurs

### Capteur DHT11

#### Présentation

Le DHT11 est un capteur numérique qui mesure simultanément la température et l'humidité ambiante, et transmet ces données via un signal numérique sur une seule broche de données.

#### Branchements

1. Relier la broche + au **5v**
2. Relier la broche - au **GND**
3. Relier la broche out à la PIN D5 de l'esp32

### Capteur LDR

#### Présentation

La LDR (Light Dependent Resistor) est un capteur de luminosité monté sur module qui délivre un signal numérique dont le seuil de déclenchement est ajustable via le potentiomètre intégré sur la carte.

#### Branchements

1. Relier la broche VCC au **5v**
2. Relier la broche GND au **GND**
3. Relier la broche DO à la PIN D27 de l'esp32

> Remarque : une fois le montage fini, il est possible de vérifier le fonctionnement du capteur. Poser votre doigt sur le capteur, une led de la carte doit s'éteindre.

### LEDS

#### Présentation

La LED (Light Emitting Diode) est un composant lumineux qui émet de la lumière lorsqu'elle est parcourue par un courant électrique

#### Branchements

##### Led verte

1. Relier la broche + (la plus grande) à la PIN D21 de l'esp32
1. Relier la broche - (la plus petite) au **GND**

##### Led jaune

1. Relier la broche + (la plus grande) à la PIN D19 de l'esp32
1. Relier la broche - (la plus petite) au **GND**

##### Led bleue

1. Relier la broche + (la plus grande) à la PIN D18 de l'esp32
1. Relier la broche - (la plus petite) au **GND**

## Encodage du payload

**Convention :** les valeurs sont encodées en hexadécimal et concaténées dans l'ordre suivant : Température (2 octets) + Humidité (1 octet) + Luminosité (1 octet)

| Grandeur    | Exemple  | Calcul                       | Payload hex |
| ----------- | -------- | ---------------------------- | ----------- |
| Température | 23.29 °C | 23.29 \* 100 = 2329 = 0x0919 | `0919`      |
| Humidité    | 50 %     | 50 = 0x32                    | `32`        |
| Luminosité  | 75 %     | 75 = 0x4B                    | `4B`        |

**Exemple complet :** `09193248` → Temp: 23.29°C, Hum: 50%, Lum: 75%

> Remarque : la température est signée (int16_t) pour supporter les valeurs négatives. Les autres grandeurs sont non signées (uint8_t).
