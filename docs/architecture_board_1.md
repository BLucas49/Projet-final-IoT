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

### Buzzer

#### Présentation

Le buzzer est un composant acoustique qui émet un signal sonore lorsqu'il est alimenté

#### Branchements

1. Relier la broche + au **5v**
2. Relier la broche - au **GND**
3. Relier la broche S à la PIN D4 de l'esp32

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
