Immersion
=========

Distraction free pager on Terminal.

```
cal 2020 | immersion
figlet Hello World! | immersion
pbpaste | immersion
immersion -w 80 -w 40 main.cpp
```

Usage
-----

```
usage: immersion [-ls] [-h rows] [-w cols] [-m rows] [file]

  options:
    -l                  line space
    -s                  word wrap
    -h rows             window height
    -w cols             window width
    -m cols             minimun margin
    file                file path

  commands:
    q                   quit
    s                   toggle line space
    r                   toggle word wrap
    i                   widen window
    o                   narrow window
    I                   make window taller
    O                   make window shorter
    j                   line down
    k                   line up
    f or = or [space]   page down
    b or -              page up
    d or J              half page down
    u or K              half page up
    g                   go to top
    G                   go to bottom
```

Build
-----

```
make
```

License
-------

MIT license (© 2019 Yuji Hirose)
