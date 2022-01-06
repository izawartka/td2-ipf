# Interfejs Pulpitów Fizycznych do gry/symulatora TD2
## UWAGA!
Kod nie nadaje się do publikacji i jestem tego w pełni świadom. To, że w ogóle widzisz ten branch tak "wcześnie" jest
zasługą wielu próśb społeczności TD'ka abym wreszcie go wypuścił. W wolnym czasie postram się doprowadzić go do stanu 
używalności, póki co jest coś takiego. Miłej zabawy!

## TODO
- Synchronizowanie stanu wszystkich urządzeń przy uruchomieniu pulpitu
- Możliwość zsynchronizowania stanu wszystkich urządzeń "na zawołanie"
- Uładnienie kodu (tak, jest kradziony XD)
- Wiele więcej...

## Jak używać?
Sketch z folderu Arduino dostosowujesz do swoich potrzeb i wgrywasz na Arduino, ustawiasz COM, port i IP w pliku INI.
Uruchamiasz w kolejności TD2, Program Arduino, Program c++

Kompilacja programu wymaga użycia Visual C++ (najniżej sprawdzany to 2010), ze względu na używanie biblioteki winsock.

## Autorzy
ja
+ ighor dodał obsługe pliku konfiguracyjnego
