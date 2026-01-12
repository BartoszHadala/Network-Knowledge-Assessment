---
title: "Założenia Projektowe - System Egzaminacyjny Klient-Serwer"
subtitle: "Programowanie Sieciowe"
author: "Bartosz Hadała"
date: "9 grudnia 2025"
lang: pl
geometry: margin=2.5cm
fontsize: 11pt
---

# Założenia Projektowe

## 1. Architektura Aplikacji

Projekt zakłada implementację systemu rozprosznego opartego na architekturze **klient-serwer**, umożliwiającego komunikację sieciową między wieloma klientami a centralnym serwerem. System zapewni niezawodną wymianę danych z wykorzystaniem protokołów TCP/IP.

## 2. Obsługa Współbieżności

Serwer zostanie zaimplementowany jako **aplikacja współbieżna**, zdolna do jednoczesnej obsługi wielu połączeń klienckich. Mechanizm wielowątkowości/wieloprocesowości zagwarantuje niezależne przetwarzanie żądań od różnych klientów bez wzajemnego blokowania.

## 3. Mechanizm Odkrywania Usług

System wykorzysta **adresację multicast** do dynamicznego wyszukiwania dostępnych serwerów w sieci lokalnej. Klienci będą mogli automatycznie lokalizować aktywne instancje serwera bez konieczności ręcznej konfiguracji adresów IP.

## 4. Komunikacja Główna

Podstawowa wymiana danych między klientem a serwerem będzie realizowana z wykorzystaniem **adresacji unicast**. Zapewni to dedykowane, niezawodne połączenia punkt-punkt dla każdej sesji komunikacyjnej.

## 5. Format Wymiany Danych

Przesyłanie informacji zostanie zrealizowane w **formacie binarnym** z wykorzystaniem struktury **TLV (Type-Length-Value)**. Takie podejście umożliwi efektywną serializację danych, elastyczność protokołu oraz łatwą rozszerzalność o nowe typy komunikatów.

## 6. Demonizacja Procesu Serwerowego

Serwer zostanie uruchomiony jako **proces demon** (daemon), działający w tle systemu operacyjnego. Wszystkie zdarzenia systemowe będą rejestrowane w **plikach logów systemowych** (syslog), co umożliwi monitoring i diagnostykę aplikacji.

## 7. Rozwiązywanie Nazw Domenowych

Aplikacja zaimplementuje obsługę **systemu nazw domenowych (DNS)** za pomocą standardowego API POSIX. Umożliwi to translację nazw symbolicznych na adresy IP oraz odwrotnie, zapewniając elastyczność konfiguracji i niezależność od statycznych adresów.

## 8. Zarządzanie Zasobami Współdzielonymi

Serwer zapewni mechanizmy **koordynacji dostępu do współdzielonych zasobów**, przechowywanych po stronie serwerowej. System blokad i synchronizacji uniemożliwi konflikty podczas jednoczesnej modyfikacji danych przez wielu klientów.

## 9. Kompleksowa Obsługa Błędów

Aplikacja będzie zawierać **wszechstronną obsługę stanów wyjątkowych i błędów**, obejmującą walidację danych wejściowych, kontrolę błędów sieciowych, timeouty połączeń oraz graceful degradation. Każdy błąd zostanie odpowiednio zarejestrowany i obsłużony.

## 10. Język Implementacji

Całość projektu zostanie zaimplementowana w **języku C**, zgodnie ze standardem POSIX, co zapewni wysoką wydajność, bezpośredni dostęp do niskopoziomowych funkcji systemowych oraz przenośność między różnymi platformami Unix/Linux.

## 11. Dokumentacja Techniczna

Projekt będzie zawierać **kompletną dokumentację techniczną** wygenerowaną przy użyciu systemu Doxygen. Dokumentacja zostanie dostarczona w formie **plików PDF** oraz dokumentacji HTML, obejmując opis API, diagramy architektury oraz instrukcje użytkowania.

---

# Założenia Funkcjonalne Aplikacji

## A. System Autoryzacji Użytkowników

System zapewni mechanizm **rejestracji i identyfikacji użytkowników** oparty na unikalnych identyfikatorach tekstowych (nick). Po nawiązaniu połączenia z serwerem, klient przesyła swój identyfikator, który podlega walidacji pod kątem:

- Poprawności formalnej (długość, dozwolone znaki)
- Unikalności w obrębie aktywnych sesji

Zaakceptowany użytkownik otrzymuje przydzielony **kontekst sesji**, umożliwiający dalszą interakcję z systemem egzaminacyjnym.

## B. Tryby Testowania

Aplikacja serwerowa udostępnia **dwa podstawowe tryby przeprowadzania egzaminów**:

### a) Tryb pojedynczego pytania

Użytkownik może w dowolnym momencie zażądać losowo wybranego pytania z bazy danych. Tryb ten służy do szybkiego sprawdzenia wiedzy lub treningu punktowego.

### b) Tryb testu standardowego

Użytkownik inicjuje formalną sesję egzaminacyjną obejmującą **10 losowo wybranych pytań bez powtórzeń**. Po zakończeniu testu system generuje kompletne podsumowanie wyników z oceną końcową.

## C. Baza Pytań Egzaminacyjnych

Serwer zarządza **centralną bazą pytań egzaminacyjnych** dotyczących zagadnień informatycznych (zgodnych z kwalifikacjami zawodowymi: E.12, E.13, EE.08 itp.). Każde pytanie w bazie zawiera:

- Treść pytania w języku polskim
- Zestaw 3-4 wariantów odpowiedzi
- Oznaczenie prawidłowej odpowiedzi
- Unikalny identyfikator pytania

Baza pytań jest odczytywana wyłącznie przez proces serwerowy, który zarządza bezpiecznym dostępem klientów do zasobów oraz **losowaniem pytań zgodnie z wybranym trybem testowania**.

## D. Weryfikacja Odpowiedzi

Po otrzymaniu pytania, klient przesyła wybraną odpowiedź do serwera w postaci **zakodowanej struktury TLV**. Serwer przeprowadza weryfikację poprawności odpowiedzi poprzez porównanie z prawidłowym wariantem przechowywanym w bazie. 

System zwraca klientowi szczegółowe informacje zwrotne obejmujące:

- Status odpowiedzi (poprawna/błędna)
- Opcjonalnie: oznaczenie prawidłowej odpowiedzi (w przypadku błędu)
- W trybie testu: **bieżący wynik pośredni i pozostałą liczbę pytań**

Po zakończeniu pełnego testu, serwer oblicza **wynik procentowy** i zwraca klientowi kompletne podsumowanie egzaminu.

## E. System Statystyk Użytkowników

Serwer utrzymuje **trwałe dane statystyczne** dla każdego zarejestrowanego użytkownika, indeksowane unikalnym identyfikatorem (nick). 

Przechowywane informacje obejmują:

- Całkowitą liczbę poprawnych odpowiedzi
- Liczbę przeprowadzonych testów standardowych
- Wynik ostatniego testu (% poprawnych odpowiedzi)
- Datę i czas ostatniej aktywności

Dane statystyczne są **aktualizowane w czasie rzeczywistym** po zakończeniu każdej sesji testowej oraz dostępne do odczytu poprzez dedykowane zapytania API.

## F. Zarządzanie Stanem Sesji

Serwer implementuje **maszynę stanów** dla każdego połączonego klienta, kontrolującą dozwolone operacje w zależności od aktualnej fazy interakcji. 

### Zdefiniowane stany sesji:

1. **CONNECTED** – połączenie nawiązane, oczekiwanie na autoryzację
2. **LOGGED_IN** – użytkownik zalogowany, możliwość wyboru trybu testu
3. **IN_TEST** – aktywna sesja testowa, oczekiwanie na odpowiedzi
4. **FINISHED** – test zakończony, dostępne podsumowanie

Każdemu stanowi odpowiada **ograniczony zestaw dozwolonych komunikatów TLV**, co zapewnia spójność protokołu i bezpieczeństwo logiki aplikacji.

---

**Uwaga:** Niniejszy dokument przedstawia założenia projektowe systemu egzaminacyjnego. Szczegółowa specyfikacja techniczna protokołu TLV, schematy bazy danych, diagramy UML oraz dokumentacja API zostaną zawarte w dokumentacji końcowej projektu.
