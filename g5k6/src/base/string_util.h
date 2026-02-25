#ifndef BASE_STRING_UTIL_H_
#define BASE_STRING_UTIL_H_

#include <cstdint>
#include <string>
#include <vector>

#define WHITESPACE_ASCII_NO_CR_LF      \
  0x09,     /* CHARACTER TABULATION */ \
      0x0B, /* LINE TABULATION */      \
      0x0C, /* FORM FEED (FF) */       \
      0x20  /* SPACE */

#define WHITESPACE_ASCII                                                  \
  WHITESPACE_ASCII_NO_CR_LF, /* Comment to make clang-format linebreak */ \
      0x0A,                  /* LINE FEED (LF) */                         \
      0x0D                   /* CARRIAGE RETURN (CR) */


namespace base {
enum TrimPositions {
  TRIM_NONE = 0,
  TRIM_LEADING = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL = TRIM_LEADING | TRIM_TRAILING,
};

const char kWhitespaceASCII[] = {WHITESPACE_ASCII, 0};

bool TrimString(const std::string& input,
                std::string* output,
                const std::string& trim_chars=kWhitespaceASCII);

std::vector<std::string> SplitString(const std::string& str,
                                     std::string::value_type delimiter);

} // namespace base

#endif  // BASE_STRING_UTIL_H_
