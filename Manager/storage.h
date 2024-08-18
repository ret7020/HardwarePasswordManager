#ifndef STORAGE_H
#define STORAGE_H

#include "FS.h"
#include "SD.h"
#include <ArduinoJson.h>

JsonDocument readPasswords(fs::FS &fs, const char *path)
{
    File file = fs.open(path);
    JsonDocument passwords;

    while (file.available())
    {
        deserializeJson(passwords, file.readStringUntil('\n'));
    }
    file.close();

    return passwords;
}

#endif