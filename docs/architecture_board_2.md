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

#### Branchements

1. Relier la broche + (ou fil rouge) et le **signal**
2. Relier la broche - (ou fil noir) et le **GND**
