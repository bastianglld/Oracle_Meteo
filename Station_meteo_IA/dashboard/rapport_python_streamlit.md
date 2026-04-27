# 🌐 Architecture Logicielle et Utilisation de Python (Oracle Météo)

Ce document détaille l'utilisation du langage **Python** et des bibliothèques associées pour la gestion des données (IoT) et la visualisation (Dashboard) du projet Oracle Météo.

---

## 1. Python : Le "Système Nerveux" du Projet

Dans cette architecture, Python ne sert pas à l'exécution de l'IA (qui est faite en C sur la STM32), mais joue le rôle de liant entre le matériel, le Cloud et l'utilisateur final. Son utilisation se divise en deux parties distinctes :
1. **La Passerelle IoT locale** : Pour l'envoi des données du capteur vers Internet.
2. **Le Dashboard Cloud** : Pour l'affichage interactif des résultats via **Streamlit**.

---

## 2. Bibliothèques Python Utilisées

Le projet s'appuie sur quatre bibliothèques piliers, chacune ayant un rôle spécifique :

| Bibliothèque | Rôle Principal | Utilisation dans le Projet |
| :--- | :--- | :--- |
| **`streamlit`** | Framework Web | Création de l'interface utilisateur et de la page web interactive. |
| **`pandas`** | Analyse de données | Manipulation des données historiques sous forme de tableaux (DataFrames). |
| **`requests`** | Communication HTTP | Récupération des données depuis l'API ThingSpeak et envoi via la gateway. |
| **`pyserial`** | Communication Série | Lecture des messages envoyés par la carte STM32 via le port USB. |

---

## 3. Focus sur Streamlit : L'interface Utilisateur

**Streamlit** est le framework choisi pour le déploiement du Dashboard. Contrairement au développement web classique (HTML/CSS/JS), Streamlit permet de transformer un script Python en application web interactive en quelques lignes de code.

### Pourquoi Streamlit ?
* **Rapidité** : Permet de créer des jauges, des graphiques et des colonnes de mise en page nativement en Python.
* **Intégration Data Science** : Parfaitement compatible avec Pandas pour filtrer les données en temps réel.
* **Déploiement Cloud** : L'application est hébergée gratuitement sur **Streamlit Community Cloud**, ce qui la rend accessible via une URL publique pour la soutenance.

### Fonctionnement du Dashboard (`oracle_dashboard.py`)
1. Le script utilise `requests` pour interroger l'API de **ThingSpeak**.
2. Il reçoit un flux JSON contenant la température, l'humidité et les prédictions de l'IA.
3. Les données sont nettoyées avec `pandas`.
4. Streamlit affiche dynamiquement :
    * Les **métriques** (valeurs actuelles).
    * Les **prévisions à 5 jours** (température, humidité, pression).
    * La **probabilité de précipitations** (classification IA).

---

## 4. Lien vers l'application en direct

Le Dashboard est accessible publiquement à l'adresse suivante pour consulter les prédictions en temps réel :

🔗 **[Accéder au Dashboard Oracle Météo](https://oracle-meteo-usmb.streamlit.app/#probabilite-de-precipitations)**

---

## 5. Script de Passerelle (`pont_thingspeak.py`)

C'est le script qui tourne en local sur l'ordinateur de test. Il assure le lien physique :
* **Input** : Lecture du flux `printf` de la STM32 via `pyserial`.
* **Traitement** : Extraction des valeurs numériques.
* **Output** : Envoi d'une requête POST vers ThingSpeak pour mettre à jour la base de données Cloud.

---
*Ce rapport souligne l'efficacité de l'écosystème Python pour le prototypage rapide d'applications IoT complexes, permettant de passer d'un signal brut sur microcontrôleur à une application web mondiale en un temps record.*
