#include "shader.h"
#include <fstream>
#include <sstream>

std::string loadShaderFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
