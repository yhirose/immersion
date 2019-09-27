#include <locale.h>
#include <ncurses.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "utf8.h"

size_t columns(const std::string& line) {
  size_t cols = 0;
  size_t pos = 0;
  while (pos < line.size()) {
    size_t col_len = 0;
    pos += utf8CharLen(line.data(), line.size(), pos, &col_len);
    cols += col_len;
  }
  return cols;
}

std::vector<std::string> fold(const std::string& line, size_t cols) {
  std::vector<std::string> lines;

  if (line.empty()) {
    lines.push_back(std::string());
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

std::vector<std::string> fold_lines(const std::vector<std::string>& lines,
                                    size_t cols, bool linespace) {
  std::vector<std::string> out;
  auto init = true;
  for (auto line : lines) {
    if (init) {
      init = false;
    } else if (linespace) {
      out.push_back(std::string());
    }
    for (auto l : fold(line, cols)) {
      out.push_back(l);
    }
  }
  return out;
}

std::vector<std::string> read_lines(std::istream& in) {
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line)) {
    lines.push_back(line);
  }
  return lines;
}

size_t max_colums(const std::vector<std::string>& lines) {
  size_t cols = 0;
  for (auto& line : lines) {
    cols = std::max(cols, columns(line));
  }
  return cols;
}

void draw(const std::vector<std::string>& lines, size_t cols,
          size_t current_line, size_t margin) {
  auto display_line_count = LINES - margin * 2;

  if (lines.size() < display_line_count) {
    int y = LINES / 2 - lines.size() / 2;
    size_t i = 0;
    for (auto& line : lines) {
      int x = COLS / 2 - cols / 2;
      mvprintw(y + i, x, line.c_str());
      i++;
    }
  } else {
    auto y = margin;
    auto count = 0;
    for (size_t i = 0; i < display_line_count; i++) {
      auto& line = lines[current_line + i];
      int x = COLS / 2 - cols / 2;
      mvprintw(y + i, x, line.c_str());
    }
  }
}

size_t calc_margin(size_t height, size_t min_margin) {
  return std::max((LINES - std::min<size_t>(LINES, height)) / 2, min_margin);
}

void parse_command_line(int argc, char* const* argv, size_t& cols,
                        size_t& height, size_t& min_margin, bool& linespace) {
  int opt;
  opterr = 0;
  while ((opt = getopt(argc, argv, "h:m:sw:")) != -1) {
    switch (opt) {
      case 'h':
        height = std::stoi(optarg);
        break;
      case 'm':
        min_margin = std::stoi(optarg);
        break;
      case 's':
        linespace = true;
        break;
      case 'w':
        cols = std::stoi(optarg);
        break;
    }
  }
}

int main(int argc, char* const* argv) {
  size_t cols = 40;
  size_t height = 24;
  size_t min_margin = 1;
  bool linespace = false;
  parse_command_line(argc, argv, cols, height, min_margin, linespace);
  argc -= optind;
  argv += optind;

  std::vector<std::string> lines;
  if (!isatty(0)) {
    lines = read_lines(std::cin);
    freopen("/dev/tty", "rw", stdin);
  } else {
    if (argc > 0) {
      auto path = argv[0];
      std::ifstream ifs(path);
      if (ifs) {
        lines = read_lines(ifs);
      } else {
        std::cerr << "failed to open "<< path << " file..." << std::endl;
        return 0;
      }
    } else {
      lines = {
          "Usage: immersion [-h rows] [-m rows] [-s] [-w cols] [path]",
          "",
          "    q             quit",
          "",
          "    s             toggle line space",
          "    =             larger",
          "    -             smaller",
          "",
          "    f / [space]   page down",
          "    b             page up",
          "    u             half page up",
          "    d             half page down",
          "    g             go to top",
          "    G             go to bottom",
      };
      cols = 0;
      height = 0;
    }
  }

  setlocale(LC_CTYPE, "");
  initscr();
  noecho();
  curs_set(0);

  auto max_cols = max_colums(lines);

  if (cols == 0) {
    cols = max_cols;
  }
  if (height == 0) {
    height = LINES;
  }

  int current_line = 0;

  auto display_lines = fold_lines(lines, cols, linespace);
  auto margin = calc_margin(height, min_margin);
  auto display_cols = std::min(max_cols, std::min<size_t>(cols, COLS));
  draw(display_lines, display_cols, current_line, margin);

  while (true) {
    int key = getch();
    if (key == 'q') break;

    auto rows = (LINES - margin * 2);
    auto bottom_line = display_lines.size() - 1 - rows;

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

    if (key == 's') {
      linespace = !linespace;
      layout = true;
    } else if (key == '=') {
      if (cols < COLS) {
        cols++;
        layout = true;
      }
    } else if (key == '-') {
      if (cols > 4) {
        cols--;
        layout = true;
      }
    } else if (key == 'j') {
      if (current_line < bottom_line) {
        current_line++;
      }
    } else if (key == 'k') {
      if (current_line > 0) {
        current_line--;
      }
    } else if (key == 'g') {
      scroll_backword(current_line);
    } else if (key == 'G') {
      scroll_forward(bottom_line - current_line);
    } else if (key == 'f' || key == ' ') {
      scroll_forward(rows);
    } else if (key == 'b') {
      scroll_backword(rows);
    } else if (key == 'd') {
      scroll_forward(rows / 2);
    } else if (key == 'u') {
      scroll_backword(rows / 2);
    } else if (key == KEY_RESIZE) {
      margin = calc_margin(height, min_margin);
      layout = true;
    }

    if (layout) {
      display_lines = fold_lines(lines, cols, linespace);
      display_cols = std::min(max_cols, std::min<size_t>(cols, COLS));
    }

    clear();
    draw(display_lines, display_cols, current_line, margin);
    refresh();
  }

  endwin();

  return 0;
}
