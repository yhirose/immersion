#include <locale.h>
#include <ncurses.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "utf8.h"

using namespace std;

#define LINES_ ((size_t)LINES)
#define COLS_ ((size_t)COLS)

template <typename T, typename U>
vector<T> filter(const vector<T>& vec, U fn) {
  vector<string> out;
  copy_if(vec.begin(), vec.end(), back_inserter(out),
          [fn](const T& val) { return fn(val); });
  return out;
}

size_t columns(const string& line) {
  size_t cols = 0;
  size_t pos = 0;
  while (pos < line.size()) {
    size_t col_len = 0;
    pos += utf8CharLen(line.data(), line.size(), pos, &col_len);
    cols += col_len;
  }
  return cols;
}

vector<string> fold(const string& line, size_t cols) {
  vector<string> lines;

  if (line.empty()) {
    lines.push_back(string());
    return lines;
  }

  size_t pos = 0;
  size_t start = 0;
  size_t col = 0;

  auto init = true;
  while (pos < line.size()) {
    size_t col_len = 0;
    auto char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);
    if (col + col_len > cols) {
      lines.push_back(line.substr(start, pos - start));
      start = pos;
      col = 0;
    }
    pos += char_len;
    col += col_len;
  }

  if (start < pos) {
    lines.push_back(line.substr(start, pos - start));
  }

  return lines;
}

vector<string> fold_lines(const vector<string>& lines, size_t cols,
                          bool linespace) {
  auto filtered =
      filter(lines, [&](auto& line) { return !linespace || !line.empty(); });

  vector<string> out;
  auto init = true;
  for (auto line : filtered) {
    if (init) {
      init = false;
    } else if (linespace) {
      out.push_back(string());
    }
    for (auto l : fold(line, cols)) {
      out.push_back(l);
    }
  }
  return out;
}

vector<string> read_lines(istream& in) {
  vector<string> lines;
  string line;
  while (getline(in, line)) {
    lines.push_back(line);
  }
  return lines;
}

size_t max_colums(const vector<string>& lines) {
  size_t cols = 0;
  for (auto& line : lines) {
    cols = max(cols, columns(line));
  }
  return cols;
}

void draw(const vector<string>& lines, size_t cols, size_t current_line,
          size_t margin) {
  auto view_lines = LINES_ - margin * 2;

  if (lines.size() < view_lines) {
    int y = LINES_ / 2 - lines.size() / 2;
    size_t i = 0;
    for (auto& line : lines) {
      int x = COLS_ / 2 - cols / 2;
      mvprintw(y + i, x, line.c_str());
      i++;
    }
  } else {
    auto y = margin;
    for (size_t i = 0; i < view_lines; i++) {
      auto& line = lines[current_line + i];
      int x = COLS_ / 2 - cols / 2;
      mvprintw(y + i, x, line.c_str());
    }
  }
}

size_t calc_margin(size_t height, size_t min_margin, size_t line_count) {
  return max((LINES_ - min(height, line_count)) / 2, min_margin);
}

void parse_command_line(int argc, char* const* argv, size_t& cols,
                        size_t& height, size_t& min_margin, bool& linespace) {
  int opt;
  opterr = 0;
  while ((opt = getopt(argc, argv, "h:m:sw:")) != -1) {
    switch (opt) {
      case 'h':
        height = stoi(optarg);
        break;
      case 'm':
        min_margin = stoi(optarg);
        break;
      case 's':
        linespace = true;
        break;
      case 'w':
        cols = stoi(optarg);
        break;
    }
  }
}

int main(int argc, char* const* argv) {
  size_t opt_cols = 0;
  size_t opt_height = 0;
  size_t opt_min_margin = 1;
  bool opt_linespace = false;
  parse_command_line(argc, argv, opt_cols, opt_height, opt_min_margin,
                     opt_linespace);
  argc -= optind;
  argv += optind;

  vector<string> lines;
  if (!isatty(0)) {
    lines = read_lines(cin);
    freopen("/dev/tty", "rw", stdin);
  } else {
    if (argc > 0) {
      auto path = argv[0];
      ifstream ifs(path);
      if (ifs) {
        lines = read_lines(ifs);
      } else {
        cerr << "failed to open " << path << " file..." << endl;
        return 0;
      }
    } else {
      lines = {
          "Usage: immersion [-h rows] [-m rows] [-s] [-w cols] [path]",
          "",
          "    q                   quit",
          "",
          "    s                   toggle line space",
          "    [                   larger",
          "    ]                   smaller",
          "",
          "    j                   line down",
          "    k                   line up",
          "    f or = or [space]   page down",
          "    b or -              page up",
          "    d or J              half page down",
          "    u or K              half page up",
          "    g                   go to top",
          "    G                   go to bottom",
      };
      opt_cols = 0;
      opt_height = 0;
    }
  }

  setlocale(LC_CTYPE, "");
  initscr();
  noecho();
  curs_set(0);

  auto min_margin = opt_min_margin;
  auto linespace = opt_linespace;
  auto max_cols = max_colums(lines);
  auto cols = opt_cols > 0 ? opt_cols : min(max_cols, COLS_ - min_margin * 2);

  auto height = opt_height > 0 ? opt_height : LINES_ - min_margin * 2;
  auto display_lines = fold_lines(lines, cols, linespace);
  auto display_cols = min(max_cols, min(cols, COLS_));
  auto margin = calc_margin(height, min_margin, display_lines.size());
  height = LINES_ - margin * 2; // adjust based on actual margin

  int current_line = 0;
  draw(display_lines, display_cols, current_line, margin);

  while (true) {
    int key = getch();
    if (key == 'q') break;

    auto page_lines = display_lines.size();
    auto bottom_line = 0;
    if (display_lines.size() > height) {
      page_lines = min(display_lines.size(), height);
      bottom_line = display_lines.size() - page_lines;
    }

    auto scroll_core = [&](int rows, bool forward) {
      while (rows > 0) {
        current_line += forward ? 1 : -1;
        draw(display_lines, display_cols, current_line, margin);
        refresh();
        rows--;
      }
    };

    auto scroll_forward = [&](size_t rows) {
      rows = (current_line + rows > bottom_line) ? bottom_line - current_line
                                                 : rows;
      scroll_core(rows, true);
    };

    auto scroll_backword = [&](size_t rows) {
      rows = (current_line - (int)rows < 0) ? current_line : rows;
      scroll_core(rows, false);
    };

    auto layout = false;

    switch (key) {
      case 's':
        linespace = !linespace;
        layout = true;
        break;

      case '[':
        if (cols < COLS_ - min_margin * 2) {
          cols++;
          layout = true;
        }
        break;

      case ']':
        if (cols > min_margin * 2) {
          cols--;
          layout = true;
        }
        break;

      case 'j':
        if (current_line < bottom_line) {
          current_line++;
        }
        break;

      case 'k':
        if (current_line > 0) {
          current_line--;
        }
        break;

      case 'g':
        scroll_backword(current_line);
        break;

      case 'G':
        scroll_forward(bottom_line - current_line);
        break;

      case 'f':
      case '=':
      case ' ':
        scroll_forward(page_lines);
        break;

      case 'b':
      case '-':
        scroll_backword(page_lines);
        break;

      case 'd':
      case 'J':
        scroll_forward(page_lines / 2);
        break;

      case 'u':
      case 'K':
        scroll_backword(page_lines / 2);
        break;

      case KEY_RESIZE:
        layout = true;
        break;
    }

    if (layout) {
      height = opt_height > 0 ? opt_height : LINES_ - min_margin * 2;
      display_lines = fold_lines(lines, cols, linespace);
      display_cols = min(max_cols, min(cols, COLS_));
      margin = calc_margin(height, min_margin, display_lines.size());
      height = LINES_ - margin * 2; // adjust based on actual margin
      current_line = 0;
    }

    clear();
    draw(display_lines, display_cols, current_line, margin);
    refresh();
  }

  endwin();

  return 0;
}
