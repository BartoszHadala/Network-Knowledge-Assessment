---
title: ""
header-includes: |
  \usepackage{graphicx}
---

\begin{center}
\includegraphics[width=0.4\textwidth]{csm_agh_znak_wielobarwn_bez_nazwy_d4b64672bc.jpg}
\end{center}

\vspace{2cm}

\begin{center}
{\Huge \textbf{Network Knowledge Assessment}}

\vspace{0.5cm}

{\Large Dokumentacja Projektu}

\vspace{2cm}

{\large Bartosz Hadała}

\vspace{1cm}

{\large Styczeń 2026}
\end{center}

\newpage

## Spis treści

1. [Architektura systemu](#architektura-systemu)
2. [Protokół komunikacji (TLV)](#protokół-komunikacji-tlv)
3. [Komponenty serwera](#komponenty-serwera)
4. [Komponenty klienta](#komponenty-klienta)
5. [Baza pytań (Quiz)](#baza-pytań-quiz)
6. [System rankingów](#system-rankingów)
7. [Statystyki serwera](#statystyki-serwera)
8. [Struktura plików](#struktura-plików)
9. [Kompilacja i uruchomienie](#kompilacja-i-uruchomienie)
10. [Bezpieczeństwo i niezawodność](#bezpieczeństwo-i-niezawodność)

---

## Architektura systemu

System składa się z trzech głównych komponentów:

```
┌─────────────────┐         ┌─────────────────┐
│                 │         │                 │
│     Client      │◄───────►│     Server      │
│   (Frontend)    │  TCP/IP │   (Daemon)      │
│                 │  IPv6   │                 │
└─────────────────┘         └────────┬────────┘
                                     │
                            ┌────────▼────────┐
                            │   Quiz DB       │
                            │ (JSON file)     │
                            └─────────────────┘
```

### Cechy systemu

- **Architektura**: Client-Server
- **Protokół transportowy**: TCP over IPv6
- **Model I/O**: Event-driven (epoll) na serwerze
- **Serializacja**: Custom TLV (Type-Length-Value)
- **Daemon**: Unix daemon z syslog logging
- **Wielowątkowość**: Thread-safe statistics (mutexes)

---

## Protokół komunikacji (TLV)

### Format wiadomości TLV

Każda wiadomość składa się z trzech pól:

```
┌─────────────┬──────────────┬──────────────────────┐
│   Type      │   Length     │       Value          │
│  (2 bytes)  │  (2 bytes)   │   (variable size)    │
└─────────────┴──────────────┴──────────────────────┘
```

- **Type**: 16-bitowy identyfikator typu wiadomości (network byte order)
- **Length**: 16-bitowa długość pola Value w bajtach (network byte order)
- **Value**: Dane wiadomości (zależne od typu)

### Typy wiadomości

| Type ID | Nazwa                     | Kierunek | Opis                                      |
|---------|---------------------------|----------|-------------------------------------------|
| 0x0001  | LOGIN_REQUEST             | C→S      | Żądanie logowania z nickiem               |
| 0x0002  | LOGIN_RESPONSE            | S→C      | Odpowiedź na logowanie                    |
| 0x0003  | REQUEST_QUESTION          | C→S      | Żądanie pytania (tryb: random/test)       |
| 0x0004  | QUESTION_DATA             | S→C      | Przesłanie pytania z odpowiedziami        |
| 0x0005  | ANSWER_SUBMIT             | C→S      | Przesłanie odpowiedzi użytkownika         |
| 0x0006  | ANSWER_RESULT             | S→C      | Wynik sprawdzenia odpowiedzi              |
| 0x0007  | SUBMIT_SCORE              | C→S      | Przesłanie wyniku testu (wynik + czas)    |
| 0x0008  | REQUEST_RANKING           | C→S      | Żądanie tabeli rankingowej                |
| 0x0009  | RANKING_DATA              | S→C      | Top 10 wyników                            |
| 0x000A  | REQUEST_SERVER_INFO       | C→S      | Żądanie informacji o serwerze             |
| 0x000B  | SERVER_INFO_DATA          | S→C      | Statystyki serwera                        |
### Przykład: LOGIN_REQUEST

**Struktura Value:**

```
┌──────────────┬────────────────────────┐
│  nick_length │        nick            │
│   (1 byte)   │   (variable, max 32)   │
└──────────────┴────────────────────────┘
```

**Przykład** (nick "Janek"):

```
[00 01] [00 06] [05] [4A 61 6E 65 6B]
  Type   Length  len   J  a  n  e  k
```

- Type: `0x0001` (LOGIN_REQUEST)
- Length: `0x0006` (6 bajtów)
- Value: `[05]` (długość nicku) + `"Janek"` (5 znaków)

### Przykład: QUESTION_DATA

**Struktura Value:**

```
┌─────────────┬───────────────┬──────────────┬────────────────┐
│ question_id │ question_len  │  question    │  num_answers   │
│  (2 bytes)  │  (2 bytes)    │  (variable)  │   (1 byte)     │
└─────────────┴───────────────┴──────────────┴────────────────┘
```

---

## Komponenty serwera

### 1. Inicjalizacja demona (`deamon_init.c`)

**Proces daemonizacji:**

1. `fork()` → Rodzic kończy działanie
2. `setsid()` → Utworzenie nowej sesji
3. `signal(SIGHUP, SIG_IGN)` → Ignorowanie SIGHUP
4. `fork()` → Ponowne rozwidlenie, rodzic kończy
5. `chdir("/tmp")` → Zmiana katalogu roboczego
6. `umask(0)` → Ustawienie maski uprawnień
7. Zamknięcie wszystkich deskryptorów oprócz socketa
8. Przekierowanie stdin/stdout/stderr → `/dev/null`
9. `setuid()` → Zmiana użytkownika

### 2. Event Loop (epoll)

```c
┌─────────────────────────────────────────┐
│         epoll_wait()                    │
│  (czeka na zdarzenia na socketach)      │
└────────────┬────────────────────────────┘
             │
    ┌────────▼───────────┐
    │   New connection?  │
    └────────┬───────────┘
             │ YES
    ┌────────▼──────────────────────┐
    │ accept() + epoll_ctl(ADD)     │
    │ stats.total_connections++     │
    │ stats.active_connections++    │
    └───────────────────────────────┘
             │ NO
    ┌────────▼─────────┐
    │   Data ready?    │
    └────────┬─────────┘
             │
    ┌────────▼──────────────────────┐
    │   recv() TLV message          │
    │   Parse header (type)         │
    └────────┬──────────────────────┘
    ┌────────▼──────────────────────┐
    │   Handle message by type:     │
    │   - LOGIN                     │
    │   - REQUEST_QUESTION          │
    │   - ANSWER_SUBMIT             │
    │   - SUBMIT_SCORE              │
    │   - REQUEST_RANKING           │
    │   - REQUEST_SERVER_INFO       │
    └───────────────────────────────┘
```

### 3. Quiz Database (`quiz.c`)

**Struktury danych:**

```c
typedef struct {
    int id;
    char pytanie[512];
    char odpowiedzi[4][256];
    int num_odpowiedzi;
    int poprawna;
} Question;

typedef struct {
    Question *questions;
    int count;
    int capacity;
} QuizDatabase;
```

**Funkcje:**

- `quiz_load_questions()` - Ładowanie pytań z pliku JSON (cJSON)
- `quiz_get_random_question()` - Losowe pytanie z bazy
- `quiz_get_question_by_id()` - Pobranie pytania po ID
- `quiz_check_answer()` - Walidacja odpowiedzi użytkownika

**Ścieżki poszukiwania pliku:**

1. `resources/questions.json`
2. `../resources/questions.json`
3. `/usr/share/networkexam/questions.json`

### 4. Statystyki serwera

**Struktura danych:**

```c
struct server_stats {
    time_t start_time;           // Timestamp uruchomienia
    uint32_t total_connections;  // Łączna liczba połączeń
    uint32_t active_connections; // Aktywne połączenia
    uint32_t tests_completed;    // Ukończone testy
    uint32_t questions_asked;    // Zadane pytania
    uint32_t total_score;        // Suma wyników (dla średniej)
    uint8_t best_score;          // Najlepszy wynik
    uint32_t best_time;          // Najlepszy czas
    char best_player[32];        // Nick rekordzisty
};
```

**Aktualizacja statystyk:**

- Nowe połączenie → `total_connections++`, `active_connections++`
- Zamknięcie → `active_connections--`
- Pytanie wysłane → `questions_asked++`
- Test ukończony → `tests_completed++`, `total_score += score`
- Nowy rekord → aktualizacja `best_score`, `best_time`, `best_player`

### 5. System rankingów

**Struktury danych:**

```c
struct score_entry {
    char nick[32];
    uint8_t score;        
    uint32_t time_seconds;
};

// Globalna tablica (thread-safe z mutex)
struct score_entry rankings[MAX_RANKINGS];
int rankings_count = 0;
pthread_mutex_t rankings_mutex;
```

**Sortowanie:**

1. Według wyniku (malejąco)
2. Przy równym wyniku: według czasu (rosnąco)

**Zwracanie:** Top 10 wyników

---

## Komponenty klienta

### 1. Menu główne (`menu.c`)

```
╔════════════════════════════════════════════╗
║         Network Knowledge Test             ║
╠════════════════════════════════════════════╣
║  1. Training - Random question             ║
║  2. Knowledge test - Set of 10 questions   ║
║  3. Ranking / User statistics              ║
║  4. Server information                     ║
║  5. Exit                                   ║
╚════════════════════════════════════════════╝
```

### 2. Przepływ interakcji

**Diagram przepływu:**

```
┌──────────┐
│  Start   │
└──────┬───┘
       │
┌──────▼────────────────┐
│  Multicast Discovery  │
│  Find server IP/Port  │
└──────┬────────────────┘
       │
┌──────▼─────────┐
│  Connect TCP   │
│  IPv6 socket   │
└──────┬─────────┘
       │
┌──────▼──────────┐
│  LOGIN_REQUEST  │
│  Send nickname  │
└──────┬──────────┘
       │
┌──────▼──────────┐
│  Main menu loop │
└──────┬──────────┘
       │
       ├─[1]──► Random Question
       │        ├─► REQUEST_QUESTION (mode=RANDOM)
       │        ├─► Display question
       │        ├─► Get user answer
       │        ├─► ANSWER_SUBMIT
       │        └─► Show result
       │
       ├─[2]──► Test (10 questions)
       │        ├─► Start timer
       │        ├─► Loop 10x:
       │        │   ├─► REQUEST_QUESTION (mode=TEST)
       │        │   ├─► Display question
       │        │   ├─► Get answer
       │        │   ├─► ANSWER_SUBMIT
       │        │   └─► Track correct count
       │        ├─► Calculate time
       │        ├─► Display score (X/10, MM:SS)
       │        └─► SUBMIT_SCORE
       │
       ├─[3]──► Rankings
       │        ├─► REQUEST_RANKING
       │        ├─► Receive RANKING_DATA
       │        └─► Display table
       │
       ├─[4]──► Server Info
       │        ├─► REQUEST_SERVER_INFO
       │        ├─► Receive SERVER_INFO_DATA
       │        └─► Display stats
       │
       └─[5]──► Exit
```

### 3. Wyświetlanie pytania

**Przykładowy format:**

```
═══════════════════════════════════════════════════════════════════════
Question #42:

  Które z poniższych stwierdzeń najlepiej opisuje działanie protokołu
  TCP w warstwie transportowej modelu ISO/OSI?

  A) TCP zapewnia niezawodną transmisję danych poprzez
     potwierdzenia odbioru i retransmisję utraconych pakietów.

  B) TCP nie gwarantuje dostarczenia pakietów w odpowiedniej
     kolejności.

  C) TCP działa w warstwie sieciowej i obsługuje routing.

  D) TCP jest protokołem bezpołączeniowym, działającym bez
     nawiązywania sesji.
═══════════════════════════════════════════════════════════════════════

Your answer (A-D): _
```

### 4. Wynik testu

**Podsumowanie testu:**

```
╔═══════════════════════════════════════╗
║         TEST COMPLETE                 ║
╠═══════════════════════════════════════╣
║  Your Score: 7/10                     ║
║  Time: 02:34                          ║
╚═══════════════════════════════════════╝
```

### 5. Ranking

**Tabela rankingowa:**

```
╔════════════════════════════════════════════════════════════════╗
║                       TOP PLAYERS RANKING                      ║
╠════════════════════════════════════════════════════════════════╣
║  Rank  Nick                Score    Time                       ║
╠════════════════════════════════════════════════════════════════╣
║  1     Janek               10/10    02:15                      ║
║  2     Anna                9/10     01:45                      ║
║  3     Bartosz             9/10     03:20                      ║
║  4     Piotr               8/10     02:50                      ║
║  5     Maria               8/10     03:10                      ║
╚════════════════════════════════════════════════════════════════╝
```

### 6. Informacje o serwerze

**Ekran informacji o serwerze:**

```
╔════════════════════════════════════════════════════════════════╗
║                     SERVER INFORMATION                         ║
╠════════════════════════════════════════════════════════════════╣
║  Server Port:           7777                                   ║
║  Uptime:                2h 34m                                 ║
║                                                                ║
║  Active Connections:    3                                      ║
║  Total Connections:     47                                     ║
║                                                                ║
║  Questions Database:    31 questions                           ║
║  Tests Completed:       15                                     ║
║  Questions Asked:       167                                    ║
║  Average Score:         7/10                                   ║
║                                                                ║
║  Best Score:            Janek               10/10 (02:15)      ║
║  Rankings Entries:      15                                     ║
╚════════════════════════════════════════════════════════════════╝
```

---

## Baza pytań (Quiz)

### Format JSON

**Struktura pliku pytań:**

```json
[
  {
    "id": 1,
    "pytanie": "Które z poniższych stwierdzeń najlepiej opisuje działanie protokołu TCP?",
    "odpowiedzi": [
      "TCP zapewnia niezawodną transmisję danych poprzez potwierdzenia odbioru i retransmisję utraconych pakietów.",
      "TCP nie gwarantuje dostarczenia pakietów w odpowiedniej kolejności.",
      "TCP działa w warstwie sieciowej i obsługuje routing.",
      "TCP jest protokołem bezpołączeniowym, działającym bez nawiązywania sesji."
    ],
    "poprawna": 1
  },
  {
    "id": 2,
    "pytanie": "Co oznacza skrót DNS?",
    "odpowiedzi": [
      "Data Network Service",
      "Domain Name System",
      "Digital Network Security",
      "Distributed Node Service"
    ],
    "poprawna": 2
  }
]
```

### Walidacja

- **id**: Unikalny identyfikator pytania
- **pytanie**: Treść pytania
- **odpowiedzi**: Tablica 2-4 odpowiedzi
- **poprawna**: Indeks poprawnej odpowiedzi

---

## System rankingów

### Mechanizm działania

**1. Zbieranie wyników:**

- Po ukończeniu testu (10 pytań) klient wysyła `SUBMIT_SCORE`
- Serwer zapisuje: `{nick, score, time_seconds}`

**2. Przechowywanie:**

- Globalna tablica: `rankings[MAX_RANKINGS]`
- Ochrona: `pthread_mutex_t rankings_mutex`
- Limit: 100 wpisów (MAX_RANKINGS)

**3. Sortowanie:**

```c
// Kryterium 1: Wynik (malejąco)
if (sorted[j].score < sorted[j+1].score)
    swap();

// Kryterium 2: Przy równym wyniku - czas (rosnąco)
if (sorted[j].score == sorted[j+1].score && 
    sorted[j].time_seconds > sorted[j+1].time_seconds)
    swap();
```

**4. Wysyłanie:**

- Top 10 wyników
- Format: `RANKING_DATA` (TLV)

---

## Statystyki serwera

### Zbierane metryki

| Metryka               | Typ       | Opis                                  |
|-----------------------|-----------|---------------------------------------|
| `start_time`          | time_t    | Timestamp uruchomienia serwera        |
| `total_connections`   | uint32_t  | Suma wszystkich połączeń              |
| `active_connections`  | uint32_t  | Aktualna liczba klientów              |
| `tests_completed`     | uint32_t  | Liczba ukończonych testów             |
| `questions_asked`     | uint32_t  | Suma zadanych pytań                   |
| `total_score`         | uint32_t  | Suma wyników (dla średniej)           |
| `best_score`          | uint8_t   | Najlepszy wynik (0-10)                |
| `best_time`           | uint32_t  | Czas najlepszego wyniku (sekundy)     |
| `best_player`         | char[32]  | Nick gracza z najlepszym wynikiem     |

### Kalkulowane wartości

- **Uptime**: `current_time - start_time`
- **Average Score**: `total_score / tests_completed`
- **Liczba pytań w bazie**: `quiz_db.count`

---

## Struktura plików

```
ProjetPS/
├── build/                      # Katalog build (CMake)
│   ├── server                  # Skompilowany daemon
│   └── client                  # Skompilowany klient
├── src/
│   ├── server.c               # Main server (epoll, event loop)
│   ├── server_utils.c         # Login handling, validation
│   ├── client.c               # Main client (menu loop)
│   ├── client_utils.c         # TLV send/receive, display
│   ├── menu.c                 # Menu interface
│   ├── quiz.c                 # Quiz database management
│   ├── tlv.c                  # TLV protocol implementation
│   ├── deamon_init.c          # Daemonization logic
│   ├── multicast_discovery.c  # Server discovery (multicast)
│   └── sock_options.c         # Socket options helper
├── include/
│   ├── server_types.h         # Server structures (stats, scores)
│   ├── server_utils.h         # Server utility functions
│   ├── client_utils.h         # Client utility functions
│   ├── menu.h                 # Menu interface
│   ├── quiz.h                 # Quiz structures & functions
│   ├── tlv.h                  # TLV protocol definitions
│   ├── deamon_init.h          # Daemon initialization
│   ├── multicast_discovery.h  # Multicast discovery
│   └── sock_options.h         # Socket options
├── resources/
│   └── questions.json         # Baza pytań (31 pytań)
├── deploy/
│   ├── rsyslog-mydaemon.conf  # Konfiguracja syslog
│   └── logrotate-mydaemon     # Rotacja logów
├── doc/
│   ├── PROJECT_DOCUMENTATION.md  # Ten dokument
│   ├── protocol.md            # Szczegóły protokołu
│   └── html/                  # Doxygen documentation
├── CMakeLists.txt             # CMake configuration
├── Doxyfile                   # Doxygen configuration
└── README.md                  # Project overview
```

---

## Kompilacja i uruchomienie

### Kompilacja

```bash
cd build
cmake ..
make
```

### Uruchomienie serwera

```bash
# Uruchomienie na porcie 7777
./server 7777

# Domyślnie: port 8888
./server
```

### Uruchomienie klienta

```bash
# Bezpośrednie połączenie
./client <IP> <PORT>

# Z multicast discovery
./client --discover
```

### Logi serwera

```bash
# Monitoring logów w czasie rzeczywistym
tail -f /var/log/mydaemon.log

# Ostatnie 100 linii
tail -n 100 /var/log/mydaemon.log
```

### Zatrzymanie serwera

```bash

# Force kill
pkill -9 server
```

---

## Bezpieczeństwo i niezawodność

### 1. Thread Safety

**Mutexes:**

- `rankings_mutex` - ochrona tablicy rankingów
- `stats_mutex` - ochrona statystyk serwera

### 2. Obsługa błędów

**Przykład:**

```c
// Validation w TLV parsing
if (nick_len >= MAX_NICK_LENGTH) {
    return -1;  // Ochrona przed buffer overflow
}

### 3. Resource limits

- **Maksymalne połączenia:** `MAXEVENTS = 2000`
- **Maksymalne rankingi:** `MAX_RANKINGS = 100`
- **Maksymalna długość nicku:** `MAX_NICK_LENGTH = 32`
- **Buffer size:** `BUFFER_SIZE = 4096`

---

## Możliwości rozwoju

### 1. Funkcjonalności

- [ ] Kategorie pytań (sieci, systemy, programowanie)
- [ ] Poziomy trudności
- [ ] Statystyki osobiste użytkownika
- [ ] Tryb wieloosobowy (współzawodnictwo)
- [ ] Eksport wyników do PDF

### 2. Techniczne

- [ ] SSL/TLS szyfrowanie
- [ ] Baza danych (PostgreSQL/MySQL) zamiast JSON
- [ ] Load balancing (multiple servers)
- [ ] Rate limiting (DDoS protection)

### 3. UI/UX

- [ ] Web interface (HTTP REST API)
- [ ] Mobile app (Android/iOS)
- [ ] Graficzny klient (GTK/Qt)

---

## Licencja i autorzy

**Projekt**: Network Knowledge Assessment  
**Technologie**: C, TCP/IPv6, epoll, cJSON, CMake  
**Rok**: 2026  
**Uniwersytet**: AGH Krakóœ - Semestr 5 - Programowanie Sieciowe

---

## Linki i referencje

- [RFC 793 - TCP](https://tools.ietf.org/html/rfc793)
- [RFC 2460 - IPv6](https://tools.ietf.org/html/rfc2460)
- [epoll man page](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [cJSON library](https://github.com/DaveGamble/cJSON)
- [Unix Daemon Best Practices](https://www.freedesktop.org/software/systemd/man/daemon.html)

