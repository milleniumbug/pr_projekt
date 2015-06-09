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

w*h bajtów (w to jest szerokość planszy, h to jest wysokość planszy, określone na początku, przesłane zawczasu przez serwer w komunikacie 02) określające kolejne pola na planszy (identyfikatory określające rodzaj pola są opisane poniżej)

4 stany dla każdego gracza.

Stan gracza określa:

2-bajtowa liczba big-endian bez znaku określającą współrzędną x pozycji na planszy

2-bajtowa liczba big-endian bez znaku określającą współrzędną y pozycji na planszy

(konieczne ze względu na możliwość kładzenia bomb pod sobą – dlatego pozycja gracza jest tu, a nie wśród danych planszy)

1-bajtowa liczba bez znaku (wartości od 0 do 100) określająca "progres w wykonaniu ruchu" 

1-bajtowa liczba ze znakiem określająca kierunek w którym idzie (wartości jak na dole w komunikacie 09, ale bez kładzenia bomby)

1-bajtowa liczba bez znaku określająca czas do wybuchnięcia bomby, przeskalowany o 8 (tj. liczba 1 otrzymana oznacza że bomba należąca do tego gracza która wybuchnie jako pierwsza w kolejności od tej chwili wybuchnie za 8 milisekund, liczba 2 - że za 16 milisekund). Jeżeli nie ma żadnych bomb, wysyłane jest 0.

1 bajt: nieużywany

06:

1-bajtowa liczba bez znaku określająca ilość graczy obecnie połączonych

1-bajtowa liczba bez znaku określająca oczekiwaną ilość graczy

1-bajtowa liczba bez znaku określająca stan gry: OCZEKIWANIE NA GRACZY (1), GRA W TRAKCIE (2)

2-bajtowa liczba big-endian bez znaku określająca szerokość planszy w

2-bajtowa liczba big-endian bez znaku określająca wysokość planszy h

1-bajtowa liczba bez znaku określająca numer gracza który jest sterowany przez dane połączenie, 0 jeżeli gracz nie jest podłączony do serwera. 

09:

1-bajtowa liczba ze znakiem oznaczającą wciśnięte klawisze przez gracza: jest to suma wartości:

-1 : góra, 1 : dół, -4 : lewo, 4 : prawo, 16 : położenie bomby

71:

"przyjazny" komunikat o błędzie, zakończony bajtem zerowym.

Identyfikatory pól na planszy:

Pusta przestrzeń - 0

Zniszczalny mur - 1

Niezniszczalny mur - 2

Bomba - 3 (wydaję mi się że możemy to rozszerzyć i dać np. "4 dla gracza 1, 5 dla gracza 2, 6 dla gracza 3 i 7 dla gracza 4", ale to tego nie robię na ten moment - to do uzgodnienia)