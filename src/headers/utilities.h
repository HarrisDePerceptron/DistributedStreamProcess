#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace Utilities
{

    std::vector<std::byte> readFileBinary(std::string path)
    {
        std::ifstream fileInput{path, std::ios::binary | std::ios::ate};

        if (!fileInput)
        {
            throw std::runtime_error{"unable to read file: " + path};
        }

        auto end = fileInput.tellg();
        fileInput.seekg(0, std::ios::beg);

        auto start = fileInput.tellg();

        auto fileSize = static_cast<long unsigned int>(end - start);

        std::vector<std::byte> buff{fileSize};

        fileInput.read((char *)buff.data(), buff.size());

        fileInput.close();

        return buff;
    }


    std::string binaryToString(std::vector<std::byte> &  buff){
        std::string stringBuff{reinterpret_cast<const char *>(buff.data()), buff.size()};
        return stringBuff;
    }
}