# Architecture du projet final IOT

> Remarque générale pour tout Branchements ne pas mettre sous tension le montage
> Pour des soucis de compréhension chacunes des deux boards ont leur propre documentation.

![alt schema](./images/mermaid_schema.png)

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

#### Parametres LoRaWAN

| Parametre        | Valeur          | Justification                                                |
| ---------------- | --------------- | ------------------------------------------------------------ |
| Mode             | OTAA            | Plus sécurisé qu'ABP (re-négociation des clés à chaque join) |
| Spreading Factor | SF7             | TODO : adapter selon la distance et la consommation          |
| Bande            | EU868           | Europe (8 canaux, 125 kHz BW)                                |
| Duty cycle       | 1%              | Réglementation ETSI EU868                                    |
| DevEUI           | [dans config.h] | Identifiant unique du device                                 |

> Le module se configure via le code lié à la board

#### Architecture board 1

#### Présentation

Board de récupération des données avec les capteurs pour la transmission vers TTN

#### Branchements

Voir architecture_board_1.md ci-contre.

#### Architecture board 2

#### Présentation

Board de gestion des ventilateurs (manuel via capteur/ authomatisée via lorawan)

#### Branchements

Voir architecture_board_2.md ci-contre.
