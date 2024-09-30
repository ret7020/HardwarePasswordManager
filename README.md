# Hardware Password Manager

## Firmware

For now project uses CYD (Cheap Yellow Display) as hardware. All source code located in `./firmware`



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


