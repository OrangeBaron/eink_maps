# 🗺️ e-ink Maps

## 📌 Cos'è

**e-ink Maps** è un progetto basato su microcontrollore (ottimizzato per ESP32-S3) che trasforma un piccolo schermo e-paper da 1.54" in un display secondario per la navigazione turn-by-turn.

Progettato per essere compatto, leggibile sotto la luce diretta del sole e a bassissimo consumo energetico, il dispositivo funge da "ripetitore" delle indicazioni stradali, ricevendo i dati in tempo reale da uno smartphone tramite connessione Bluetooth Low Energy (BLE). Il progetto è stato pensato per interfacciarsi con app di automazione Android, come **Tasker**.

## ⚙️ Cosa fa

Il sistema agisce come un Server BLE in attesa di connessione. Una volta collegato allo smartphone, esegue le seguenti funzioni:

* **Ricezione dati in tempo reale:** Riceve pacchetti di dati via Bluetooth contenenti le istruzioni di navigazione (strutturati nel formato `distanza|indicazione testuale|codice icona|info viaggio`).
* **Interfaccia di guida:** Interpreta il payload ricevuto e lo disegna sul display e-ink, mostrando all'utente:
* L'icona grafica della manovra da eseguire (svolte, rotonde, uscite, arrivo a destinazione, ecc.).
* La distanza esatta dalla prossima manovra.
* L'indicazione testuale (es. il nome della via o la direzione).
* Informazioni generali sul viaggio in corso (es. tempo di arrivo stimato).

* **Gestione intelligente del display:** Per garantire reattività durante la guida ed evitare il fastidioso effetto "ghosting" tipico degli schermi e-ink, il software alterna **aggiornamenti parziali** (velocissimi, per i dati che cambiano spesso come la distanza) ad **aggiornamenti totali** periodici dello schermo.
* **Gestione del Risparmio Energetico:** Se il dispositivo perde la connessione Bluetooth per più di un minuto, entra automaticamente in modalità *Hibernate / Deep Sleep*, spegnendo l'alimentazione dello schermo e mostrando un'icona di splash per massimizzare la durata della batteria.
* **Multitasking nativo:** Sfrutta le capacità FreeRTOS dell'ESP32 separando la logica di ricezione Bluetooth dall'aggiornamento grafico del display su core e task differenti, evitando blocchi e garantendo una ricezione fluida dei dati.
