# putz_project_2
Zweites NVS Projekt, 5AHIF

## Beschreibung

Das Progamm ConnectSim dient dazu, eine Übertragung von Daten mittels einem Sliding Window Algorithmus darzustellen.
Weiters bietet das Programm noch die Funktionen, drei Fehlerarten im Networking darzustellen. Diese wären Paketverlust,
Paketverfäschung (Datenmanulupation) und eine falsche Paketreihenfolge.

Vom Sender aus werden zufällige ASCII-Zeichen, deren
Menge optional angeben werden kann, in derer dezimalen Darstellung, mittels einer Verbindung zu einem Server übertragen. Diese
Verbindung ist vordefiniert. Der Server wird vor der Ausführung des Programms manuell vom User gestartet, dieser wartet dann auf einkommende
Verbindungsanfragen und startet den Alogrithmus bei erfolgreicher Verbindung.

Die größe des Sliding Window Fensters veträgt Standardgemäß 1, ist aber per Kommandozeile vom User aus konfigurierbar.
Die Daten weden Client zum Server übertragen. Zuerst wird die Anzahl, aller zu übertrageneder Datenframes, dann die größe des Sliding Window Fensters
dem Server mitgeteilt.

Danach beginnt die eigentliche Übertragung der Daten. Der Server nimmt ein Datenframe aus, addiert den Wert zu einer Checksumme 
welche pro Fenster berechnet wird und sendet die Daten, als AKN zurück. Wurden so viele Daten aufgenommen, wie die Sliding Window größe vorher übermittelt worden
ist, sendet der Server einen AKN an den Client mit der gesamten Checksumme des Fensters und alles beginnt wieder von vorne, bis alle Daten auf diese Art übertragen worden sind.
Am Ende überträgt der Server nochmals die maximale Summer der Übertragenen Daten pro Fenster zum Client. Diese Summe hat sich der Client selbst vor dem Abschicken der
Daten gebildet. Diese wird nun verglichen ob sie dieselbe ist. 

Das Programm gibt diesen Vorgang in einer Logdatei für den Server, und einer Logdatei für den Client aus. Der Name und Pfad der Logdatei für den Client ist frei konfigurierbar.
Es gibt noch einige weitere Funktionen, wie auch die Simulation von Netzwerkfehlern.

Eine genaue Anleitung und detailliertere Beschreibung findet man im _documents_-Ordner unter
connectsim_ausarbeitung_putz.pdf".
