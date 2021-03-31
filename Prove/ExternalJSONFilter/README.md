Estensione di APIToCurrent in cui cerco di renderlo più generico. L'obiettivo primario sarebbe quello di ricevere dall'esterno URL della risorsa da interrogare e campo a cui si è interessati, senza doverlo hardcodare.
Al momento sono anche al lavoro sulla questione HTTPS visto che alcune API vogliono quello, ma nell'implementazione attuale la compatibilità HTTPS esclude HTTP. Bisogna quindi capire quale protocollo va utilizzato dall'URI e istanziare il client WiFi corretto (WiFiClient per HTTP e BearSSL::WiFiClientSecure per HTTPS).

UPDATE:
Ora lo sketch è in grado, a partire da un URL API, un JSON di filtro (formato come la risposta ma contenente solo i campi necessari con assegnato true) e una ``path`` del tipo ``"places/0/latitude"`` di estrarsi un campo e rappresentarlo elettricamente. Funziona sia con URL ``http://`` che ``https://``

Esempio di output:

    [HTTPS] begin...
    [HTTPS] GET... code: 200

    Filter:
    {"last_trade_price":true}
    --- Inizio parte esclusa da JSON ---
    5e
    ---  Fine parte esclusa da JSON  ---

    {"symbol":"BTC-USD","price_24h":57758.9,"volume_24h":1156.74365945,"last_trade_price":58282.9}
    Documento filtrato:
    {"last_trade_price":58282.9}
    Token: last_trade_price [OBJ]

    Valore estratto: 58282.90

    Mappato a : 745 
    Free heap: 50208
