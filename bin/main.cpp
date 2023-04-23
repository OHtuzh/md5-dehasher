#include <iostream>

#include "lib/Decoder.h"

#include "boost/algorithm/hex.hpp"

void TryToDecode(ohtuzh::Decoder& decoder, const std::string& hash, const std::filesystem::path& config) {
    std::cout << decoder.Decode(hash, config) << std::endl;
}

void TryToContinueDecoding(ohtuzh::Decoder& decoder) {
    std::cout << decoder.ContinueDecoding() << std::endl;
}

void WaitInput(ohtuzh::Decoder& decoder) {
    std::string s;
    while (true) {
        std::cin >> s;
        if (s == "pause") {
            decoder.RequestPause();
            return;
        } else {
            std::cout << "Unknown command!" << std::endl;
        }
    }
}


int main(int argc, char** argv) {
    ohtuzh::Decoder decoder;
    std::thread th(WaitInput, std::ref(decoder));
    if (argc == 2) {
        if (std::string(argv[1]) != "resume") {
            std::cerr << "Invalid call construction!" << std::endl;
            return -1;
        }
        TryToContinueDecoding(decoder);
    } else if (argc == 3) {
        std::string hash = argv[1];
        std::filesystem::path config_file_path = argv[2];
        TryToDecode(decoder, hash, config_file_path);
    } else {
        std::cerr << "Invalid call construction!" << std::endl;
        return -1;
    }
    th.detach();

    return 0;
}
