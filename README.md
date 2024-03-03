## wCenterWindow

Copyright (c) 2020-2024 Vladislav Salikov aka W0LF.<br>
Program icon made by Kirill Topov.

---

This program centers the current active window by a `LCTRL + LWIN + C` hotkey, or pressing `LCTRL + LWIN + MMB` (Middle Mouse Button).

`LCTRL + LWIN + V` - manual editing of size and position of the window.

`LCTRL + LWIN + I` hotkey is used to hide/show trayicon.
You can also use commandline option `/hide` to hide trayicon at startup.

The `/noupdate` commandline option is used to disable check for updates. By default, the program checks for updates upon startup and once a day.

The `Use workarea` option (at popup menu) means that the window is centered without a taskbar, otherwise, the full resolution of the monitor will be used.

If some windows does not centers, you should run wCenterWindow with administrative rights.

## Automatic startup

Usually, to start the application when Windows starts, it is enough to put the application's shortcut in the "Startup" folder - 
"C:\Users\\<*Your user name*>\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup". 
(The easiest way to get there - press 'WIN + R' and type `shell:startup`).<br>
However, in this case wCenterWindow will not be able to work with windows that are open with elevated privileges.
And if you enable the "Run as administrator" option in the shortcut, then wCenterWindow will not start. This is related to the security of Windows (maybe only in 10/11, I did not check).
This behavior can be bypassed by creating a task in the Task Scheduler with the "Run with highest privileges" option.<br>
**Note:** If you run wCenterWindow via the Task Scheduler, I highly recommend enabling the "Delay task for" option for 15-30 seconds, otherwise you may get an error message "Can't create tray icon".

## License

This software is licensed under a [MIT License](https://mit-license.org).<br>
This software uses the [PicoJSON](https://github.com/kazuho/picojson) - a C++ JSON parser / serializer.
