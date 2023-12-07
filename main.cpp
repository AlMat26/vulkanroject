#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "src/app.h"
#include "src/window.h"

std::vector<char> readShader(std::string path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}


int main(int argc, char **argv) 
{
    Application app{};
    Window window{};

    window.init();
    app.init(window);


    std::cout << "Hello, world!" << std::endl;

    auto vertCode = readShader("../shaders/shader.vert.spv");
    auto fragCode = readShader("../shaders/shader.frag.spv");

    std::cout << vertCode.size() << std::endl;
    std::cout << fragCode.size() << std::endl;

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!window.isShouldClose()) {
        window.pollEvents();

        window.swapBuffers();
    }

    return 0;
}
