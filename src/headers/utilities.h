#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include<algorithm>



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

        fileInput.read(reinterpret_cast<char *>(buff.data()), buff.size());

        fileInput.close();

        return buff;
    }

    std::string binaryToString(std::vector<std::byte> &buff)
    {
        std::string stringBuff{reinterpret_cast<const char *>(buff.data()), buff.size()};
        return stringBuff;
    }

    std::vector<std::string> splitString(const std::string &s, char token)
    {

        std::stringstream test{s};
        std::string segment;
        std::vector<std::string> seglist;

        while (std::getline(test, segment, token))
        {
            seglist.push_back(segment);
        }

        return seglist;
    }

    bool is_number(const std::string &s)
    {

        auto begin = s.begin();
        if (*begin=='-'){
            begin++;
        }

        return !s.empty() && std::find_if(begin,
                                          s.end(), [](unsigned char c)
                                          { return !std::isdigit(c); }) == s.end();
    }
}