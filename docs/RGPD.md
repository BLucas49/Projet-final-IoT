# Fiche RGPD

**Date :** _[01/05/2026]_
**Version :** _2_

---

## 1. Donnees collectees (Art. 30 RGPD -- Registre des traitements)

| Donnée collectée                        | Type              | Capteur          | Fréquence        | Durée de rétention                   | Qualifié de DCP ?                                |
| --------------------------------------- | ----------------- | ---------------- | ---------------- | ------------------------------------ | ------------------------------------------------ |
| 23.1                                    | Numérique (float) | DHT11            | 15 min           | 30 jours                             | Non -- donnée physique                           |
| 39                                      | Numérique (float) | DHT11            | 15 min           | 30 jours                             | Non -- donnée physique                           |
| DevEUI du device                        | Identifiant       | RAK3172          | À chaque join    | 30 jours                             | **Potentiellement** -- identifiant unique        |
| Métadonnées TTN (RSSI, SNR, gateway_id) | Métadonnée réseau | TTN              | À chaque envoi   | Supprimé à la consomation du message | **Potentiellement** -- géolocalisation indirecte |

> **Donnée à caractère personnel (DCP) :**

Sur ce projet aucune données personnelles ne sont utilisée. Toutefois il est nécessaire de stipuler que les Métadonnées et géolocalisation des capteurs pourrais le devenir lors d'une mise en production.

### Analyse spécifique au cas d'usage

> TODO : décrire ici si les données collectées dans votre cas d'usage peuvent
> qualifier de DCP. Justifier en une phrase.
>
> Exemple (présence) : "Un capteur PIR détectant des mouvements dans un domicile
> privé permet d'inférer des habitudes de vie. Ces données qualifient de DCP
> au sens de l'art. 4(1) RGPD."
>
> Exemple (température extérieure) : "La température extérieure mesurée sur une
> terrasse ne permet pas d'identifier une personne. Ces données ne qualifient
> pas de DCP. Cependant, les métadonnées TTN (gateway_id, RSSI) permettant
> une géolocalisation indirecte sont traitées avec prudence."

---

## 2. Mesures techniques appliquees (Art. 32 RGPD)

| Mesure                           | Composant   | Implémentation                                    |
| -------------------------------- | ----------- | ------------------------------------------------- |
| Authentification broker MQTT     | Mosquitto   | password_file, allow_anonymous false              |
| Chiffrement en transit MQTT      | Mosquitto   | TLS port 8883, certificat auto-signé              |
| Chiffrement radio LoRaWAN        | RAK3172/TTN | AES-128 natif, clés OTAA                          |
| Contrôle d'accès base de données | InfluxDB    | Token API, 1 token par service                    |
| Contrôle d'accès tableau de bord | Grafana     | Mot de passe admin, sign-up désactivé             |
| Minimisation des données         | Payload hex | Payload 2 octets (valeur seule, sans métadonnées) |
| Limitation de durée de rétention | InfluxDB    | Rétention 30 jours (configurable)                 |

1. Sécurité MQTT

Port sécurisé utilisé : 8883
Utilisation de login/mot de passe
TLS

2. Sécurité TTN

APP Key + Identifiant de machine

3. Sécurité InfluxDB

Utilisation de login/mot de passe

Génération d'un token par service

![alt image](../images/influxdb_token.png)

4. Grafana

Utilisation de login/mot de passe

5. RGPD

Rétention des données : 30 jours

### Mesures non appliquees et justification

| Mesure non appliquée                     | Raison                                             | Impact résiduel                                      |
| ---------------------------------------- | -------------------------------------------------- | ---------------------------------------------------- |
| Certificat TLS signé par une CA reconnue | Contexte de formation, CA auto-signée              | Risque d'attaque MITM sur le réseau local uniquement |
| Chiffrement au repos (InfluxDB)          | Hors scope formation, ressources RPi limitées      | Accès physique au RPi = accès données                |
| Pseudonymisation device_id               | Identification du device nécessaire pour le projet | Lien device <-> étudiant connu du seul intervenant   |

---

## 3. Droits des personnes (Art. 15-22 RGPD)

Les données collectées ne qualifiant pas de DCP, les droits des personnes au titre du RGPD ne s'appliquent pas directement. Cependant, par souci de transparence, les données sont supprimées automatiquement après 30 jours.

---
