#ifndef VULKANPROJECT_LOADBINFILE_H
#define VULKANPROJECT_LOADBINFILE_H

#include <fstream>
#include <string>
#include <vector>

namespace utils {
    std::vector<char> readFile(const std::string &filename);
}

#endif //VULKANPROJECT_LOADBINFILE_H
