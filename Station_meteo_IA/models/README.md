# 📊 Fiabilité et Performance du Modèle

Ce document présente l'analyse de fiabilité de l'IA **Oracle Météo** générée lors de la phase d'entraînement. Ces graphiques valident la précision du modèle avant son déploiement en `int8` sur la carte STM32.

![Analyse de Fiabilité](Graphique_Model.jpg)

### 🔍 Interprétation des résultats :

1. **Courbes de Perte (Loss)** : La convergence des courbes d'entraînement et de validation confirme l'absence de **surapprentissage** (*overfitting*).
2. **Suivi Chronologique** : Le modèle (en rouge) anticipe avec succès les cycles de température réels (en bleu) sur un échantillon de test.
3. **Corrélation** : La concentration des points sur la diagonale démontre une forte précision de la régression pour les températures.
4. **Matrice de Confusion** : Ce quadrant valide la capacité de l'IA à classifier correctement les épisodes de pluie.

---
🔗 **Dashboard Live :** [oracle-meteo-usmb.streamlit.app](https://oracle-meteo-usmb.streamlit.app/#probabilite-de-precipitations)
