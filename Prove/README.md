Nel file [atm_viaCeloriaIstitutoBesta.json](atm_viaCeloriaIstitutoBesta.json) si trova il risultato della prova di una chiamata all'API di ATM giromilano che ho trovato
nella repo della pensilina ATM. Non sono sicuro che questa cosa rispetti i TOS, come menzionato anche nell'altra repo.\
La prova è stata fatta sulla fermata "Via Celoria (Istituto Besta)" della linea 93 in direzione Viale Omero, numero 12493 - [Street View](https://www.google.com/maps/@45.4771774,9.2299794,3a,24.9y,275.32h,85.92t/data=!3m6!1e1!3m4!1srrvyfBm-6PV-0ohOH_GFiA!2e0!7i16384!8i8192)

URL API: https://giromilano.atm.it/proxy.ashx \
Body della richiesta POST: {url: tpPortal/geodata/pois/stops/12493}

Aprendo il sito di giromilano e ad esempio cliccando sulle info di una fermata della metro 1 si nota dalla scheda network della console una chiamata a https://giromilano.atm.it/proxy.ashx con body {url: tpPortal/tpl/journeyPatterns/-1|0} (direzione opposta: tpPortal/tpl/journeyPatterns/-1|1) \
La risposta è contenuta nel file [atm_M1.json](atm_M1.json)

Da lì è quindi possibile risalire agli id di ogni fermata fra cui Sesto 1° Maggio FS con codice -88 (pare non variare con direzione, probabilmente essendo geodata non è distinto, mentre per gli autobus sì in quanto le fermate sono fisicamente separate, magari ci sono altre API a cui fare riferimento per la metro con direzioni opposte):

URL API: https://giromilano.atm.it/proxy.ashx \
Body della richiesta POST: {url: tpPortal/geodata/pois/stops/-88}

Risultato: [atm_M1_Sesto_1_Maggio.json](atm_M1_Sesto_1_Maggio.json) [stesso concetto dell'autobus]

Questo è sfociato in un [progetto personale](https://github.com/busolind/Pensilina-ATM-IoT/blob/main/README.md). Di non enorme utilità ai fini del progetto generale ma è servito ad approfondire le richieste alle API, specialmente con HTTPS che su ESP8266 non è triviale

Conclusioni:
- Bisogna stare molto attenti alle scarse risorse dell'ESP, infatti con le risposte da vari kilobyte delle API ATM (esempio estremo) l'unico modo per ottenere qualcosa di funzionante è stato usare gli stream e impostare un filtro su ArduinoJson. Questo mi porta al prossimo punto:
- Nell'ottica di fornire "da remoto" all'ESP link ad API e campi interessanti, bisogna capire come impostare un filtro basato su informazioni esterne e non "hardcodato", così come generalizzare l'esecuzione delle richieste
- Offrire un'interfaccia web "on board" potrebbe non essere possibile sempre per il motivo delle scarse risorse

---

In [ApiToCurrent](ApiToCurrent) ho provato a far dipendere un output fisico dal risultato di una richiesta.

La prova successiva riguarderà l'utilizzo di un JSON per creare il filtro, senza hardcodarlo, e trovare un modo comunque per selezionare "da fuori" il campo del JSON a cui si è interessati

In [ExternalJSONFilter](ExternalJSONFilter) Cerco di generalizzare il concetto di prendere una risorsa e rappresentarla. Più appiunti nel README dedicato
