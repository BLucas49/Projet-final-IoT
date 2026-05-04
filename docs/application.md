# Présentation de l'application - NodeWatch

## Contexte et but de l'application

L'application se place dans le contexte de la surveillance d'un datacenter.
Elle a pour but de permettre aux techniciens de surveiller en temps réel les informations des différents capteurs présent dans le batîment.

## Fonctionnalitées

- Suveillance en temps réél de l'humidité, la température, et la luminosité
- Affichage de l'historique des données sous forme de graphique (1h, 24h, 7j)
- Configurer les seuils d'alertes sur les capteurs cité précédemment
- Afficher les alertes généré sur la page et envoyer des notifications push sur le téléphone de l'utilisateur.

## Technologies et outils

- Flutter (Dart) + Riverpod (state management)
- Node.js + Express
- InfluxDB (stockage des séries temporelles)
- MQTT via The Things Network (TTN)
- WebSocket (push temps réel)

## Backend

Le backend est un serveur **Node.js / Express** qui joue le rôle de passerelle entre les capteurs IoT et l'application mobile.

### Flux de données capteurs

1. Les capteurs envoient leurs mesures (température, humidité, luminosité) via **LoRaWAN** vers **The Things Network (TTN)**.
2. Le backend est abonné au broker MQTT de TTN et reçoit les trames uplink.
3. Les données sont stockées dans **InfluxDB** dans le measurement `capteurs_env`.

### Contrôle du ventilateur

- L'application envoie une commande HTTP `POST /actuators/fan` au backend.
- Le backend publie un **downlink TTN** (payload hex `01`/`00` sur le port 15) via MQTT pour allumer ou éteindre le ventilateur.
- Les changements d'état physiques du ventilateur (uplink TTN) sont reçus par MQTT et diffusés aux clients via le WebSocket `/ws/fan`.

### Alertes et seuils

- Les seuils (min/max) pour chaque capteur sont stockés dans InfluxDB (measurement `thresholds`) et accessibles via `GET /alerts/thresholds`.
- Le service `alertService` évalue les données reçues par rapport aux seuils et enregistre une alerte dans le measurement `alerts` lorsqu'un dépassement est détecté.
- Les 30 derniers jours d'alertes sont disponibles via `GET /alerts`.

### Routes REST exposées

| Méthode | Route | Description |
|---------|-------|-------------|
| `GET` | `/sensors/latest` | Dernière mesure des capteurs |
| `GET` | `/sensors/history?range=1h\|24h\|7d` | Historique agrégé |
| `GET` | `/alerts` | Liste des alertes (30 derniers jours) |
| `GET` | `/alerts/thresholds` | Seuils configurés |
| `POST` | `/alerts/thresholds` | Mise à jour des seuils |
| `GET` | `/actuators/fan` | État actuel du ventilateur |
| `POST` | `/actuators/fan` | Commande ON/OFF du ventilateur |
| `GET` | `/health` | Healthcheck |

### WebSockets

| Endpoint | Description |
|----------|-------------|
| `ws://.../ws/sensors` | Push des données capteurs toutes les 5 secondes |
| `ws://.../ws/fan` | Push de l'état du ventilateur en temps réel |

---

## Application

L'application mobile est développée en **Flutter** et utilise **Riverpod** pour la gestion d'état. Elle est organisée en trois écrans accessibles via une barre de navigation en bas de l'écran.

### Navigation

```
MainNavigation
├── Tableau de bord  (DashboardScreen)
├── Historique       (HistoryScreen)
└── Alertes          (AlertsScreen)
```

### Tableau de bord

L'écran principal affiche en temps réel les données des capteurs. Au démarrage, l'app récupère la dernière mesure via HTTP (`GET /sensors/latest`), puis se connecte au WebSocket `/ws/sensors` pour recevoir les mises à jour. En cas d'indisponibilité du WebSocket, l'app bascule automatiquement sur du **polling HTTP** toutes les 5 secondes.

- **Bannière de statut** : indique si l'ensemble des mesures est dans les seuils acceptables (vert) ou si une sonde est en dépassement (rouge/orange).
- **Cartes de capteurs** : affichent la valeur courante de la température (°C), l'humidité (%) et la luminosité (lux) avec indication visuelle du dépassement de seuil.
- **Carte ventilateur** : permet d'activer ou désactiver manuellement le ventilateur. Le contrôle est aussi **automatique** : si la température dépasse le seuil max configuré, le ventilateur s'allume ; il s'éteint dès que la température repasse sous le seuil.
- **Badge EN DIRECT / HORS LIGNE** : indique l'état de la connexion WebSocket en temps réel.

### Historique

Affiche l'évolution des trois capteurs sous forme de **graphiques**. L'utilisateur peut choisir la plage de temps :

| Plage | Agrégation |
|-------|------------|
| 1 heure | par minute |
| 24 heures | par 15 minutes |
| 7 jours | par heure |

Les données sont récupérées via `GET /sensors/history?range=<plage>`.

### Alertes

Liste les alertes générées lorsqu'un capteur dépasse ses seuils configurés. L'écran charge d'abord les 5 alertes les plus récentes, puis propose un bouton "Charger plus" qui ajoute 10 entrées supplémentaires à chaque clic.

Chaque alerte affiche : le capteur concerné, le type de dépassement (min/max), la valeur mesurée, le seuil franchi et l'horodatage.

L'écran donne également accès aux **curseurs de configuration des seuils** pour chaque capteur. Les modifications sont sauvegardées via `POST /alerts/thresholds` et prennent effet immédiatement sur le tableau de bord et la génération future d'alertes.

### Notifications push

Un provider de notifications (`alertNotificationProvider`) est actif pour toute la durée de vie de l'application. Il surveille le flux de données capteurs et déclenche une **notification locale** (via `flutter_local_notifications`) dès qu'une valeur franchit un seuil configuré, même lorsque l'application est en arrière-plan.


## Liens externes

Git du backend: https://github.com/NockIA/iot-dashboard-backend.git

Git de l'application mobile: https://github.com/NockIA/iot-dashboard-app.git
