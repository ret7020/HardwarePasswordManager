# Hyprland Copy password to clipboard

Add following code to your hyprland config file:
```
bind=SUPER_SHIFT,P,exec,wl-copy $(python3 <SCRIPT> --bypass_checks 1 --command get_curr_value) && hyprctl notify -1 500 "rgb(32a852)" "Copied"
```

Change `<SCRIPT>` to absolute path to client.py script from this repo.