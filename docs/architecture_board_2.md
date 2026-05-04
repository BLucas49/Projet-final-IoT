# Architecture de la board de gestion des ventilateurs

> Remarque : cette architecture fonctionne avec le code du fichier GestionVentilateur.ino (/code/...)

## Objectifs

L'objectif de ce montage est de gérer l'activation du système de ventilation.
Il peut être activé de deux manières :

- Via action manuelle (captée par le capteur HC-SR04)
- De manière automatisée (commandée depuis le TTN)

## Configuration générale

Sur l'esp32, la PIN VIN (5v) est reliée à un rail + de la board (appelé **5v** dans la suite de cette documentation). La PIN GND est reliée à une rail - de la board (appelé **GND** dans la suite de cette documentation).
Un rail de la board est dédié à l'allumage des ventilateurs. Dans la suite de cette documentation il sera nommé **signal**

> Il est nécessaire de faire le branchement du module LoRa (RAK3172) avant de continuer (voir /docs/architecture.md)

## Paramètre LORAWAN

| Parametre        | Valeur                      | Justification                                                   |
| ---------------- | --------------------------- | --------------------------------------------------------------- |
| Mode             | OTAA                        | Plus sécurisé qu'ABP (re-négociation des clés à chaque join)    |
| Classe           | A                           | Envoi périodique uniquement, pas besoin de réception permanente |
| Spreading Factor | SF7                         | Portée courte, débit maximal, latence minimale                  |
| Bande            | EU868                       | Europe (8 canaux, 125 kHz BW)                                   |
| Duty cycle       | 1%                          | Réglementation ETSI EU868                                       |
| Intervalle       | 30 secondes                 | Envoi périodique des données capteurs                           |
| DevEUI           | Défini dans `credentials.h` | Identifiant unique du device, fourni par TTN                    |
| AppEUI           | Défini dans `credentials.h` | Identifiant de l'application TTN                                |
| AppKey           | Défini dans `credentials.h` | Clé de chiffrement OTAA, à ne pas partager                      |

## Configuration capteurs

### Capteur HC-SR04

#### Présentation

Le HC-SR04 est un capteur de distance à ultrasons qui mesure l'espace entre lui et un obstacle en émettant une impulsion sonore et en calculant le temps de retour de l'écho. Il fonctionne sous 5V.

#### Branchements

1. Relier la broche VCC au **5v**
2. Relier la broche GND au **GND**
3. Relier la broche Trig à la PIN D13 de l'esp32
4. Relier la broche Echo à la PIN D14 de l'esp32

### Module relais

#### Présentation

> Ce module est utilisé pour allumer et éteindre les ventilateurs car nous ne disposons pas de ventilateur avec un fil pilote

Le SRD-05VDC-SL-C est un relais 5V qui permet de contrôler un circuit électrique à partir d'un signal logique basse tension. Il agit comme un interrupteur commandé.

#### Branchements

1. Relier la broche S à la PIN D23
2. Relier la broche + au **5v**
3. Relier la broche - au **GND**

> Pour la suite il faut se placer face au boîtier relais, côté bleu devant soi

4. Relier le connecteur du milieu et le **5v**
5. Relier le connecteur de droite et le **signal**

> Remarque : le connecteur de gauche reste vide

### Ventilateur

#### Présentation

Le ventilateur génère un flux d'air lorsqu'il est alimenté en courant continu via ses deux fils, positif (VCC) et négatif (GND).
Dans ce projet il permet de simuler un bloc de climatisation

#### Branchements

1. Relier la broche + (ou fil rouge) et le **signal**
2. Relier la broche - (ou fil noir) et le **GND**

### Buzzer

#### Présentation

Le buzzer est un composant acoustique qui émet un signal sonore lorsqu'il est alimenté
Dans ce projet il permet de faire une alarme en cas de dépassement de seuil

#### Branchements

1. Relier la broche - au **GND**
2. Relier la broche S à la PIN D22 de l'esp32

> Remarque : la broche + du buzzer est inutile le 5V par la broche signal

### Bouton poussoir

#### Présentation

Le bouton poussoir permet d'interagir manuellement avec le système.
Dans ce projet, il sert à allumer ou éteindre le buzzer en appuyant dessus.

#### Branchements

1. Relier une broche du bouton au **GND**
2. Relier l'autre broche à la PIN D5 de l'esp32

## Code gestion ventilateur depuis le TTN

1. Eteindre : '00'
1. Allumage sans alarme : '01'
1. Allumage avec alarme : '02'
