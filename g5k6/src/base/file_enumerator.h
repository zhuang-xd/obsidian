#ifndef BASE_FILE_ENUMERATOR_H_
#define BASE_FILE_ENUMERATOR_H_

#include <string>
#include <vector>

class FileEnumerator {
public:
    FileEnumerator() = default;
    ~FileEnumerator() = default;

public:
    static std::vector<std::string> FetchAll(const std::string& root,
        const std::vector<std::string>& ext_filter=std::vector<std::string>(),
        bool recursion=false);
};

#endif  // BASE_FILE_ENUMERATOR_H_
