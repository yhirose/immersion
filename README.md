Immersion
=========

Distraction free pager on Terminal.

```
cal 2020 | immersion
figlet Hello World! | immersion
pbpaste | immersion
immersion -c 80 -r 40 main.cpp
```

Usage
-----

```
usage: immersion [-sw] [-r rows] [-c cols] [-m margin] [file]

  options:
    -s                  line space
    -w                  word wrap
    -r rows             window height
    -c cols             window width
    -m margin           minimun margin
    file                file path

  commands:
    q                   quit
    s                   toggle line space
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
