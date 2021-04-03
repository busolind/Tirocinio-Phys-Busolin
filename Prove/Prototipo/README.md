# Prototipo

Lo sviluppo su ExternalJSONFilter stava andando ben oltre il nome della cartella quindi continuo qui.

Ora tramite MQTT (``Phys/setFromJSON``) è possibile inviare un JSON di configurazione del tipo:

    {
      apiUrl: "https://api.blockchain.com/v3/exchange/tickers/BTC-USD",
      filterJSON: {last_trade_price: true},
      path: "last_trade_price",
      min_value: 57000,
      max_value: 59000
    }
  
  Dove:
- ``apiUrl`` è appunto l'URL alla risorsa (stringa)
- ``filterJSON`` è un documento JSON che contiene ``true`` come placeholder del/i campi che si vuole considerare dalla risposta (stringa/oggetto)
- ``path`` è una stringa che contiene il "percorso" del campo che si vuole rappresentare fisicamente (campo: float, path: stringa)
- ``min_value`` indica il valore minimo della scala per rappresentare il valore (float)
- ``max_value`` indica il valore massimo (float)

È anche possibile settare i singoli campi via MQTT:
- ``Phys/setApiUrl``
- ``Phys/setFilterJson``
- ``Phys/setPath``
- ``Phys/setMinValue``
- ``Phys/setMaxValue``

Ho anche implementato un AsyncWebServer che per ora contiene solo un form che consente di inserire un JSON di configurazione (equivalente a quello di ``Phys/setFromJSON``)

ToDo:
- ✓ Cambiare il nome a ``interval_min`` e ``interval_max`` per non confondersi con l'intervallo di tempo
- Aggiungere configurazione per l'intervallo di tempo tra una richiesta e l'altra
- Salvare configurazione su file per mantenere modifiche dopo riavvio - [``LittleFS``](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html)

## Esempio di output su seriale (non aggiornato alle nuove variabili)
    *WM: AutoConnect Try No.:
    *WM: 0
    *WM: Connecting as wifi client...
    *WM: Using last saved values, should be faster
    *WM: Connection result: 
    *WM: 3
    *WM: IP Address:        
    *WM: 192.168.178.52     
    connected...yeey :)     

    WiFi connected
    IP address:
    192.168.178.52
    Attempting MQTT connection...connected
    [HTTPS] begin... URL: https://csrng.net/csrng/csrng.php?min=0&max=80000
    [HTTPS] GET... code: 200

    Filter: [{"random":true}]
    --- Inizio parte esclusa da JSON ---
    ---  Fine parte esclusa da JSON  ---

    [{"status":"success","min":0,"max":80000,"random":36440}]
    Documento filtrato:
    [{"random":36440}]
    Token: 0 [ARRAY]
    Token: random [OBJ]

    Valore estratto: 36440.00
    Minimo: 0.00
    Massimo: 80000.00

    Mappato a : 465
    Free heap: 44624
    Message arrived [Phys/setFromJSON]:
    {
      apiUrl: "http://www.randomnumberapi.com/api/v1.0/random?min=0&max=10000",
      filterJSON: [true],
      path: "0",
      interval_min: 0,
      interval_max: 10000
    }

    [HTTP] begin... URL: http://www.randomnumberapi.com/api/v1.0/random?min=0&max=10000
    [HTTP] GET... code: 200

    Filter: [true]
    --- Inizio parte esclusa da JSON ---
    ---  Fine parte esclusa da JSON  ---

    [1886]
    Documento filtrato:
    [1886]
    Token: 0 [ARRAY]

    Valore estratto: 1886.00
    Minimo: 0.00
    Massimo: 10000.00

    Mappato a : 192
    Free heap: 44304
    Messaggio ricevuto via POST: 
    {
      apiUrl: "https://api.blockchain.com/v3/exchange/tickers/BTC-USD",
      filterJSON: {last_trade_price: true},
      path: "last_trade_price",
      interval_min: 58000,
      interval_max: 60000
    }

    [HTTPS] begin... URL: https://api.blockchain.com/v3/exchange/tickers/BTC-USD
    [HTTPS] GET... code: 200

    Filter: {"last_trade_price":true}
    --- Inizio parte esclusa da JSON ---
    5e
    ---  Fine parte esclusa da JSON  ---

    {"symbol":"BTC-USD","price_24h":58741.8,"volume_24h":1242.38709665,"last_trade_price":59307.9}
    Documento filtrato:
    {"last_trade_price":59307.9}
    Token: last_trade_price [OBJ]

    Valore estratto: 59307.90
    Minimo: 58000.00
    Massimo: 60000.00

    Mappato a : 668
    Free heap: 44064
