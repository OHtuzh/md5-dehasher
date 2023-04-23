#include "Decoder.h"

#include <iostream>
#include <iterator>
#include <set>

#include "boost/algorithm/hex.hpp"

namespace ohtuzh {

    std::string Decoder::ContinueDecoding() {
        if (!LoadData("alphabet.cfg")) {
            throw;
        }
        std::ifstream fin("hash.cfg");
        if (fin.bad()) {
            throw;
        }
        std::string expected_hash;
        fin >> expected_hash;
        fin.close();

        fin.open("brute_force_data.cfg");
        if (fin.bad()) {
            throw;
        }

        const auto caller =
                [this](StarterValues sv, const std::string& wanted_hash) {
                    BruteForce(sv.left, sv.right, wanted_hash, sv);
                };

        unsigned int number_of_threads;
        fin >> number_of_threads;
        StarterValues sv{};
        for (unsigned int i = 0; i < number_of_threads; ++i) {
            fin >> sv.init_length >> sv.init_i >> sv.init_current >> sv.left >> sv.right;
            threads_.emplace_back(caller, sv, expected_hash);
        }
        WaitForResult(expected_hash);

        return answer_;
    }

    std::string Decoder::Decode(const std::string& expected_hash, const std::filesystem::path& config_file_path) {
        if (expected_hash.length() != 32) {
            std::cerr << "Wrong expected_hash length: 32 expected, got " << expected_hash.length() << " instead"
                      << std::endl;
            throw;
        }
        if (!LoadData(config_file_path)) {
            throw;
        }

        request_to_stop_ = false;

        const unsigned int kNumberOfThreads = std::min(std::thread::hardware_concurrency(),
                                                       static_cast<unsigned int>(dictionary_.size()));

        const unsigned int kInitialSymbolsPerThread = dictionary_.size() / kNumberOfThreads;
        const unsigned int kInitialSymbolsInLastThread =
                kInitialSymbolsPerThread + dictionary_.size() % kInitialSymbolsPerThread;

        const auto caller = [this](unsigned int left, unsigned int right, const std::string& wanted_hash) {
            BruteForce(left, right, wanted_hash, {1, left, 0ull});
        };

        for (unsigned int i = 0; i < dictionary_.size();) {
            if (threads_.size() + 1 == kNumberOfThreads) {
                threads_.emplace_back(caller, i, i + kInitialSymbolsInLastThread, expected_hash);
                i += kInitialSymbolsInLastThread;
            } else {
                threads_.emplace_back(caller, i, i + kInitialSymbolsPerThread, expected_hash);
                i += kInitialSymbolsPerThread;
            }
        }

        WaitForResult(expected_hash);
        return answer_;
    }

    bool Decoder::LoadData(const std::filesystem::path& config_file_path) {
        if (config_file_path.extension() != ".cfg") {
            std::cerr << "Not '.cfg' file" << std::endl;
            return false;
        }
        std::ifstream fin(config_file_path);
        if (fin.bad() || !fin.is_open()) {
            std::cerr << "Troubles with file!" << std::endl;
            return false;
        }

        dictionary_.clear();
        std::set<char> tmp_container;
        for (auto it = std::istream_iterator<char>(fin); it != std::istream_iterator<char>(); ++it) {
            tmp_container.insert(*it);
        }
        std::copy(tmp_container.begin(), tmp_container.end(), std::back_inserter(dictionary_));
        return true;
    }

    void
    Decoder::BruteForce(unsigned int left, unsigned int right, const std::string& expected_hash, StarterValues values) {
        for (unsigned int length = values.init_length; answer_.empty() && !request_to_stop_; ++length) {
            for (unsigned int i = values.init_i; i < right; ++i) {
                values.init_i = left;
                std::string str;
                str.resize(length, ' ');
                str[0] = dictionary_[i];
                if (length == 1) {
                    if (IsSameHash(str, expected_hash)) {
                        answer_ = str;
                        return;
                    }
                    continue;
                }
                const auto upper_bound = static_cast<unsigned long long>(std::pow(dictionary_.size(), length - 1));
                for (auto current = values.init_current; current < upper_bound; ++current) {
                    values.init_current = 0ull;
                    auto tmp = current;
                    for (int j = 1; j < length; ++j) {
                        str[j] = dictionary_[tmp % dictionary_.size()];
                        tmp /= dictionary_.size();
                    }
                    if (IsSameHash(str, expected_hash)) {
                        answer_ = str;
                        return;
                    }
                    if (request_to_stop_) {
                        std::lock_guard lg(stop_data_mutex_);
                        stop_data_ << length << ' ' << i << ' ' << current << ' ' << left << ' ' << right << '\n';
                        return;
                    }
                }
            }
        }
    }

    bool Decoder::IsSameHash(const std::string& to_encode, const std::string& hash) {
        boost::uuids::detail::md5::digest_type digest;
        boost::uuids::detail::md5 md5;

        md5.process_bytes(to_encode.data(), to_encode.size());
        md5.get_digest(digest);

        std::string encoded;
        encoded.reserve(32);
        boost::algorithm::hex(digest, digest + 4, std::back_inserter(encoded));
        std::transform(encoded.begin(), encoded.end(), encoded.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return encoded == hash;
    }


}
