Når en Bluetooth enhet kobler til får den en ID og addressen lagres i address_array.
Hvis den scanner for lengre enn 10 sekunder eller den kobler seg til maks antall enheter 
stopper scanninga og dk'et kobler seg til nRF Cloud. Når enheten mottar data over Bluetooth
blir ID'en gjort om til en addresse og meldingen vidresendes til Cloud.
For å skjønne hvilken enhet den fikk meldingen fra må den starte med:
*{id}
For å sende en melding fra terminalen i cloud til en spesifikk BLE enhet:
{"message":"*{id til enhet} {melding}"}
For å manuelt starte scanning:
{"message":"StartScan"} 