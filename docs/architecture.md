# Architecture du projet final IOT

> Remarque générale pour tout Branchements ne pas mettre sous tension le montage
> Pour des soucis de compréhension chacunes des deux boards ont leur propre documentation.

Certains branchements se répète ils seront donc ici

## Configuration générale

### Module LoRa (RAK3172)

#### Présentation

Le RAK3172 est un module de communication LoRaWAN ultra-basse consommation basé sur le STM32WLE5CC, conçu pour transmettre de petites quantités de données sur de longues distances (jusqu'à plusieurs kilomètres) via le protocole LoRa. Il permetra à nos esp32 de communiquer avec l'interface TTN

#### Branchements

1. Broche GND relié au **GND**
2. Broche 3v relié à la PIN 3v3 du module esp32
3. Broche UART2_Tx relié à la PIN D16 du module esp32
4. Broche UART2_Rx relié à la PIN D17 du module esp32

> Les broches cités ici sont les broches du module RAK3172

###

Architecture board 1 => Board de récupération des données avec les capteurs pour la transmission vers TTN
Architecture board 2 => Board de gestion des ventilateurs (manuel via capteur/ authomatisée via lorawan)
