Gra Bomberman, w trybie multiplayer dla 4 graczy.
----

Gracze chodzą po planszy widocznej od góry, dwuwymiarowej podzielonej na pola. Celem gry jest pozostać ostatni na planszy nie będąc zranionym bombą. W tym celu gracz może upuszczać bomby, w celu ranienia innych graczy lub niszczenia murów. Dla urozmaicenia rozrywki przy niszczonych murach mogą pojawić się bonusy, które mogą wpłynąć na moc bomb upuszczonych przez gracza, dać graczowi nieśmiertelność przez jakiś czas lub inne.

Elementy na planszy:

 - gracz
 - niezniszczalny mur
 - zniszczalny mur
 - bomba
 - eksplozja
 - bonus

Architektura klient-serwer
-----

Komunikacja będzie odbywać się przez protokół UDP. Protokół TCP został odrzucony ze względu na gwarancje otrzymania danych pakietów w tej samej kolejności, w której klient wysyłał pakiety - ta gwarancja jest zbędna gdyż pakiety są niezależne od siebie, a może opóźnić faktyczne wysłanie pakietu przez bardzo długi czas, przez co nie nadaje się do zastosowań real-time.

Przyjęte jest założenie, że serwer jest autorytetem - jeżeli klient i serwer mają sprzeczne opinie na temat obecnego stanu gry, to serwer ma ostateczne zdanie na ten temat. Podział ról zakłada że serwer przechowuje stan obecnej gry oraz cała logika gry jest wykonywana na serwerze. Klient z kolei przesyła jedynie akcje wykonane przez gracza: "idź w lewo", "idź w prawo", "postaw bombę". W odpowiedzi serwer przesyła obecny stan gry. Dzięki temu oszukiwanie jest utrudnione - co prawda klient może dowolnie interpretować dane które wysłał od serwera, jednak nie będzie to miało wpływu na to co się dzieje na serwerze.

Protokół
----

Pierwsze cztery bajty zawierają liczbę w formacie big-endian określającą wersję programu. Jeżeli wersja gry się nie zgadza, serwer jest zobowiązany odrzucić połączenie. Kolejny bajt określa rodzaj zapytania lub odpowiedzi (numery oznaczają liczbę o podstawie heksadecymalnej):

 - 01 - (klient) żądanie przyłączenia się do gry
 - 02 - (serwer) zaakceptowanie i przyjęcie do gry (+)
 - 03 - (klient) żądanie przesłania obecnego stanu gry
 - 04 - (serwer) przesłanie obecnego stanu gry (+)
 - 05 - (klient) żądanie przesłania informacji o serwerze
 - 06 - (serwer) informacje o serwerze: ilość graczy, oczekiwana ilość graczy, stan gry (+)
 - 07 - (klient) sygnał utrzymania połącznenia (keep-alive)
 - 08 - (klient) "grzeczne" zakończenie połączenia przez klienta
 - 09 - (klient) akcja wykonana przez gracza (+)
...nie przydzielone...
 - 71 - (serwer) generyczny komunikat o nieznanym rodzaju zapytania (+)
 - 72 - (serwer) odrzucenie połączenia: niekompatybilna wersja
 - 73 - (serwer) odrzucenie połączenia: gra już rozpoczęta
 - 74 - (serwer) odrzucenie połączenia: serwer pełny

W większości z tych komunikatów niepotrzebne są żadne dodatkowe informacje, z wyjątkiem tych oznaczonych przez (+) dla tych przypadków dodatkowe dane są opisane poniżej:

02:

Te same informacje co w komunikacie 06.

04: 

4-bajtowa liczba big-endian bez znaku określająca pozostały czas do końca gry w milisekundach.

w*h bajtów (w to jest szerokość planszy, h to jest wysokość planszy, określone na początku, przesłane zawczasu przez serwer w komunikacie 02) określające kolejne pola na planszy

4 stany dla każdego gracza.

Stan gracza określa:

2-bajtowa liczba big-endian bez znaku określającą współrzędną x pozycji na planszy

2-bajtowa liczba big-endian bez znaku określającą współrzędną y pozycji na planszy

(konieczne ze względu na możliwość kładzenia bomb pod sobą – dlatego pozycja gracza jest tu, a nie wśród danych planszy)

4-bajtowe pole bitowe określające rodzaje bonusów otrzymane przez gracza. 

06:

1-bajtowa liczba bez znaku określająca ilość graczy obecnie połączonych

1-bajtowa liczba bez znaku określająca oczekiwaną ilość graczy

1 bajt określający stan gry: OCZEKIWANIE NA GRACZY, GRA W TRAKCIE

2-bajtowa liczba big-endian bez znaku określająca szerokość planszy w

2-bajtowa liczba big-endian bez znaku określająca wysokość planszy h

09:

1-bajtowa liczba ze znakiem oznaczającą wciśnięte klawisze przez gracza: jest to suma wartości:

-1 : góra, 1 : dół, -4 : lewo, 4 : prawo, 16 : położenie bomby

71:

"przyjazny" komunikat o błędzie, zakończony bajtem zerowym.
