# Hardware Password Manager

![passmanager](https://github.com/user-attachments/assets/01ae791b-53bd-465e-a10c-61364f054fff)

## Hardware

For now project uses CYD (Cheap Yellow Display) as hardware. All source code located in `./firmware`

## Firmware

### How passwords stored
All passwords stored in single file in SPIFFS (for now, only internal flash supported, but next we will support sd cards), default path is `/passwords.crypt`. Content of this file crypted with AES-128 by master-key (this key used to unlock device). This code have to be at leas 16 symbols. If you decrypt file content you will see serialized json with such format:

```json

{
    "CATEGORY_0": {
        "service_0": [["login", "pass"], ["login2", "pass2"]],
        "service_1": [["login", "pass"], ["login2", "pass2"]]
    },
    "CATEGORY_1": {
        "service_2": [["login", "pass"], ["login2", "pass2"]],
        "service_3": [["login", "pass"], ["login2", "pass2"]]
    }
}
```

Default file content:

```json
{
    "web": {
        EMPTY
    }
}

```
In future we will have different special categories with different stored data formats. For example to store web sites passwords you need save:

* url (no full, just name of site)
* login
* password

For SSH creds you can use same format, because you need to store:

* host (ip:port)
* user name
* password

But if you want to store API tokens from different services you don't need to store login/pass. You just need 2 fields (host, token). 




## UART API

Works only in firmare compiled with defined `MAINTANCE_MODE`
Commands:
| Command  | Description | Auth level | Args |
| -------- | ----------- | ---------- | ---- |
| set_maintance  | Disable GUI on display | NO | - |
| sf_reboot | Reboot device via abort(); used to quit maintance_mode | NO | - |
| internal_dump | Writes on UART dump from stored passwords file (not decrypted) | Unlock + Master-key via UART call | - |
| fs_clear_init | NOT IMPLEMENTED - clear passwords file and base struct (see above) crypted with given password, used for first time setup| NO | - |
| add_pass | Bulk add passwords to device via json | Unlock + Master-key via UART call | Master-key && passwords in serialized json |


If command doesn't require arguments you have just to send command over UART and wait for output. If command requires some input you need to use following pipeline:

General descriptions

```
SEND_COMMAND -> WAIT 50 ms -> DEVICE waits 1 second - in this time you have to transmit command arguments ending with `\n` -> read output
```

Example



## Client
This is python client (for cross-platfrom) to upload new passwords and make sync via UART (USB)

### Passwords add json
```json
[  
    {"category": "web", "service": "yandex", "login": "email1@ya.ru", "password": "hello1"},
    {"category": "web", "service": "yandex", "login": "email2@ya.ru", "password": "hello2"}
]
```


