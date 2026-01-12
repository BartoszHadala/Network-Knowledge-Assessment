# Quiz System - Instrukcja użycia

## Uruchomienie serwera

```bash
# Z repo głównego:
./server 8080

# Lub z build/:
./build/server 8080
```

Serwer automatycznie:
- Załaduje pytania z `resources/questions.json`
- Uruchomi się jako demon (background)
- Logi będą w `/var/log/syslog`

## Sprawdzanie logów serwera

```bash
# Wszystkie logi:
tail -f /var/log/syslog | grep server

# Lub jeśli skonfigurowałeś rsyslog:
sudo cp deploy/rsyslog-mydaemon.conf /etc/rsyslog.d/99-mydaemon.conf
sudo systemctl restart rsyslog
tail -f /var/log/mydaemon.log
```

## Uruchomienie klienta

```bash
# Tryb discovery (automatyczne wykrycie serwera):
./client --discover

# Ręczne podanie IP i portu:
./client 127.0.0.1 8080
```

## Korzystanie z quizu

Po zalogowaniu wyświetli się menu:

### Opcja 1: Training Mode - Losowe pytanie
- Wybierz opcję `1`
- Otrzymasz losowe pytanie z bazy
- Wybierz odpowiedź (A, B, C, lub D)
- System poinformuje czy odpowiedź jest poprawna
- Możesz powtarzać ile razy chcesz

### Opcja 2: Knowledge Test - Test 10 pytań
- Wybierz opcję `2`
- Rozpocznie się test składający się z 10 pytań
- Po każdej odpowiedzi system pokazuje:
  - Czy odpowiedź była poprawna
  - Jaka była prawidłowa odpowiedź (jeśli źle)
  - Postęp (np. "Progress: 5/10 questions, 3 correct")
- Po zakończeniu widzisz wynik końcowy

## Przykładowy przebieg sesji

```
Enter your nickname: JohnDoe
✓ Login successful: Welcome JohnDoe

╔════════════════════════════════════════════╗
║         IT EXAM SYSTEM - MAIN MENU         ║
╠════════════════════════════════════════════╣
║  1. Training - Random question             ║
║  2. Knowledge test - Set of 10 questions   ║
...
╚════════════════════════════════════════════╝

Enter your choice (1-7): 1

=== Training Mode - Random Question ===

╔════════════════════════════════════════════════════════════════════════╗
║ QUESTION #3
╠════════════════════════════════════════════════════════════════════════╣
║ Co łączy okablowanie pionowe w projekcie sieci LAN?
╠════════════════════════════════════════════════════════════════════════╣
║ A) Dwa sąsiednie punkty abonenckie
║ B) Główny punkt rozdzielczy z pośrednimi punktami rozdzielczymi
║ C) Gniazdo abonenckie z pośrednim punktem rozdzielczym
║ D) Główny punkt rozdzielczy z gniazdem abonenckim
╚════════════════════════════════════════════════════════════════════════╝

Your answer (A-D): B

Submitting answer...
✓ Correct answer!
```

## Format pliku pytań (resources/questions.json)

```json
[
  {
    "id": 1,
    "pytanie": "Treść pytania",
    "odpowiedzi": ["Odp A", "Odp B", "Odp C", "Odp D"],
    "poprawna": 2
  }
]
```

- `id`: unikalny numer pytania
- `pytanie`: tekst pytania
- `odpowiedzi`: tablica odpowiedzi (do 4)
- `poprawna`: numer poprawnej odpowiedzi (1-4, nie 0-3!)

## Zatrzymanie serwera

```bash
# Znajdź PID:
pidof server

# Wyślij SIGTERM (graceful shutdown):
kill -TERM <PID>

# Lub SIGINT:
kill -INT <PID>

# Sprawdź logi:
tail -f /var/log/syslog | grep server
```

## Struktura protokołu

1. **LOGIN_REQUEST/RESPONSE** - autoryzacja klienta
2. **REQUEST_QUESTION** - żądanie pytania (mode: RANDOM lub TEST)
3. **QUESTION_DATA** - serwer wysyła pytanie + odpowiedzi
4. **ANSWER_SUBMIT** - klient wysyła odpowiedź
5. **ANSWER_RESULT** - serwer potwierdza poprawność

Wszystkie wiadomości używają formatu TLV (Type-Length-Value).
