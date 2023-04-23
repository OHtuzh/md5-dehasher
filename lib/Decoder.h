#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <thread>
#include <fstream>

#include "boost/uuid/detail/md5.hpp"

namespace ohtuzh {

    class Decoder {
    public:
        Decoder() = default;

        std::string Decode(const std::string& expected_hash, const std::filesystem::path& config_file_path);

        std::string ContinueDecoding();

        void RequestPause() noexcept {
            stop_data_.open("brute_force_data.cfg");
            stop_data_ << threads_.size() << '\n';
            request_to_stop_ = true;
        }

        struct StarterValues {
            unsigned int init_length;
            unsigned int init_i;
            unsigned long long init_current;
            unsigned int left;
            unsigned int right;
        };
    private:
        bool LoadData(const std::filesystem::path& config_file_path);

        void BruteForce(unsigned int left, unsigned int right, const std::string& expected_hash, StarterValues values);

        static bool IsSameHash(const std::string& to_encode, const std::string& hash);

        void WaitForResult(const std::string& expected_hash) {
            while (answer_.empty() && !request_to_stop_) {}

            for (auto& t: threads_) {
                t.join();
            }

            if (request_to_stop_) {
                std::ofstream of("alphabet.cfg");
                for (char c: dictionary_) {
                    of << c;
                }
                of.close();
                of.open("hash.cfg");
                of << expected_hash;
            }
        }

        std::vector<std::thread> threads_;
        std::vector<char> dictionary_;
        std::string answer_;
        bool request_to_stop_{false};

        std::mutex stop_data_mutex_;
        std::ofstream stop_data_;
    };

}