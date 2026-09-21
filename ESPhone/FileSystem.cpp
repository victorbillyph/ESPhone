#include "FileSystem.h"
#include <LittleFS.h>

FileSystemClass Fs;

bool FileSystemClass::begin() {
  if (mounted_) return true;
  if (!LittleFS.begin()) {
    Serial.println("[Fs] ERRO: falha ao montar LittleFS");
    return false;
  }
  mounted_ = true;
  bool ok = ensureBaseDirs();
  Serial.printf("[Fs] LittleFS ok: %u B usados / %u B total\n",
                (unsigned)LittleFS.usedBytes() / 1024,
                (unsigned)LittleFS.totalBytes() / 1024);
  return ok;
}

bool FileSystemClass::mounted() const { return mounted_; }

bool FileSystemClass::ensureBaseDirs() {
  return ensureDir("/system") && ensureDir("/apps") &&
         ensureDir("/config") && ensureDir("/data");
}

bool FileSystemClass::ensureDir(const String &p) {
  if (p.length() == 0) return false;

  // cria nível a nível (LittleFS.mkdir cria 1 nível)
  String cur;
  for (unsigned i = 1; i < p.length(); i++) {
    if (p[i] == '/') {
      if (cur.length() > 0 && !LittleFS.exists(cur) && !LittleFS.mkdir(cur)) return false;
    }
    cur += p[i];
  }
  if (cur.length() > 0 && !LittleFS.exists(cur) && !LittleFS.mkdir(cur)) return false;

  // confirma que é diretório (File::isDirectory — a API do 3.3.x)
  if (!LittleFS.exists(p)) return false;
  File d = LittleFS.open(p);
  bool isDir = d && d.isDirectory();
  if (d) d.close();
  return isDir;
}

String FileSystemClass::readString(const char *path) {
  if (!LittleFS.exists(path)) return String();
  File f = LittleFS.open(path, "r");
  if (!f) return String();
  String out;
  while (f.available()) out += (char)f.read();
  f.close();
  return out;
}

bool FileSystemClass::writeString(const char *path, const String &v) {
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  size_t n = f.print(v);
  f.close();
  return n != 0;
}

bool FileSystemClass::exists(const char *path) {
  return LittleFS.exists(path);
}

bool FileSystemClass::remove(const char *path) {
  if (!LittleFS.exists(path)) return false;
  return LittleFS.remove(path);
}

String FileSystemClass::readJson(const char *path) { return readString(path); }
bool FileSystemClass::writeJson(const char *path, const String &v) { return writeString(path, v); }

String FileSystemClass::readJsonDir(const char *path) {
  String out = "[";
  File root = LittleFS.open(path);
  if (root && root.isDirectory()) {
    File f = root.openNextFile();
    bool first = true;
    while (f) {
      String name = String(f.name());
      int sl = name.lastIndexOf('/');
      if (sl >= 0) name = name.substring(sl + 1);
      if (!first) out += ",";
      first = false;
      out += "{\"name\":\"\""; // placeholder simples
      if (f.isDirectory()) out += ",\"dir\":true";
      else out += String(",\"size\":") + String(f.size());
      out += "}";
      f = root.openNextFile();
    }
  }
  out += "]";
  return out;
}

String FileSystemClass::appDir(const String &appId)     { return String("/apps/") + appId; }
String FileSystemClass::appDataDir(const String &appId) { return String("/data/") + appId; }
String FileSystemClass::configPath(const char *name)    { return String("/config/") + name; }
