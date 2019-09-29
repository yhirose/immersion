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
Usage: immersion [-h rows] [-m rows] [-s] [-r] [-w cols] [path]

    q                   quit

    s                   toggle line space
    r                   toggle word wrap
    [                   larger
    [                   smaller

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
