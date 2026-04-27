# Interface Web : L'Oracle Météo

Ce dossier contient le code source de l'interface utilisateur web développée avec **Streamlit**. 
Elle permet de visualiser de manière interactive les prédictions générées par la carte STM32N6 et poussées sur le Cloud ThingSpeak.

## Lancer le site en local (sur votre machine)

1. Assurez-vous d'avoir Python installé sur votre machine.
2. Ouvrez un terminal dans ce dossier `dashboard/`.
3. Installez les dépendances requises :
   ```bash
   pip install -r requirements.txt
4. Lancez l'application Streamlit :
   Bash
   streamlit run oracle_dashboard.py