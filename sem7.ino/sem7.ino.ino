#include <SPI.h>
#include <SdFat.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const size_t PATH_CAP = 80;  
static const size_t LINE_CAP = 160;

inline char asciiLower(char c) {
  return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

inline bool sameText(const char* a, const char* b) {
  while (*a && asciiLower(*a) == asciiLower(*b)) { ++a; ++b; }
  return *a == *b;
}

inline bool cliSpace(char c) { return c == ' ' || c == '\t'; }

inline char* skipSpaces(char* s) {
  while (cliSpace(*s)) ++s;
  return s;
}

inline char* nextToken(char*& p, bool& valid) {
  p = skipSpaces(p);
  if (!*p) return NULL;
  if (*p == '"') {
    char* start = ++p;
    while (*p && *p != '"') ++p;
    if (!*p) { valid = false; return NULL; }
    *p++ = '\0';
    if (*p && !cliSpace(*p)) valid = false;
    return start;
  }
  char* start = p;
  while (*p && !cliSpace(*p)) ++p;
  if (*p) *p++ = '\0';
  return start;
}

inline bool normalizePath(const char* cwd, const char* input,
                          char* out, size_t capacity) {
  if (!input || !*input || capacity < 2) return false;
  size_t length = 1;
  out[0] = '/'; out[1] = '\0';
  if (*input != '/') {
    length = strlen(cwd);
    if (length >= capacity) return false;
    memcpy(out, cwd, length + 1);
  }
  while (*input) {
    while (*input == '/') ++input;
    const char* part = input;
    while (*input && *input != '/') ++input;
    const size_t n = size_t(input - part);
    if (!n || (n == 1 && part[0] == '.')) continue;
    if (n == 2 && part[0] == '.' && part[1] == '.') {
      while (length > 1 && out[length - 1] != '/') --length;
      if (length > 1) --length;
      out[length] = '\0';
      continue;
    }
    const size_t separator = length > 1 ? 1 : 0;
    if (length + separator + n >= capacity) return false;
    if (separator) out[length++] = '/';
    memcpy(out + length, part, n);
    length += n;
    out[length] = '\0';
  }
  return true;
}

inline bool appendName(char* path, const char* name, size_t capacity) {
  const size_t length = strlen(path), n = strlen(name);
  const size_t separator = length > 1 ? 1 : 0;
  if (length + separator + n >= capacity) return false;
  if (separator) path[length] = '/';
  memcpy(path + length + separator, name, n + 1);
  return true;
}

inline bool isAncestor(const char* ancestor, const char* path) {
  if (sameText(ancestor, "/")) return true;
  while (*ancestor && asciiLower(*ancestor) == asciiLower(*path)) {
    ++ancestor; ++path;
  }
  return !*ancestor && (!*path || *path == '/');
}

inline bool hasBmpExtension(const char* path) {
  const char* dot = strrchr(path, '.');
  return dot && sameText(dot, ".bmp");
}


inline uint8_t crc4Byte(uint8_t crc, uint8_t value) {
  for (uint8_t bit = 0; bit < 8; ++bit) {
    const bool feedback = ((crc >> 3) ^ (value >> 7)) & 1;
    crc = (crc << 1) & 0x0F;
    if (feedback) crc ^= 0x07;
    value <<= 1;
  }
  return crc;
}

inline uint16_t little16(const uint8_t* p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}

inline uint32_t little32(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
         (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}


inline char pixelChar(uint8_t b, uint8_t g, uint8_t r) {
  const char chars[] = {' ', '@', '-', '+', '$', '%', '^', '&',
                         '*', ';', '/', ' ', '?', '#', '~', ' '};
  uint16_t best = 766;
  uint8_t selected = 0;
  for (uint8_t i = 0; i < 16; ++i) {
    uint8_t rr, gg, bb;
    if (i == 7) rr = gg = bb = 192;
    else if (i == 8) rr = gg = bb = 128;
    else {
      const uint8_t color = i > 8 ? i - 8 : i;
      const uint8_t level = i > 8 ? 255 : 128;
      rr = color & 4 ? level : 0;
      gg = color & 2 ? level : 0;
      bb = color & 1 ? level : 0;
    }
    const int16_t dr = int16_t(r) - rr;
    const int16_t dg = int16_t(g) - gg;
    const int16_t db = int16_t(b) - bb;
    const uint16_t distance = (dr < 0 ? -dr : dr) +
                              (dg < 0 ? -dg : dg) +
                              (db < 0 ? -db : db);
    if (distance < best) { best = distance; selected = i; }
  }
  return chars[selected];
}


const uint8_t SD_CS = 10;
const uint8_t MAX_DEPTH = 8;
SdFat32 sd;  
char cwd[PATH_CAP] = "/";
char line[LINE_CAP];
char pathA[PATH_CAP];
char pathB[PATH_CAP];
uint8_t ioBuffer[32];
uint8_t lineLength = 0;
bool lineOverflow = false;
bool sdReady = false;
bool previousCR = false;

void prompt() {
  Serial.print(cwd);
  Serial.print(F(" > "));
}

void errorIO() { Serial.println(F("Eroare de acces/citire/scriere SD.")); }

void help() {
  Serial.println(F("cd [cale] | mkdir cale | rmdir cale | ls"));
  Serial.println(F("cat fisier | echo \"text\" > fisier"));
  Serial.println(F("find nume | cp sursa director | crc fisier"));
  Serial.println(F("Cai absolute/relative, . si ..; help pentru ajutor."));
}

bool resolve(const char* argument, char* out) {
  if (!normalizePath(cwd, argument, out, PATH_CAP)) {
    Serial.println(F("Cale goala sau prea lunga (maxim 79 caractere)."));
    return false;
  }
  return true;
}

bool openFile(FatFile& file, const char* path) {
  if (!file.open(path, O_RDONLY) || !file.isFile()) {
    file.close();
    Serial.println(F("Fisier inexistent sau cale de director."));
    return false;
  }
  return true;
}

void printTwo(uint8_t value) {
  if (value < 10) Serial.print('0');
  Serial.print(value);
}

void printCreation(FatFile& entry) {
  uint16_t date, time;
  if (!entry.getCreateDateTime(&date, &time) || !date) {
    Serial.print(F("data indisponibila"));
    return;
  }
  Serial.print(1980 + (date >> 9)); Serial.print('-');
  printTwo((date >> 5) & 15); Serial.print('-');
  printTwo(date & 31); Serial.print(' ');
  printTwo(time >> 11); Serial.print(':');
  printTwo((time >> 5) & 63); Serial.print(':');
  printTwo((time & 31) * 2);
}

void listDirectory() {
  FatFile directory, entry;
  if (!directory.open(cwd, O_RDONLY) || !directory.isDir()) {
    directory.close(); errorIO(); return;
  }
  Serial.println(F("Tip\tCreat la\t\t\tBytes\tNume (8.3)"));
  while (entry.openNext(&directory, O_RDONLY)) {
    char name[13];
    if (!entry.getSFN(name, sizeof(name))) {
      entry.close(); errorIO(); break;
    }
    Serial.print(entry.isDir() ? 'd' : 'f'); Serial.print('\t');
    printCreation(entry); Serial.print('\t');
    Serial.print(entry.fileSize()); Serial.print('\t');
    Serial.println(name);
    entry.close();
  }
  if (directory.getError()) errorIO();
  directory.close();
}

void changeDirectory(const char* argument) {
  if (!argument) { Serial.println(cwd); return; }
  if (!resolve(argument, pathA)) return;
  FatFile directory;
  if (!directory.open(pathA, O_RDONLY) || !directory.isDir()) {
    Serial.println(F("Director inexistent."));
  } else {
    strcpy(cwd, pathA);
    Serial.println(cwd);
  }
  directory.close();
}

void removeDirectory(const char* argument) {
  if (!resolve(argument, pathA)) return;
  if (isAncestor(pathA, cwd)) {
    Serial.println(F("Nu se sterge radacina/directorul curent/un stramos."));
    return;
  }
  FatFile directory;
  if (!directory.open(pathA, O_RDONLY) || !directory.isDir()) {
    Serial.println(F("Director inexistent."));
  } else if (!directory.rmdir()) {
    Serial.println(F("Stergere esuata: director nevid sau eroare SD."));
  } else Serial.println(F("Director sters."));
  directory.close();
}

void printBMP(FatFile& file) {
  uint8_t header[54];
  if (file.read(header, sizeof(header)) != sizeof(header) ||
      header[0] != 'B' || header[1] != 'M') {
    Serial.println(F("Antet BMP invalid/incomplet.")); return;
  }
  const uint32_t dibSize = little32(header + 14);
  const uint32_t offset = little32(header + 10);
  const int32_t width = int32_t(little32(header + 18));
  const int32_t signedHeight = int32_t(little32(header + 22));
  if (dibSize < 40 || dibSize > file.fileSize() - 14 ||
      offset < 14 + dibSize || width < 1 || width > 170 ||
      signedHeight == 0 || signedHeight < -60 || signedHeight > 60 ||
      little16(header + 26) != 1 || little16(header + 28) != 24 ||
      little32(header + 30) != 0) {
    Serial.println(F("BMP nesuportat: cerut 24 biti, fara compresie, <=170x60."));
    return;
  }
  const uint32_t height = signedHeight < 0 ? -signedHeight : signedHeight;
  const uint32_t rowSize = (uint32_t(width) * 3 + 3) & ~uint32_t(3);
  if (offset > file.fileSize() ||
      rowSize * height > file.fileSize() - offset) {
    Serial.println(F("Date BMP incomplete.")); return;
  }
  for (uint32_t y = 0; y < height; y += 2) {
    const uint32_t row = signedHeight > 0 ? height - 1 - y : y;
    if (!file.seekSet(offset + rowSize * row)) { errorIO(); return; }
    for (int16_t x = 0; x < width; ++x) {
      uint8_t bgr[3];
      if (file.read(bgr, 3) != 3) { errorIO(); return; }
      Serial.print(pixelChar(bgr[0], bgr[1], bgr[2]));
    }
    Serial.println();
  }
}

void catFile(const char* argument) {
  if (!resolve(argument, pathA)) return;
  FatFile file;
  if (!openFile(file, pathA)) return;
  if (hasBmpExtension(pathA)) printBMP(file);
  else {
    int count;
    while ((count = file.read(ioBuffer, sizeof(ioBuffer))) > 0)
      Serial.write(ioBuffer, count);
    if (count < 0) errorIO();
    Serial.println();
  }
  file.close();
}

void echoFile(char* p) {
  p = skipSpaces(p);
  if (*p != '"') { Serial.println(F("Sintaxa: echo \"text\" > fisier")); return; }
  char* text = ++p;
  char* write = text;
  while (*p && *p != '"') {
    if (*p == '\\' && (p[1] == '"' || p[1] == '\\')) ++p;
    *write++ = *p++;
  }
  if (*p != '"') { Serial.println(F("Ghilimele neincheiate.")); return; }
  ++p;
  *write = '\0';
  p = skipSpaces(p);
  if (*p++ != '>') { Serial.println(F("Lipseste >.")); return; }
  bool valid = true;
  char* filename = nextToken(p, valid);
  if (!filename || !*filename || nextToken(p, valid) || !valid) {
    Serial.println(F("Dupa > trebuie un singur nume de fisier.")); return;
  }
  if (!resolve(filename, pathA)) return;
  FatFile file;
  if (!file.open(pathA, O_WRONLY | O_CREAT | O_TRUNC)) { errorIO(); return; }
  const size_t count = strlen(text);
  const bool written = file.write(text, count) == count;
  const bool closed = file.close();
  if (written && closed) Serial.println(F("Fisier scris (suprascris daca exista)."));
  else errorIO();
}

void findFiles(const char* target) {
  if (strchr(target, '/') || !*target) {
    Serial.println(F("find primeste un nume 8.3, fara cale.")); return;
  }
  strcpy(pathB, cwd);
  uint32_t resume[MAX_DEPTH + 1] = {0};
  uint8_t parentLength[MAX_DEPTH + 1];
  uint8_t depth = 0;
  bool found = false;
  bool incomplete = false;
  FatFile directory, entry;
  while (true) {
    if (!directory.open(pathB, O_RDONLY) || !directory.isDir() ||
        !directory.seekSet(resume[depth])) {
      directory.close(); errorIO(); return;
    }
    if (!entry.openNext(&directory, O_RDONLY)) {
      const bool failed = directory.getError();
      directory.close();
      if (failed) { errorIO(); return; }
      if (!depth) break;
      pathB[parentLength[depth--]] = '\0';
      continue;
    }
    resume[depth] = directory.curPosition();
    char name[13];
    const bool isDir = entry.isDir();
    const bool isFile = entry.isFile();
    const bool named = entry.getSFN(name, sizeof(name));
    entry.close(); directory.close();
    if (!named) { errorIO(); return; }
    if (sameText(name, ".") || sameText(name, "..")) continue;
    const uint8_t previousLength = strlen(pathB);
    if (!appendName(pathB, name, PATH_CAP)) {
      incomplete = true; continue;
    }
    if (isFile && sameText(name, target)) {
      Serial.println(pathB); found = true;
    }
    if (isDir && depth < MAX_DEPTH) {
      ++depth;
      parentLength[depth] = previousLength;
      resume[depth] = 0;
    } else {
      if (isDir) incomplete = true;
      pathB[previousLength] = '\0';
    }
  }
  if (!found) Serial.println(F("Niciun fisier gasit."));
  if (incomplete)
    Serial.println(F("Cautare partiala: cale >79 caractere sau adancime >8..."));
}

void copyFile(const char* source, const char* destination) {
  if (!resolve(source, pathA) || !resolve(destination, pathB)) return;
  FatFile input, output;
  if (!output.open(pathB, O_RDONLY) || !output.isDir()) {
    output.close(); Serial.println(F("Destinatia trebuie sa fie un director...")); return;
  }
  output.close();
  if (!openFile(input, pathA)) return;
  char name[13];
  if (!input.getSFN(name, sizeof(name)) || !appendName(pathB, name, PATH_CAP)) {
    input.close(); Serial.println(F("Cale destinatie prea lunga.")); return;
  }
  if (!output.open(pathB, O_WRONLY | O_CREAT | O_EXCL)) {
    input.close(); Serial.println(F("Destinatia exista deja sau nu poate fi creata.")); return;
  }
  bool ok = true;
  int count;
  while ((count = input.read(ioBuffer, sizeof(ioBuffer))) > 0) {
    if (output.write(ioBuffer, count) != size_t(count)) { ok = false; break; }
  }
  if (count < 0) ok = false;
  input.close();
  if (!output.close()) ok = false;
  if (ok) Serial.println(F("Fisier copiat."));
  else {
    sd.remove(pathB);  
    errorIO();
  }
}

void crcFile(const char* argument) {
  if (!resolve(argument, pathA)) return;
  FatFile file;
  if (!openFile(file, pathA)) return;
  uint8_t crc = 0;
  int count;
  while ((count = file.read(ioBuffer, sizeof(ioBuffer))) > 0)
    for (int i = 0; i < count; ++i) crc = crc4Byte(crc, ioBuffer[i]);
  file.close();
  if (count < 0) { errorIO(); return; }
  Serial.print(F("CRC-4 = 0x")); Serial.print(crc, HEX);
  Serial.print(F(" ("));
  for (int8_t bit = 3; bit >= 0; --bit) Serial.print((crc >> bit) & 1);
  Serial.println(')');
}

void execute(char* commandLine) {
  bool valid = true;
  char* cursor = commandLine;
  char* command = nextToken(cursor, valid);
  if (!valid) { Serial.println(F("Sintaxa invalida.")); return; }
  if (!command) return;
  if (sameText(command, "echo")) { echoFile(cursor); return; }
  char* a = nextToken(cursor, valid);
  char* b = nextToken(cursor, valid);
  char* extra = nextToken(cursor, valid);
  if (!valid || extra || (a && !*a) || (b && !*b)) {
    Serial.println(F("Argumente invalide.")); return;
  }
  if (sameText(command, "help") && !a) help();
  else if (sameText(command, "ls") && !a) listDirectory();
  else if (sameText(command, "cd") && !b) changeDirectory(a);
  else if (sameText(command, "mkdir") && a && !b) {
    if (resolve(a, pathA))
      Serial.println(sd.mkdir(pathA, true) ? F("Director creat/existent.") : F("Creare esuata."));
  }
  else if (sameText(command, "rmdir") && a && !b) removeDirectory(a);
  else if (sameText(command, "cat") && a && !b) catFile(a);
  else if (sameText(command, "find") && a && !b) findFiles(a);
  else if (sameText(command, "cp") && a && b) copyFile(a, b);
  else if (sameText(command, "crc") && a && !b) crcFile(a);
  else Serial.println(F("Comanda necunoscuta sau argumente gresite. Scrie help."));
}

void setup() {
  Serial.begin(9600);
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  sdReady = sd.begin(SD_CS, SD_SCK_MHZ(4));
  if (!sdReady) {
    Serial.println(F("Initializare SD esuata. Verifica CS, cablarea si FAT16/FAT32."));
    Serial.println(F("Dupa remediere apasa RESET."));
    return;
  }
  Serial.println(F("CLI SD pentru Arduino Uno. Serial: 9600 baud, Newline/CRLF."));
  help(); prompt();
}

void loop() {
  if (!sdReady) return;
  while (Serial.available() > 0) {
    const char c = Serial.read();
    if (c == '\n' && previousCR) { previousCR = false; continue; }
    previousCR = c == '\r';
    if (c == '\r' || c == '\n') {
      Serial.println();
      if (lineOverflow) Serial.println(F("Linie prea lunga (maxim 159 caractere)."));
      else { line[lineLength] = '\0'; execute(line); }
      lineLength = 0; lineOverflow = false; prompt();
    } else if ((c == '\b' || c == 127) && !lineOverflow) {
      if (lineLength) --lineLength;
    } else if (uint8_t(c) >= 32 || c == '\t') {
      if (lineLength < LINE_CAP - 1 && !lineOverflow) line[lineLength++] = c;
      else lineOverflow = true;  
    }
  }
}
