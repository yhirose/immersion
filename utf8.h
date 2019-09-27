#ifndef UTF8_H
#define UTF8_H

size_t utf8CharLen(const char* buf, size_t buf_len, size_t pos,
                   size_t* col_len);

#endif /* UTF8_H */

