Estensione di APIToCurrent in cui cerco di renderlo più generico. L'obiettivo primario sarebbe quello di ricevere dall'esterno URL della risorsa da interrogare e campo a cui si è interessati, senza doverlo hardcodare.
Al momento sono anche al lavoro sulla questione HTTPS visto che alcune API vogliono quello, ma nell'implementazione attuale la compatibilità HTTPS esclude HTTP. Bisogna quindi capire quale protocollo va utilizzato dall'URI e istanziare il client WiFi corretto (WiFiClient per HTTPS e BearSSL::WiFiClientSecure per HTTPS)

Esempio di output:

    [HTTP] begin...
    [HTTP] GET... code: 200
    5d
    {"symbol":"BTC-USD","price_24h":55429.4,"volume_24h":323.09501294,"last_trade_price":57905.7}
    Documento filtrato:
    {"last_trade_price":57905.7}
    Mappato a : 740
    Free heap: 48968
