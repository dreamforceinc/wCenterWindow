## wCenterWindow

This program centers the current active window by a `LCTRL + LWIN + C` hotkey,
or pressing `LCTRL + LWIN + MMB` (Middle Mouse Button).

`LCTRL + LWIN + V` - manual editing of size and position of the window.

You can use `LCTRL + LWIN + I` hotkey for hide/show trayicon.
You can also use commandline option `/hide` for hide trayicon at startup.

The `/noupdate` option is used to disable check for updates. By default, the check is repeated once a day.

`Use workarea` option means that the window is centered without a taskbar, otherwise, the full resolution of the monitor will be used.

If some windows does not centers you should run wCenterWindow with administrative rights.

## Automatic startup

Usually, to start the application when Windows starts, it is enough to put the application's shortcut in the "Startup" folder -\
"C:\Users\\<*Your user name*>\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup". (The easiest way to get there - press 'WIN + R' and type 'shell:startup').\
However, in this case wCenterWindow will not be able to work with windows that are open with elevated privileges.
And if you enable the "Run as administrator" option in the shortcut, then wCenterWindow will not start. This is related to the security of Windows (maybe only in 10/11, I did not check).\
This behavior can be bypassed by creating a task in the Task Scheduler with the "Run with highest privileges" option.\
**Note:** If you run wCenterWindow via the Task Scheduler, I highly recommend enabling the "Delay task for" option for 15-30 seconds, otherwise you may get an error message "Can't create tray icon".

## License

MIT License

Copyright (c) 2023 W0LF aka 'dreamforce'

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
