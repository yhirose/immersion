#!/usr/bin/env python3
import argparse
import curses
import locale
import os
import signal
import sys
import unicodedata

def columns(line):
    cols = 0;
    for c in list(line):
        w = unicodedata.east_asian_width(c)
        if c == 'A' or c == 'F' or c == 'W':
            cols += 2
        else:
            cols += 1
    return cols

def max_colums(lines):
    cols = 0
    for line in lines:
        cols = max(cols, columns(line))
    return cols

def calc_margin(rows, min_margin, line_count, ROWS):
    rows = rows if rows > 0 else ROWS - min_margin * 2
    return max(int((ROWS - min(rows, line_count)) / 2), min_margin)

def calc_page_lines(display_lines, rows):
    page_lines = display_lines
    bottom_line = 0
    if display_lines > rows:
        page_lines = min(display_lines, rows)
        bottom_line = display_lines - page_lines
    return page_lines, bottom_line

def column_len(c):
    w = unicodedata.east_asian_width(c)
    return 2 if w == 'W' or w == 'F' or w == 'A' else 1

def is_invalid_start_char(c):
    return c == '.' or c == ',' or c == ';' or c == '?' or c == '!' or c == '。' or c == '，' or c == '？' or c == '！' or c == '･'

def fold_line(line, cols, word_warp):
    lines = []

    if len(line) == 0:
        lines.append('')
        return lines

    pos = 0
    start = 0
    col = 0

    init = True
    while pos < len(line):
        c = line[pos]
        col_len = column_len(c)
        if pos + 1 < len(line) and ord(line[pos + 1]) == 0x08:
            pos += 2
            c = line[pos]
            col_len = column_len(c)

        if col + col_len <= cols:
            col += col_len;
            pos += 1
        else:
            if is_invalid_start_char(c):
                pos += 1
                lines.append(line[start:pos])
                while pos < len(line):
                    if line[pos] != ' ':
                        break;
                    pos += 1
                start = pos
                col = 0
            else:
                if c == ' ':
                    lines.append(line[start:pos])
                    pos += 1
                    start = pos
                    col = 0
                elif word_warp:
                    pos2 = pos - 1;
                    while start <= pos2:
                        if line[pos2] == ' ':
                            break
                        pos2 -= 1
                    if pos2 < start:
                        lines.append(line[start:pos])
                        start = pos
                        col = col_len
                        pos += 1
                    else:
                        lines.append(line[start:pos2])
                        start = pos2 + 1
                        col = 0
                        pos = pos2 + 1
                else:
                    lines.append(line[start:pos])
                    start = pos
                    col = col_len
                    pos += 1

    if start < pos:
        lines.append(line[start: pos])

    return lines

def to_attributed_line(line):
    result = []
    pos = 0;
    while pos < len(line):
        c = line[pos]
        if pos + 1 < len(line) and ord(line[pos + 1]) == 0x08:
             pos += 2;
             c2 = line[pos]
             pos += 1;
             if c == '_':
                 result.append((c2, curses.A_UNDERLINE));
             if c == c2:
                 result.append((c2, curses.A_BOLD));
        else:
            pos += 1;
            result.append((c, curses.A_NORMAL));

    return result

def fold_lines(lines, cols, linespace, word_warp):
    filtered = [line for line in lines if linespace == False or len(lines) > 0]
    out = []
    init = True
    for line in filtered:
        if init:
            init = False
        elif linespace:
            out.append(to_attributed_line(''))
        for l in fold_line(line, cols, word_warp):
            out.append(to_attributed_line(l))
    return out

def draw(scr, ROWS, COLS, lines, cols, current_line, margin):
    view_lines = ROWS - margin * 2
    if len(lines) < view_lines:
        y = int(ROWS / 2) - int(len(lines) / 2)
        i = 0;
        for line in lines:
            x = int(COLS / 2) - int(cols / 2)
            scr.move(y + i, x)
            for c, att in line:
                scr.addstr(c, att)
            i += 1
    else:
        y = margin
        for i in range(view_lines):
            line = lines[current_line + i];
            x = int(COLS / 2) - int(cols / 2)
            scr.move(y + i, x)
            for c, att in line:
                scr.addstr(c, att)

def pager(stdscr, lines, opt_cols, opt_rows, opt_min_margin, opt_linespace, opt_word_wrap):
    signal.signal(signal.SIGINT, lambda a, b: None)

    locale.setlocale(locale.LC_ALL, '')
    os.environ['TERM'] = 'xterm-256color'

    stdscr = curses.initscr()
    curses.noecho()
    curses.curs_set(0);
    curses.use_default_colors()

    ROWS, COLS = stdscr.getmaxyx()

    linespace = opt_linespace
    max_cols = max_colums(lines)
    cols = opt_cols if opt_cols > 0 else min(max_cols, int(COLS - opt_min_margin * 2))
    rows = opt_rows

    display_lines = fold_lines(lines, cols, linespace, opt_word_wrap)
    display_cols = min(max_cols, min(cols, COLS))
    margin = calc_margin(rows, opt_min_margin, len(display_lines), ROWS)
    rows = ROWS - margin * 2  # adjust based on actual margin
    page_lines, bottom_line = calc_page_lines(len(display_lines), rows)
    current_line = 0

    draw(stdscr, ROWS, COLS, display_lines, display_cols, current_line, margin)

    while True:
        key = stdscr.getch()
        if key == ord('q'):
            break

        layout = False;

        if key == ord('s'):
            linespace = False if linespace else True
            layout = True
        elif key == ord('i'):
            if cols < COLS - opt_min_margin * 2:
                cols += 1
                layout = True
        elif key == ord('o'):
            if cols > opt_min_margin * 2:
                cols -= 2
                layout = True
        elif key == ord('I'):
            if rows < ROWS - opt_min_margin * 2:
                rows += 2
                layout = True
        elif key == ord('O'):
            if rows > opt_min_margin * 2:
                rows -= 2
                layout = True
        elif key == ord('j'):
            if current_line < bottom_line:
                current_line += 1
        elif key == ord('k'):
            if current_line > 0:
                current_line -= 1
        elif key == curses.KEY_RESIZE:
            ROWS, COLS = stdscr.getmaxyx()
            layout = True

        if layout:
            display_lines = fold_lines(lines, cols, linespace, opt_word_wrap)
            display_cols = min(max_cols, min(cols, COLS))
            margin = calc_margin(rows, opt_min_margin, len(display_lines), ROWS)
            rows = ROWS - margin * 2  # adjust based on actual margin
            page_lines, bottom_line = calc_page_lines(len(display_lines), rows)
            if current_line > bottom_line:
                current_line = bottom_line

        stdscr.clear()
        draw(stdscr, ROWS, COLS, display_lines, display_cols, current_line, margin)
        stdscr.refresh()

    curses.endwin()

def run_pager(lines, opt_cols, opt_rows, opt_min_margin, opt_linespace, opt_word_wrap):
    curses.wrapper(pager, lines, opt_cols, opt_rows, opt_min_margin, opt_linespace, opt_word_wrap)

#------------------------------------------------------------------------------

def command_usange():
    return '''\
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
'''.split('\n')

#------------------------------------------------------------------------------

parser = argparse.ArgumentParser(prog='immersion')
parser.add_argument('file', nargs='?', help='file path')
parser.add_argument('-c', type=int, default=0, metavar='cols', help='columns')
parser.add_argument('-r', type=int, default=0, metavar='rows', help='rows')
parser.add_argument('-m', type=int, default=2, metavar='margin', help='minimun margin')
parser.add_argument('-s', action='store_true', help='show line space')
parser.add_argument('-w', action='store_true', help='word wrap')

args = parser.parse_args()

opt_cols = args.c
opt_rows = args.r
opt_min_margin = args.m
opt_linespace = args.s
opt_word_wrap = args.w

lines = []
if not sys.stdin.isatty():
    lines = sys.stdin.readlines()
    f = open('/dev/tty', 'r')
    os.dup2(f.fileno(), 0)
else:
    if args.file:
        try:
            with open(args.file, 'r') as f:
                lines = f.readlines()
        except IOError as e:
            print('failed to open {} file...'.format(args.file))
            exit(-1)
    else:
        opt_cols = 0
        opt_rows = 0
        lines = command_usange()

run_pager(lines, opt_cols, opt_rows, opt_min_margin, opt_linespace, opt_word_wrap)
