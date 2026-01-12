# Specyfikacja Protokołu TLV

## 1. Struktura Podstawowa

Każdy komunikat w protokole wykorzystuje format **TLV (Type-Length-Value)**:

```c
struct tlv_header {
    uint16_t type;      // Typ wiadomości (network byte order)
    uint16_t length;    // Długość pola value w bajtach (network byte order)
    uint8_t value[];    // Dane zmiennej długości
};
```

## 2. Typy Komunikatów

| Typ | Kod (hex) | Nazwa | Kierunek | Opis |
|-----|-----------|-------|----------|------|
| 1 | 0x0001 | LOGIN_REQUEST | Client → Server | Żądanie logowania z nickiem użytkownika |
| 2 | 0x0002 | LOGIN_RESPONSE | Server → Client | Odpowiedź na próbę logowania (sukces/błąd) |
| 3 | 0x0003 | REQUEST_QUESTION | Client → Server | Żądanie pytania (tryb losowy lub test) |
| 4 | 0x0004 | QUESTION_DATA | Server → Client | Przesłanie pytania z odpowiedziami |
| 5 | 0x0005 | ANSWER_SUBMIT | Client → Server | Przesłanie odpowiedzi użytkownika |
| 6 | 0x0006 | ANSWER_RESULT | Server → Client | Wynik weryfikacji odpowiedzi |

---

## 3. Szczegółowa Specyfikacja Komunikatów

### 3.1. LOGIN_REQUEST (0x0001)

**Kierunek:** Client → Server

**Struktura value:**
```c
struct login_request {
    uint8_t nick_length;    // Długość nicku (1-32)
    char nick[];            // Nick użytkownika (UTF-8)
};
```

**Opis:**
- Klient wysyła swój nick do serwera w celu autoryzacji
- Nick musi mieć długość 1-32 znaki
- Dozwolone znaki: a-z, A-Z, 0-9, _, -

**Przykład:**
```
Type: 0x0001
Length: 0x0006 (6 bajtów)
Value: 0x05 'U' 's' 'e' 'r' '1'
       (length=5, nick="User1")
```

---

### 3.2. LOGIN_RESPONSE (0x0002)

**Kierunek:** Server → Client

**Struktura value:**
```c
struct login_response {
    uint8_t status;         // 0 = sukces, 1 = błąd (nick zajęty), 2 = błąd (nieprawidłowy nick)
    uint8_t message_length; // Długość komunikatu
    char message[];         // Komunikat tekstowy (opcjonalny)
};
```

**Opis:**
- Status 0: Logowanie pomyślne, sesja utworzona
- Status 1: Nick już jest w użyciu przez innego klienta
- Status 2: Nick zawiera niedozwolone znaki lub ma złą długość

**Przykład (sukces):**
```
Type: 0x0002
Length: 0x0010 (16 bajtów)
Value: 0x00 0x0F 'L' 'o' 'g' 'i' 'n' ' ' 's' 'u' 'c' 'c' 'e' 's' 's' '!'
       (status=0, message_length=15, message="Login success!")
```

---

### 3.3. REQUEST_QUESTION (0x0003)

**Kierunek:** Client → Server

**Struktura value:**
```c
struct request_question {
    uint8_t mode;           // 0 = pytanie losowe, 1 = test (10 pytań)
    uint8_t question_index; // Numer pytania w teście (0-9), ignorowane w trybie losowym
};
```

**Opis:**
- Mode 0: Pojedyncze losowe pytanie (trening)
- Mode 1: Rozpoczęcie/kontynuacja testu 10 pytań
- question_index: Użyte tylko w trybie testu (0 = pierwsze pytanie, 9 = ostatnie)

**Przykład (test, pierwsze pytanie):**
```
Type: 0x0003
Length: 0x0002 (2 bajty)
Value: 0x01 0x00
       (mode=1 [test], question_index=0)
```

---

### 3.4. QUESTION_DATA (0x0004)

**Kierunek:** Server → Client

**Struktura value:**
```c
struct question_data {
    uint16_t question_id;       // Unikalny ID pytania (network byte order)
    uint8_t num_answers;        // Liczba odpowiedzi (3-4)
    uint16_t question_length;   // Długość treści pytania (network byte order)
    char question_text[];       // Treść pytania (UTF-8)
    
    // Po question_text następuje num_answers bloków:
    struct {
        uint8_t answer_id;      // ID odpowiedzi (A=0, B=1, C=2, D=3)
        uint16_t answer_length; // Długość odpowiedzi (network byte order)
        char answer_text[];     // Treść odpowiedzi (UTF-8)
    } answers[];
};
```

**Opis:**
- Pytanie zawiera unikalny ID do identyfikacji
- Liczba odpowiedzi: 3 lub 4
- Każda odpowiedź ma swój identyfikator (A, B, C, D)
- Klient NIE otrzymuje informacji, która odpowiedź jest poprawna

**Przykład:**
```
Type: 0x0004
Length: 0x0058 (88 bajtów)
Value:
  question_id: 0x007B (123)
  num_answers: 0x03 (3)
  question_length: 0x0020 (32)
  question_text: "Co oznacza skrót TCP?"
  
  answer[0]:
    answer_id: 0x00 (A)
    answer_length: 0x0018 (24)
    answer_text: "Transmission Control Protocol"
  
  answer[1]:
    answer_id: 0x01 (B)
    answer_length: 0x0015 (21)
    answer_text: "Transfer Control Point"
  
  answer[2]:
    answer_id: 0x02 (C)
    answer_length: 0x0016 (22)
    answer_text: "Transport Code Protocol"
```

---

### 3.5. ANSWER_SUBMIT (0x0005)

**Kierunek:** Client → Server

**Struktura value:**
```c
struct answer_submit {
    uint16_t question_id;   // ID pytania, na które udzielana jest odpowiedź (network byte order)
    uint8_t answer_id;      // ID wybranej odpowiedzi (A=0, B=1, C=2, D=3)
};
```

**Opis:**
- Klient przesyła ID pytania oraz wybraną odpowiedź
- question_id musi zgadzać się z ostatnio otrzymanym pytaniem
- answer_id musi być w zakresie dostępnych odpowiedzi

**Przykład:**
```
Type: 0x0005
Length: 0x0003 (3 bajty)
Value: 0x007B 0x00
       (question_id=123, answer_id=0 [odpowiedź A])
```

---

### 3.6. ANSWER_RESULT (0x0006)

**Kierunek:** Server → Client

**Struktura value:**
```c
struct answer_result {
    uint16_t question_id;       // ID pytania (network byte order)
    uint8_t is_correct;         // 1 = poprawna, 0 = błędna
    uint8_t correct_answer_id;  // ID poprawnej odpowiedzi (A=0, B=1, C=2, D=3)
    uint8_t test_mode;          // 0 = tryb losowy, 1 = tryb testu
    uint8_t questions_answered; // Liczba udzielonych odpowiedzi w teście (0-10)
    uint8_t correct_count;      // Liczba poprawnych odpowiedzi w teście
};
```

**Opis:**
- Informacja czy odpowiedź była poprawna
- Zawsze zwracany jest ID poprawnej odpowiedzi (do nauki)
- W trybie testu: statystyki postępu (ile pytań, ile poprawnych)
- Po 10 pytaniach w teście: questions_answered = 10

**Przykład (odpowiedź poprawna w teście, 3/10 pytań, 2 poprawne):**
```
Type: 0x0006
Length: 0x0007 (7 bajtów)
Value: 0x007B 0x01 0x00 0x01 0x03 0x02
       (question_id=123, is_correct=1, correct_answer_id=0,
        test_mode=1, questions_answered=3, correct_count=2)
```

**Przykład (odpowiedź błędna w trybie losowym):**
```
Type: 0x0006
Length: 0x0007 (7 bajtów)
Value: 0x007B 0x00 0x00 0x00 0x01 0x00
       (question_id=123, is_correct=0, correct_answer_id=0,
        test_mode=0, questions_answered=1, correct_count=0)
```

---

## 4. Przepływ Komunikacji

### 4.1. Scenariusz: Pojedyncze Pytanie

```
Client                          Server
  |                               |
  |--- LOGIN_REQUEST ------------>|
  |<-- LOGIN_RESPONSE ------------|
  |                               |
  |--- REQUEST_QUESTION (mode=0)->|
  |<-- QUESTION_DATA -------------|
  |                               |
  |--- ANSWER_SUBMIT ------------>|
  |<-- ANSWER_RESULT -------------|
  |                               |
```

### 4.2. Scenariusz: Test 10 Pytań

```
Client                          Server
  |                               |
  |--- LOGIN_REQUEST ------------>|
  |<-- LOGIN_RESPONSE ------------|
  |                               |
  |--- REQUEST_QUESTION (mode=1)->|
  |<-- QUESTION_DATA (1/10) ------|
  |--- ANSWER_SUBMIT ------------>|
  |<-- ANSWER_RESULT (1/10) ------|
  |                               |
  |--- REQUEST_QUESTION (mode=1)->|
  |<-- QUESTION_DATA (2/10) ------|
  |--- ANSWER_SUBMIT ------------>|
  |<-- ANSWER_RESULT (2/10) ------|
  |                               |
  ... (pytania 3-9)               |
  |                               |
  |--- REQUEST_QUESTION (mode=1)->|
  |<-- QUESTION_DATA (10/10) -----|
  |--- ANSWER_SUBMIT ------------>|
  |<-- ANSWER_RESULT (10/10) -----|
  |     [Test zakończony]         |
```

---

## 5. Obsługa Błędów

Wszystkie błędy protokołu powinny być logowane przez serwer do syslog.

### Możliwe błędy:
- **Nieprawidłowy typ komunikatu** - nieznany kod typu
- **Nieprawidłowa długość** - length nie zgadza się z rzeczywistą długością value
- **Błąd sekwencji** - komunikat wysłany w niewłaściwym momencie (np. ANSWER_SUBMIT przed REQUEST_QUESTION)
- **Nieprawidłowe dane** - np. answer_id poza zakresem, question_id nie zgadza się

### Zalecenia:
- Każdy błąd powinien skutkować logiem w syslog
- Klient powinien otrzymać komunikat o błędzie
- Połączenie może być zamknięte w przypadku poważnych naruszeń protokołu

---

## 6. Uwagi Implementacyjne

1. **Byte Order**: Wszystkie pola wielobajtowe (uint16_t) przesyłane są w **network byte order** (big-endian). Używaj `htons()`, `ntohs()`, `htonl()`, `ntohl()`.

2. **Kodowanie tekstów**: Wszystkie stringi (nick, pytania, odpowiedzi) używają kodowania **UTF-8**.

3. **Maksymalne rozmiary**:
   - Nick: 32 bajty
   - Treść pytania: 1024 bajty
   - Treść odpowiedzi: 256 bajtów

4. **Timeout**: Klient powinien czekać maksymalnie 30 sekund na odpowiedź serwera.

5. **Bufory**: Zalecany rozmiar bufora odczytu: 4096 bajtów (pokrywa największy możliwy komunikat QUESTION_DATA).
