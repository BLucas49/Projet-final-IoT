# Architecture du Système IoT Edge & Monitoring (Stack MING)

Remarque : Cette architecture logicielle est déployée sur Raspberry Pi et communique avec l'ESP32 via le protocole MQTT sécurisé (TLS).

## **Objectifs**

L'objectif de ce système est de créer une passerelle Edge Computing complète pour :

* **Piloter** le système de ventilation via l'ESP32.  
* **Collecter** et stocker les données environnementales (Température, Humidité, Distance) via un deuxième ESP32.  
* **Monitorer** la santé du Raspberry Pi et des conteneurs (CPU, RAM, Réseau).  
* **Visualiser** l'ensemble des données sur un dashboard centralisé.

## **Configuration logicielle (Docker)**

Le système repose sur une architecture conteneurisée gérée par docker-compose. Tous les services communiquent via un réseau isolé nommé ming.

### **Services de la Stack MING**

1. **Mosquitto (Broker)** : Point central des messages MQTT (Port 8883 \- TLS).  
2. **InfluxDB** : Base de données temporelle pour l'historique des capteurs.  
3. **Node-RED** : Moteur de règles pour envoyé les messages à l'application de gestion
4. **Grafana** : Interface de visualisation pour les données IoT et le monitoring.

### **Backend JS pour l'application**

**Backend JS** : Interface de visualisation pour les données IoT et le monitoring.

Il faut ajouter l'image docker via scp : 
```bash
scp backend.tar user_de_vortre_pi@ip_de_votre_pi:/home/user_de_vortre_pi/ming
```
N'oubliez pas de renseigné les variable d'environnement nécessaire dans un .env au même niveau que le backend.tar dans pi (voir [.env.example](../.env.example))

### **Services de Monitoring**

1. **Prometheus** : Base de données collectant les métriques de performance.  
2. **cAdvisor** : Analyseur de ressources Docker (CPU, RAM par conteneur).

## ---

## Configuration de la Sécurité (MQTT TLS)

### **Présentation**

Pour protéger les échanges entre l'ESP32 et le Raspberry Pi, le port **8883** est utilisé avec un chiffrement SSL/TLS et une authentification par mot de passe.

### **Certificats et Authentification**

1. **ca.crt** : Certificat de l'autorité racine, à installer sur l'ESP32 et MQTT Explorer.  
2. **server.crt / server.key** : Utilisés par Mosquitto pour prouver son identité.  
3. **Fichier passwd** : Contient les identifiants générés via mosquitto\_passwd.

Remarque : Pour que le monitoring fonctionne sur Raspberry Pi, il est impératif d'activer les cgroups dans /boot/firmware/cmdline.txt avec cgroup\_enable=cpuset cgroup\_enable=memory cgroup\_memory=1

```bash
# Création des certificats
cd ~/ming
mkdir -p mosquitto/certs mosquitto/config
cd ~/ming/mosquitto/certs
# 1. Création de la clé privée de la CA (ca.key)
openssl genrsa -out ca.key 2048

# 2. Création du certificat public de la CA (ca.crt)
# Note : Le paramètre -subj évite que l'outil te pose plein de questions.
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt -subj "/C=FR/O=Ynov/OU=IoT/CN=MyLocalCA"

# 1. Création de la clé privée du serveur (server.key)
openssl genrsa -out server.key 2048

# 2. Création de la requête de signature (CSR)
# /!\ IMPORTANT : Remplace 10.44.20.13 par l'IP exacte de ton Pi si elle change
openssl req -new -key server.key -out server.csr -subj "/C=FR/O=Ynov/OU=IoT/CN=10.44.20.13"

# 3. Signature du certificat serveur par ta propre CA (génère server.crt)
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 3650
```

## ---

**Configuration capteurs (ESP32)**

### **Capteur HC-SR04 (Ultrasons)**

#### **Présentation**

Le HC-SR04 mesure la distance entre le boîtier et l'utilisateur pour une activation manuelle sans contact.

#### **Branchements**

1. Relier la broche VCC au **5v**  
2. Relier la broche GND au **GND**  
3. Relier la broche Trig à la PIN D13 de l'esp32  
4. Relier la broche Echo à la PIN D14 de l'esp32

### **Module relais (Ventilateur)**

#### **Présentation**

Le relais SRD-05VDC-SL-C permet à l'ESP32 de piloter la puissance du ventilateur (interrupteur piloté).

#### **Branchements**

1. Relier la broche S à la PIN D23  
2. Relier la broche \+ au **5v**  
3. Relier la broche \- au **GND**  
4. Relier le connecteur du milieu au **5v**  
5. Relier le connecteur de droite au **signal** (fil rouge du ventilateur)

## ---

**Flux de données et Dashboarding**

### **1\. Collecte (Node-RED)**

Node-RED est abonné au topic v1/devices/me/telemetry. À chaque message reçu :

* Il écrit la donnée dans **InfluxDB**.  
* Il envoie une requête POST vers le script **Google Sheets** (via l'URL de déploiement).

### **2\. Monitoring (Prometheus)**

Prometheus "scrappe" les données de cAdvisor toutes les 15 secondes.

* **Requête CPU type** : sum(container\_cpu\_usage\_seconds\_total) by (container\_label\_com\_docker\_compose\_service)  
* **Requête RAM type** : container\_memory\_working\_set\_bytes

### **3\. Visualisation (Grafana)**

Grafana utilise deux sources de données :

* **InfluxDB** : Pour les courbes de température et l'état du ventilateur.  
* **Prometheus** : Pour le dashboard de santé des conteneurs (ID 10619).

## ---

**Guide de dépannage rapide**

| Problème | Cause probable | Solution |
| :---- | :---- | :---- |
| Erreur JavaScript MQTT Explorer | Clé privée mise dans le champ certificat | Supprimer la "Client Key" dans la config TLS |
| Connexion Refused (Port 8883\) | Conteneur Mosquitto arrêté | Vérifier les permissions des certificats (chmod 644\) |
| Métriques Monitoring à 0 | Cgroups non activés sur le Pi | Ajouter les paramètres dans cmdline.txt et reboot |
| JSON Parse Error (Google Script) | Redirection Google non suivie | Activer setFollowRedirects dans le code ESP32 |
