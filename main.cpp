#include <locale.h>
#include <ncurses.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "utf8.h"

using namespace std;

#define ROWS_ ((size_t)LINES)
#define COLS_ ((size_t)COLS)

template <typename T, typename U>
vector<T> filter(const vector<T>& vec, U fn) {
  vector<T> out;
  copy_if(vec.begin(), vec.end(), back_inserter(out),
          [fn](const T& val) { return fn(val); });
  return out;
}

auto read_lines(istream& in) {
  vector<string> lines;
  string line;
  while (getline(in, line)) {
    lines.push_back(line);
  }
  return lines;
}

size_t columns(const string& line) {
  size_t cols = 0;
  size_t pos = 0;
  while (pos < line.size()) {
    size_t col_len = 0;
    auto char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);

    if (line[pos] == 0x1b) {
      pos += char_len;

      while (pos < line.size()) {
        size_t col_len2 = 0;
        auto char_len2 = utf8CharLen(line.data(), line.size(), pos, &col_len2);
        auto ch2 = line.substr(pos, char_len2);

        pos += char_len2;

        if (ch2[0] == 'm') {
          char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);
          pos += char_len;
          break;
        }
      }
      char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);
    } else if (pos + char_len < line.size() && line[pos + char_len] == 0x08) {
      pos += char_len + 1;
      char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);
    } else {
      pos += char_len;
    }

    cols += col_len;
  }
  return cols;
}

bool is_invalid_start_char(const string& ch) {
  return ch == u8"." || ch == u8"," || ch == u8";" || ch == u8"?" ||
         ch == u8"!" || ch == u8"。" || ch == u8"，" || ch == u8"？" ||
         ch == u8"！" || ch == u8"･";
}

auto fold_line(const string& line, size_t cols, bool word_warp) {
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

    // Skip attribute
    if (line[pos] == 0x1b) {
      pos += char_len;

      while (pos < line.size()) {
        size_t col_len2 = 0;
        auto char_len2 = utf8CharLen(line.data(), line.size(), pos, &col_len2);
        auto ch2 = line.substr(pos, char_len2);

        pos += char_len2;

        if (ch2[0] == 'm') {
          char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);
          break;
        }
      }
      char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);
    } else if (pos + char_len < line.size() && line[pos + char_len] == 0x08) {
      pos += char_len + 1;
      char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);
    }

    if (col + col_len <= cols) {
      col += col_len;
      pos += char_len;
    } else {
      auto ch = line.substr(pos, char_len);
      if (is_invalid_start_char(ch)) {
        pos += char_len;
        lines.push_back(line.substr(start, pos - start));
        while (pos < line.size()) {
          if (line[pos] != ' ') {
            break;
          }
          pos++;
        }
        start = pos;
        col = 0;
      } else {
        if (ch == u8" ") {
          lines.push_back(line.substr(start, pos - start));
          pos += char_len;
          start = pos;
          col = 0;
        } else if (word_warp) {
          auto pos2 = pos - 1;
          while (start <= pos2) {
            if (line[pos2] == ' ') {
              break;
            }
            pos2--;
          }
          if (pos2 < start) {
            lines.push_back(line.substr(start, pos - start));
            start = pos;
            col = col_len;
            pos += char_len;
          } else {
            lines.push_back(line.substr(start, pos2 - start));
            start = pos2 + 1;
            col = 0;
            pos = pos2 + 1;
          }
        } else {
          lines.push_back(line.substr(start, pos - start));
          start = pos;
          col = col_len;
          pos += char_len;
        }
      }
    }
  }

  if (start < pos) {
    lines.push_back(line.substr(start, pos - start));
  }

  return lines;
}

template <typename Out>
void split(const std::string& s, char delim, Out result) {
  std::istringstream iss(s);
  std::string item;
  while (std::getline(iss, item, delim)) {
    *result++ = std::stoi(item);
  }
}

std::vector<int> split(const std::string& s, char delim) {
  std::vector<int> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

auto to_attributed_line(const string& line) {
  vector<pair<string, chtype>> result;

  chtype type = A_NORMAL;
  size_t pos = 0;
  while (pos < line.size()) {
    size_t col_len = 0;
    auto char_len = utf8CharLen(line.data(), line.size(), pos, &col_len);
    auto ch = line.substr(pos, char_len);

    if (ch[0] == 0x1b) {
      auto esc_start_pos = pos;
      pos += char_len;

      while (pos < line.size()) {
        size_t col_len2 = 0;
        auto char_len2 = utf8CharLen(line.data(), line.size(), pos, &col_len2);
        auto ch2 = line.substr(pos, char_len2);

        pos += char_len2;

        if (ch2[0] == 'm') {
          type = A_NORMAL;
          esc_start_pos += 2;
          auto esc_len = pos - esc_start_pos - 1;
          if (esc_len > 0) {
            auto esc_values = line.substr(esc_start_pos, esc_len);
            auto vals = split(esc_values, ';');
            for (auto val : vals) {
              switch (val) {
                case 1:
                  type |= A_BOLD;
                  break;
                case 4:
                  type |= A_UNDERLINE;
                  break;
                case 30:
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                case 36:
                case 37:
                  type |= COLOR_PAIR(val);
                  break;
              }
            }
          }
          break;
        }
      }
    } else if (pos + char_len < line.size() && line[pos + char_len] == 0x08) {
      pos += char_len + 1;

      size_t col_len2 = 0;
      auto char_len2 = utf8CharLen(line.data(), line.size(), pos, &col_len2);
      auto ch2 = line.substr(pos, char_len2);

      pos += char_len2;
      if (ch == "_") {
        result.emplace_back(ch2, A_UNDERLINE);
      }
      if (ch == ch2) {
        result.emplace_back(ch2, A_BOLD);
      }
    } else {
      pos += char_len;
      result.emplace_back(ch, type);
    }
  }

  return result;
}

size_t max_colums(const vector<string>& lines) {
  size_t cols = 0;
  for (auto& line : lines) {
    cols = max(cols, columns(line));
  }
  return cols;
}

auto fold_lines(const vector<string>& lines, size_t cols, bool linespace,
                bool word_warp) {
  auto filtered =
      filter(lines, [&](auto& line) { return !linespace || !line.empty(); });

  vector<vector<pair<string, chtype>>> out;
  auto init = true;
  for (auto line : filtered) {
    if (init) {
      init = false;
    } else if (linespace) {
      out.push_back(to_attributed_line(string()));
    }
    for (auto l : fold_line(line, cols, word_warp)) {
      out.push_back(to_attributed_line(l));
    }
  }
  return out;
}

void draw(const vector<vector<pair<string, chtype>>>& lines, size_t cols,
          size_t current_line, size_t margin) {
  auto view_lines = ROWS_ - margin * 2;
  if (lines.size() < view_lines) {
    int y = ROWS_ / 2 - lines.size() / 2;
    size_t i = 0;
    for (auto& line : lines) {
      int x = COLS_ / 2 - cols / 2;
      move(y + i, x);
      for (auto& [ch, att] : line) {
        attron(att);
        printw(ch.c_str());
        attroff(att);
      }
      i++;
    }
  } else {
    auto y = margin;
    for (size_t i = 0; i < view_lines; i++) {
      auto& line = lines[current_line + i];
      int x = COLS_ / 2 - cols / 2;
      move(y + i, x);
      for (auto& [ch, att] : line) {
        attron(att);
        printw(ch.c_str());
        attroff(att);
      }
    }
  }
}

size_t calc_margin(size_t rows, size_t min_margin, size_t line_count) {
  rows = rows > 0 ? rows : ROWS_ - min_margin * 2;
  return max((ROWS_ - min(rows, line_count)) / 2, min_margin);
}

size_t calc_page_lines(size_t display_lines, size_t rows, size_t& bottom_line) {
  auto page_lines = display_lines;
  bottom_line = 0;
  if (display_lines > rows) {
    page_lines = min(display_lines, rows);
    bottom_line = display_lines - page_lines;
  }
  return page_lines;
}

void parse_command_line(int argc, char* const* argv, size_t& cols, size_t& rows,
                        size_t& min_margin, bool& linespace, bool& word_warp) {
  int opt;
  opterr = 0;
  while ((opt = getopt(argc, argv, "r:c:m:sw")) != -1) {
    switch (opt) {
      case 'r':
        rows = stoi(optarg);
        break;
      case 'c':
        cols = stoi(optarg);
        break;
      case 'm':
        min_margin = stoi(optarg);
        break;
      case 's':
        linespace = true;
        break;
      case 'w':
        word_warp = true;
        break;
    }
  }
}

int main(int argc, char* const* argv) {
  size_t opt_cols = 0;
  size_t opt_rows = 0;
  size_t opt_min_margin = 2;
  bool opt_linespace = false;
  bool opt_word_wrap = false;

  parse_command_line(argc, argv, opt_cols, opt_rows, opt_min_margin,
                     opt_linespace, opt_word_wrap);
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
        return -1;
      }
    } else {
      opt_cols = 0;
      opt_rows = 0;
      lines = {
          "usage: immersion [-sw] [-r rows] [-c cols] [-m margin] [file]",
          "",
          "  options:",
          "    -s                  line space",
          "    -w                  word wrap",
          "    -r rows             window height",
          "    -c cols             window width",
          "    -m margin           minimun margin",
          "    file                file path",
          "",
          "  commands:",
          "    q              quit",
          "    s              toggle line space",
          "    i              widen window",
          "    o              narrow window",
          "    I              make window taller",
          "    O              make window shorter",
          "    j              line down",
          "    k              line up",
          "    f or [space]   page down",
          "    b              page up",
          "    d              half page down",
          "    u              half page up",
          "    g              go to top",
          "    G              go to bottom",
      };
    }
  }

  setlocale(LC_CTYPE, "");

  initscr();
  noecho();
  curs_set(0);

  use_default_colors();
  start_color();
  init_pair(30, COLOR_BLACK, -1);
  init_pair(31, COLOR_RED, -1);
  init_pair(32, COLOR_GREEN, -1);
  init_pair(33, COLOR_YELLOW, -1);
  init_pair(34, COLOR_BLUE, -1);
  init_pair(35, COLOR_MAGENTA, -1);
  init_pair(36, COLOR_CYAN, -1);
  init_pair(37, COLOR_WHITE, -1);

  auto linespace = opt_linespace;
  auto max_cols = max_colums(lines);
  auto cols =
      opt_cols > 0 ? opt_cols : min(max_cols, COLS_ - opt_min_margin * 2);
  auto rows = opt_rows;

  auto display_lines = fold_lines(lines, cols, linespace, opt_word_wrap);
  auto display_cols = min(max_cols, min(cols, COLS_));
  auto margin = calc_margin(rows, opt_min_margin, display_lines.size());
  rows = ROWS_ - margin * 2;  // adjust based on actual margin
  size_t bottom_line = 0;
  auto page_lines = calc_page_lines(display_lines.size(), rows, bottom_line);
  int current_line = 0;

  draw(display_lines, display_cols, current_line, margin);

  while (true) {
    int key = getch();
    if (key == 'q') break;

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

      case 'i':
        if (cols < COLS_ - opt_min_margin * 2) {
          cols++;
          layout = true;
        }
        break;

      case 'o':
        if (cols > opt_min_margin * 2) {
          cols -= 2;
          layout = true;
        }
        break;

      case 'I':
        if (rows < ROWS_ - opt_min_margin * 2) {
          rows += 2;
          layout = true;
        }
        break;

      case 'O':
        if (rows > opt_min_margin * 2) {
          rows -= 2;
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
        current_line = 0;
        layout = true;
        break;

      case 'G':
        current_line = bottom_line;
        layout = true;
        break;

      case 'f':
      case ' ':
        scroll_forward(page_lines);
        break;

      case 'b':
        scroll_backword(page_lines);
        break;

      case 'd':
        scroll_forward(page_lines / 2);
        break;

      case 'u':
        scroll_backword(page_lines / 2);
        break;

      case KEY_RESIZE:
        layout = true;
        break;
    }

    if (layout) {
      display_lines = fold_lines(lines, cols, linespace, opt_word_wrap);
      display_cols = min(max_cols, min(cols, COLS_));
      margin = calc_margin(rows, opt_min_margin, display_lines.size());
      rows = ROWS_ - margin * 2;  // adjust based on actual margin
      page_lines = calc_page_lines(display_lines.size(), rows, bottom_line);
      if (current_line > bottom_line) {
        current_line = bottom_line;
      }
    }

    erase();
    draw(display_lines, display_cols, current_line, margin);
    refresh();
  }

  endwin();

  return 0;
}
