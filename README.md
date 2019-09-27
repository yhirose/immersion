immersion
=========

Distraction free pager on Terminal

```
cal | immersion
cal 2020 | immersion -w 0 -h 0
immersion -w 80 -w 40 main.cpp
```

Usage
-----

```
Usage: immersion [-h rows] [-m rows] [-s] [-w cols] [path]

    q             quit

    s             toggle line space"
    =             larger
    -             smaller

    f / [space]   page down
    b             page up
    u             half page up
    d             half page down
    g             go to top
    G             go to bottom
```

Build
-----

```
make
```

License
-------

MIT license (© 2019 Yuji Hirose)
