Lo sviluppo su ExternalJSONFilter stava andando ben oltre il nome della cartella quindi continuo qui.

Ora tramite MQTT (``Phys/setFromJSON``) è possibile inviare un JSON di configurazione del tipo:

    {
      apiUrl: "https://api.blockchain.com/v3/exchange/tickers/BTC-USD",
      filterJSON: {last_trade_price: true},
      path: "last_trade_price",
      interval_min: 57000,
      interval_max: 59000
    }
  
  Dove:
- ``apiUrl`` è appunto l'URL alla risorsa (stringa)
- ``filterJSON`` è un documento JSON che contiene ``true`` come placeholder del/i campi che si vuole considerare dalla risposta (stringa/oggetto)
- ``path`` è una stringa che contiene il "percorso" del campo che si vuole rappresentare fisicamente (campo: float, path: stringa)
- ``interval_min`` indica il valore minimo della scala per rappresentare il valore (float)
- ``interval_max`` indica il valore massimo (float)

È anche possibile settare i singoli campi via MQTT:
- ``Phys/setApiUrl``
- ``Phys/setFilterJson``
- ``Phys/setPath``
- ``Phys/setIntervalMin``
- ``Phys/setIntervalMax``

Ho anche implementato un AsyncWebServer che per ora contiene solo un form che consente di inserire un JSON di configurazione (equivalente a quello di ``Phys/setFromJSON``)

ToDo:
- Cambiare il nome a ``interval_min`` e ``interval_max`` per non confondersi con l'intervallo di tempo
- Aggiungere configurazione per l'intervallo di tempo tra una richiesta e l'altra
- Salvare configurazione su file per mantenere modifiche dopo riavvio - [``LittleFS``](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html)
