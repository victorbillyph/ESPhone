#ifndef ES_FS_H
#define ES_FS_H

#include <Arduino.h>
#include <LittleFS.h>

// LittleFS do ESPhone. Estrutura canonica de pastas (contrato com app nativo):
//   /system  arquivos do sistema (state, manifest, logo, bootcount)
//   /apps    um app por pasta  -> /apps/<appId>/
//   /config  configs           -> /config/<nome>.json
//   /data    dados por app     -> /data/<appId>/<arquivo>.json
// O setup (portal) e o app nativo falam com essas pastas via JSON.

class FileSystemClass {
public:
  bool begin();                  // monta LittleFS + pastas base
  bool mounted() const;          // true se begin() ok

  String readString(const char *path);  // "" se nao existe
  bool   writeString(const char *path, const String &v);
  bool   exists(const char *path);
  bool   remove(const char *path);

  String readJson(const char *path);        // JSON como texto
  bool   writeJson(const char *path, const String &v);
  String readJsonDir(const char *path);     // lista dir como JSON array

  String appDir(const String &appId);       // /apps/<appId>
  String appDataDir(const String &appId);   // /data/<appId>
  String configPath(const char *name);      // /config/<name>

private:
  bool ensureBaseDirs();        // cria /system /apps /config /data
  bool ensureDir(const String &p);  // mkdir recursivo
  bool mounted_ = false;
};

extern FileSystemClass Fs;

#endif
