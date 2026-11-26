#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>

SSCMA AI;

const int CONFIDENCE_MIN = 70;
const unsigned long WINDOW_MS = 5000;
const unsigned long POLL_MS = 250;
const unsigned int MIN_HITS = 3;

bool connected = false;
bool collecting = false;

unsigned long windowStart = 0;
unsigned int hits[2] = {0, 0};
unsigned long lastHit[2] = {0, 0};

int confirmed = -1;

const char* breedName(int id) {
  return id == 0 ? "Berner Sennenhund" : "Golden Retriever";
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  Wire.setBufferSize(512);

  for (int attempt = 0; attempt < 5; attempt++) {
    if (AI.begin(&Wire, -1, 0x62, 10, 100000)) {
      connected = true;
      break;
    }
    delay(1000);
  }

  if (!connected) {
    Serial.println("Vision-Modul nicht erreichbar.");
  }
}

void loop() {
  if (!connected) {
    delay(1000);
    return;
  }

  int result = AI.invoke(1, false, false);
  unsigned long now = millis();

  // Nur die stärkste gültige Erkennung je Abfrage zählen.
  int bestClass = -1;
  int bestScore = -1;

  if (result == 0) {
    for (const auto& box : AI.boxes()) {
      if ((box.target == 0 || box.target == 1)
          && box.score >= CONFIDENCE_MIN
          && box.score > bestScore) {
        bestClass = box.target;
        bestScore = box.score;
      }
    }
  }

  // Bestätigung nach fünf Sekunden ohne passenden Treffer aufheben.
  if (confirmed != -1 && now - lastHit[confirmed] >= WINDOW_MS) {
    confirmed = -1;
  }

  // Abgelaufenes Zeitfenster auswerten.
  if (collecting && now - windowStart >= WINDOW_MS) {
    int winner = -1;

    if (hits[0] >= MIN_HITS && hits[0] > hits[1]) {
      winner = 0;
    } else if (hits[1] >= MIN_HITS && hits[1] > hits[0]) {
      winner = 1;
    }

    if (winner != -1 && now - lastHit[winner] < WINDOW_MS) {
      confirmed = winner;
    }

    collecting = false;
    hits[0] = 0;
    hits[1] = 0;
  }

  if (bestClass != -1) {
    lastHit[bestClass] = now;

    if (!collecting) {
      collecting = true;
      windowStart = now;
    }

    hits[bestClass]++;
  }

  if (confirmed == -1) {
    Serial.println("Nichts erkannt");
  } else {
    Serial.print(breedName(confirmed));
    Serial.println(" erkannt");
  }

  delay(POLL_MS);
}