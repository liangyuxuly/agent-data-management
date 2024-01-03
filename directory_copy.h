#ifndef DIRECTORYCOPY_DIRECTORY_COPY_H
#define DIRECTORYCOPY_DIRECTORY_COPY_H

#include <iostream>
#include <sys/statvfs.h>
#include <filesystem>
#include <openssl/md5.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>
#include <regex>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
//namespace json = nlohmann::json;

class DirectoryCopy {
public:
    DirectoryCopy(fs::path &, fs::path &, int);

    ~DirectoryCopy();

    int copyDirectory();

private:
    int traverseDirectory(const fs::path &);

    int checkDirectory(const fs::path &);

    int checkSpace(const fs::path &, const fs::path &);

    int deleteDirectory(const fs::path &);

    std::string calculateMD5(const std::string &);

    int copySingleFile(const fs::path &, const std::string &);

    int copySingleDirectory(const fs::path &);

    void updateCopyDetails(const fs::path &);

    void printCopyDetails();

    bool hasPrefix(const fs::path &, std::string &);

    uint64_t getUsedSpace(const fs::path &, uint64_t &);

    void ticker();

private:
    int _maxThreads;
    std::string _dirPattern;
    fs::path _srcDir;
    fs::path _dstDir;
    std::mutex _copy_mutex;
    nlohmann::json _copyDetails;
    std::atomic<bool> _copy_single_stop;
    int _copy_single_ticker_interval;
};

#endif //DIRECTORYCOPY_DIRECTORY_COPY_H
