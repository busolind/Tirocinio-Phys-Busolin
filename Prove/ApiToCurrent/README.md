Piccolo progetto in cui provo a trasformare in qualcosa di tangibile il risultato di una richiesta API (randomnumberapi.com) richiedendo un numero casuale tra 0 e 100 ogni volta, per simulare una percentuale.
Questa viene mappata all'intervallo da 0 a 1023 di analogWrite e "visualizzata" mediante un LED.
Un funzionamento simile sarà richiesto per l'uso con gli strumenti vintage.

(atrent) in realtà per modulare i LED si usa la PWM ;) - per un voltmetro potrebbe bastare la PWM, dobbiamo fare prove, altrimenti ok l'analogWrite, tutto da sperimentare

La randomicità del risultato permette di notare subito che le richieste vanno a buon fine semplicemente guardando il comportamento del LED
