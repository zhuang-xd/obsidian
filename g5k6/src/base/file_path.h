#ifndef BASE_FILE_Path_H_
#define BASE_FILE_Path_H_

#include <stdint.h>
#include <string>
#include <vector>


class FilePath {
public:
    FilePath(const std::string& path);
    FilePath() = default;
    ~FilePath() = default;

public:
    using StringType = std::string;
    using CharType = StringType::value_type;

    static constexpr CharType kSeparators[] = "/";

    static constexpr size_t kSeparatorsLength = sizeof(kSeparators);

    static constexpr CharType kCurrentDirectory[] = ".";

    // The special path component meaning "the parent directory."
    static constexpr CharType kParentDirectory[] = "..";

    // The character used to identify a file extension.
    static constexpr CharType kExtensionSeparator = '.';

public:
    StringType Extension(void) const;
    StringType BaseName(void) const;

    bool empty() const {
        return path_.empty();
    }

    const StringType& value() const {
        return path_;
    }

    static int64_t Size(int fd);

private:
    StringType path_;
};

#endif  // BASE_FILE_Path_H_


