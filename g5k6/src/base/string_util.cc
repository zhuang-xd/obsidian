#include "base/string_util.h"


namespace base::internal
{
template <typename T, typename CharT = typename T::value_type>
std::basic_string<CharT> ToLowerASCIIImpl(T str) {
  std::basic_string<CharT> ret;
  ret.reserve(str.size());
  for (size_t i = 0; i < str.size(); i++) {
    ret.push_back(ToLowerASCII(str[i]));
  }
  return ret;
}

template <typename T, typename CharT = typename T::value_type>
std::basic_string<CharT> ToUpperASCIIImpl(T str) {
  std::basic_string<CharT> ret;
  ret.reserve(str.size());
  for (size_t i = 0; i < str.size(); i++) {
    ret.push_back(ToUpperASCII(str[i]));
  }
  return ret;
}

template <typename T, typename CharT = typename T::value_type>
TrimPositions TrimStringT(T input,
                          T trim_chars,
                          TrimPositions positions,
                          std::basic_string<CharT>* output) {
  // Find the edges of leading/trailing whitespace as desired. Need to use
  // a std::string_view version of input to be able to call find* on it with the
  // std::string_view version of trim_chars (normally the trim_chars will be a
  // constant so avoid making a copy).
  const size_t last_char = input.length() - 1;
  const size_t first_good_char =
      (positions & TRIM_LEADING) ? input.find_first_not_of(trim_chars) : 0;
  const size_t last_good_char = (positions & TRIM_TRAILING)
                                    ? input.find_last_not_of(trim_chars)
                                    : last_char;

  // When the string was all trimmed, report that we stripped off characters
  // from whichever position the caller was interested in. For empty input, we
  // stripped no characters, but we still need to clear |output|.
  if (input.empty() || first_good_char == std::basic_string<CharT>::npos ||
      last_good_char == std::basic_string<CharT>::npos) {
    bool input_was_empty = input.empty();  // in case output == &input
    output->clear();
    return input_was_empty ? TRIM_NONE : positions;
  }

  // Trim.
  output->assign(input.data() + first_good_char,
                 last_good_char - first_good_char + 1);

  // Return where we trimmed from.
  return static_cast<TrimPositions>(
      (first_good_char == 0 ? TRIM_NONE : TRIM_LEADING) |
      (last_good_char == last_char ? TRIM_NONE : TRIM_TRAILING));
}
} // namespace base::internal

namespace base {

bool TrimString(const std::string& input,
                std::string* output,
                const std::string& trim_chars)
{
    return (internal::TrimStringT(input, trim_chars, TRIM_ALL, output) !=
        TRIM_NONE);
}

std::vector<std::string> SplitString(const std::string& str, std::string::value_type delimiter) {
  std::vector<std::string> result;
  if (str.empty())
    return result;

  using ViewType = std::string;

  size_t start = 0;
  while (start != ViewType::npos) {
    size_t end = str.find_first_of(delimiter, start);

    ViewType piece;
    if (end == ViewType::npos) {
      piece = str.substr(start);
      start = ViewType::npos;
    } else {
      piece = str.substr(start, end - start);
      start = end + 1;
    }

    TrimString(piece, &piece);
    if (!piece.empty())
      result.emplace_back(piece);
  }
  return result;
}


}  // namespace base