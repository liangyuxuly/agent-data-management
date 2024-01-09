#include "directory_copy.h"
#include "constants.h"
#include "thread_pool.h"
#include <iostream>
#include <cstdio>
#include <nlohmann/json.hpp>

DirectoryCopy::DirectoryCopy(fs::path &srcDir, fs::path &dstDir, int maxThread)
        : _srcDir(srcDir), _dstDir(dstDir), _maxThreads(maxThread), _dirPattern("^\\d{4}_\\d{2}_\\d{2}"),
          _copySingleTickerInterval(500), _copyStopSignal(false) {}

DirectoryCopy::~DirectoryCopy() {}

int DirectoryCopy::copyDirectory() {
    int ret = checkDirectory(_srcDir);
    if (ret == SUCCESS) {
        ret = checkDirectory(_dstDir);
    }

    std::vector<std::string> dirList;
    nlohmann::json overall;

    if (ret == SUCCESS) {
        ret = traverseDirectory(_srcDir, dirList);
    }

    if (ret == SUCCESS) {
        _copyDetails.clear();
        overall["src_dir"] = _srcDir.string();
        overall["dst_dir"] = _dstDir.string();
        overall["copy_dir_list"] = dirList;
        overall["total_dir_count"] = dirList.size();
        _copyDetails["overall"] = overall;
        _copyDetails["step_tag"] = STEP_TAG_COPY_DIRECTORY_START;
        _copyDetails["step_status"] = STEP_STATUS_SUCCESS;
        std::cout << _copyDetails.dump(4) << std::endl;
        // TODO upload to platform

        ret = traverseDirectoryAndCopy(_srcDir);
        if (ret == SUCCESS) {
            _copyDetails["step_status"] = STEP_STATUS_SUCCESS;
        } else if (ret == STOP_COPY_SIGNAL_RECEIVED) {
            _copyDetails["step_status"] = STEP_STATUS_STOPPED;
            // reset ret
            ret = SUCCESS;
        } else {
            _copyDetails["step_status"] = STEP_STATUS_FAILED;
            _copyDetails["step_err_msg"] = getErrMsg(ret);
        }
        _copyDetails["step_tag"] = STEP_TAG_COPY_DIRECTORY_FINISHED;
        std::cout << _copyDetails.dump(4) << std::endl;
        // TODO upload to platform
        std::cout << "finish copy, src: [" << _srcDir.string() << "], dst: [" << _dstDir.string() << "]" << std::endl;
    }

    return ret;
}

int DirectoryCopy::traverseDirectory(const fs::path &path, std::vector<std::string> &dirList) {
    int ret = SUCCESS;
    if (fs::is_directory(path)) {
        for (const auto &entry: fs::directory_iterator(path)) {
            if (fs::is_directory(entry)) {
                if (hasPrefix(entry, _dirPattern)) {
                    dirList.push_back(entry.path().string());
                } else {
                    ret = traverseDirectory(entry, dirList);
                    if (ret != SUCCESS) {
                        break;
                    }
                }
            } else if (fs::is_regular_file(entry)) {
                //std::cout << entry.path() << std::endl;
                continue;
            }
        }
    }
    return ret;
}

int DirectoryCopy::traverseDirectoryAndCopy(const fs::path &path) {
    if (_copyStopSignal) {
        return STOP_COPY_SIGNAL_RECEIVED;
    }
    int ret = SUCCESS;
    if (fs::is_directory(path)) {
        for (const auto &entry: fs::directory_iterator(path)) {
            if (fs::is_directory(entry)) {
                if (hasPrefix(entry, _dirPattern)) {
                    ret = copySingleDirectory(entry.path());
                    if (ret != SUCCESS) {
                        break;
                    }
                    continue;
                }
                ret = traverseDirectoryAndCopy(entry);
                if (ret != SUCCESS) {
                    break;
                }
            } else if (fs::is_regular_file(entry)) {
                //std::cout << entry.path() << std::endl;
                continue;
            }
        }
    }
    return ret;
}

int DirectoryCopy::checkDirectory(const fs::path &path) {
    if (!fs::exists(path)) {
        std::cout << "directory " << path.string() << " not exists, please check." << std::endl;
        return ERR_DIRECTORY_NOT_EXISTS;
    }
    return SUCCESS;
}

int DirectoryCopy::checkSpace(const fs::path &srcDir, const fs::path &dstDir) {
    struct statvfs stat;

    if (statvfs(dstDir.string().c_str(), &stat) != 0) {
        std::cerr << "Failed to get file system statistics\n";
        return ERR_STAT_FILEPATH_FAILED;
    }

    uint64_t freeSize = stat.f_bfree * stat.f_frsize;
    //std::cout << "src dir Free size: " << freeSize << " bytes\n";
    //std::cout << "src dir Free size: " << freeSize / 1024.0 / 1024 / 1024 << " GB\n";

    uint64_t totalFileCount = 0, totalFileSize = 0;
    totalFileSize = getUsedSpace(srcDir, totalFileCount);
    _copyDetails["total_file_count"] = totalFileCount;
    _copyDetails["total_file_size"] = totalFileSize;
    //std::cout << "dst dir used size: " << usedSize << " bytes\n";
    //std::cout << "dst dir used size: " << usedSize / 1024.0 / 1024 / 1024 << " GB\n";

    if (totalFileSize > freeSize) {
        std::cerr << "no enough space to copy, srcDir: [" << srcDir.string() << "], dstDir: [" << dstDir.string()
                  << "], src size: " << totalFileSize << ", dst free size: " << freeSize << std::endl;
        return ERR_NO_ENOUGH_SPACE;
    }

    return SUCCESS;
}

uint64_t DirectoryCopy::getUsedSpace(const fs::path &dir, uint64_t &fileCount) {
    uintmax_t size = 0;
    for (auto &p: fs::recursive_directory_iterator(dir))
        if (p.is_regular_file()) {
            size += p.file_size();
            fileCount++;
        }
    return size;
}

//uintmax_t DirectoryCopy::getUsedSpace(const fs::path &dir) {
//    uintmax_t size = 0;
//    char command[1024];
//    sprintf(command, "du -sb %s", dir.string().c_str());
//    // std::cout << command << std::endl;
//    FILE *stream = popen(command, "r");
//    if (stream) {
//        fscanf(stream, "%ju", &size);
//        pclose(stream);
//    }
//    return size;
//}

int DirectoryCopy::deleteDirectory(const fs::path &deleteDir) {
    try {
        fs::remove_all(deleteDir);
        //std::cout << "Successfully removed directory: " << deleteDir << std::endl;
    } catch (fs::filesystem_error &e) {
        std::cout << "Failed to remove directory: " << deleteDir << std::endl;
        std::cout << "Reason: " << e.what() << std::endl;
        return ERR_DELETE_DIRECTORY_FAILED;
    }
    return SUCCESS;
}

std::string DirectoryCopy::calculateMD5(const std::string &path) {
    std::ifstream file(path, std::ifstream::binary);
    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    char buf[1024 * 16];
    while (file.good()) {
        file.read(buf, sizeof(buf));
        MD5_Update(&md5Context, buf, file.gcount());
    }

    unsigned char result[MD5_DIGEST_LENGTH];
    MD5_Final(result, &md5Context);

    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int) result[i];

    return ss.str();
}

int DirectoryCopy::copySingleFile(const fs::path &source, const std::string &target) {
    fs::copy(source, target);

    // Check if the source and destination files have the same size
    if (fs::file_size(source) != fs::file_size(target)) {
        std::cout << "Error copying file " << source.string() << std::endl;
        return ERR_DIFFERENT_FILE_SIZE;
    }

    std::string sourceMD5 = this->calculateMD5(source.string());
    std::string targetMD5 = this->calculateMD5(target);
    if (sourceMD5 != targetMD5) {
        std::cout << "Error copying file " << source.string() << std::endl;
        return ERR_DIFFERENT_MD5;
    } else {
        std::cout << "copy file: [" << target << "] success, md5: [" << targetMD5 << "]." << std::endl;
    }
    return SUCCESS;
}

void DirectoryCopy::updateCopyDetails(const fs::path &path) {
    std::lock_guard<std::mutex> lock(_copyMutex);
    uint64_t copyFileCount = _copyDetails["current"]["copy_file_count"].get<uint64_t>();
    uint64_t copyFileSize = _copyDetails["current"]["copy_file_size"].get<uint64_t>();
    _copyDetails["current"]["copy_file_count"] = copyFileCount + 1;
    _copyDetails["current"]["copy_file_size"] = copyFileSize + fs::file_size(path);
}

void DirectoryCopy::printCopyDetails() {
    std::lock_guard<std::mutex> lock(_copyMutex);
    _copyDetails["step_tag"] = STEP_TAG_COPY_SINGLE_DIRECTORY_IN_PROGRESS;
    // TODO upload to platform
    std::cout << "copy progress, details: " << _copyDetails.dump(4) << std::endl;
}

int DirectoryCopy::copySingleDirectory(const fs::path &srcDir) {
    // auto srcDir = entry.path();
    if (_copyStopSignal) {
        return STOP_COPY_SIGNAL_RECEIVED;
    }

    int ret = SUCCESS;
    auto relativeDirStr = srcDir.string();
    auto dstDir = _dstDir.string() + relativeDirStr.substr(_srcDir.string().length());

    nlohmann::json current;
    current["src_dir"] = srcDir.string();
    current["dst_dir"] = dstDir;
    current["copy_file_count"] = 0ull;
    current["copy_file_size"] = 0ull;

    // TODO need upload to platform when single directory copy start (YYYY_MM_DD_xx_xx_xx)
    std::cout << "start copy single directory: src: [" << srcDir.string() << "], dst: [" << dstDir << "]" << std::endl;

    if (fs::exists(dstDir)) {
        std::cout << "Destination directory " << dstDir << " already exists, delete it" << std::endl;
        ret = this->deleteDirectory(dstDir);
        if (ret != SUCCESS) {
            std::cout << "delete directory failed!" << std::endl;
        }
    }

    if (ret == SUCCESS) {
        try {
            if (!fs::create_directories(dstDir)) {
                std::cout << "could not create destination directory: " << dstDir << std::endl;
                ret = ERR_CREATE_DIRECTORY_FAILED;
            }
        } catch (fs::filesystem_error &e) {
            std::cout << "Failed to create destination directory: " << dstDir << std::endl;
            std::cout << "Reason: " << e.what() << std::endl;
            ret = ERR_CREATE_DIRECTORY_FAILED;
        }
    }

    if (ret == SUCCESS) {
        struct statvfs stat;

        if (statvfs(dstDir.c_str(), &stat) != 0) {
            std::cout << "Failed to get file system statistics" << std::endl;
            ret = ERR_STAT_FILEPATH_FAILED;
        } else {
            uint64_t freeSize = stat.f_bfree * stat.f_frsize;
            //std::cout << "src dir Free size: " << freeSize << " bytes\n";
            //std::cout << "src dir Free size: " << freeSize / 1024.0 / 1024 / 1024 << " GB\n";

            uint64_t totalFileCount = 0, totalFileSize = 0;
            totalFileSize = getUsedSpace(srcDir, totalFileCount);
            current["total_file_count"] = totalFileCount;
            current["total_file_size"] = totalFileSize;
            //std::cout << "dst dir used size: " << usedSize << " bytes\n";
            //std::cout << "dst dir used size: " << usedSize / 1024.0 / 1024 / 1024 << " GB\n";

            if (totalFileSize > freeSize) {
                std::cout << "no enough space to copy, srcDir: [" << srcDir.string() << "], dstDir: [" << dstDir
                          << "], src size: " << totalFileSize << ", dst free size: " << freeSize << std::endl;
                ret = ERR_NO_ENOUGH_SPACE;
            }
        }
    }

    if (ret == SUCCESS) {
        _copyDetails["current"] = current;
        _copyDetails["step_tag"] = STEP_TAG_COPY_SINGLE_DIRECTORY_START;
        _copyDetails["step_status"] = STEP_STATUS_SUCCESS;
        // _copyDetails["overall"].erase("");
        // TODO upload to platform
        std::cout << "start copy, details: " << _copyDetails.dump(4) << std::endl;
        //std::thread t([this]() { this->ticker(); });
        ThreadPool pool(_maxThreads);

        // print copy progress thread
        std::atomic<bool> copy_stop = false;
        std::thread t([this, &copy_stop]() { this->ticker(copy_stop); });

        int copyFailedCount = 0;
        for (const auto &dirEntry: fs::recursive_directory_iterator(srcDir)) {
            const auto &srcPath = dirEntry.path();
            auto relativePathStr = srcPath.string();
            auto dstPath = dstDir + relativePathStr.substr(srcDir.string().length());
            std::cout << "srcPath: " << srcPath.string() << ", relativePathStr: " << relativePathStr << ", dstPath: "
                      << dstPath << std::endl;
            if (fs::is_directory(srcPath)) {
                try {
                    if (!fs::create_directories(dstPath)) {
                        std::cout << "create directories failed: " << dstPath << std::endl;
                        ret = ERR_CREATE_DIRECTORY_FAILED;
                        break;
                    }
                } catch (fs::filesystem_error &e) {
                    std::cout << "Failed to create directories file: " << dstPath << std::endl;
                    std::cout << "Reason: " << e.what() << std::endl;
                    ret = ERR_CREATE_DIRECTORY_FAILED;
                    break;
                }
            } else {
                pool.enqueue([srcPath, dstPath, this, &copyFailedCount]() {
                    try {
                        if (fs::copy_file(srcPath, dstPath)) {
                            this->updateCopyDetails(srcPath);
                        }
                        //std::cout << "Successfully removed directory: " << deleteDir << std::endl;
                    } catch (fs::filesystem_error &e) {
                        std::cout << "Failed to copy file: " << srcPath << std::endl;
                        std::cout << "Reason: " << e.what() << std::endl;
                        copyFailedCount++;
                    }
                });
            }
        }

        copy_stop = true;
        t.detach();

        if (copyFailedCount > 0) {
            std::cout << "some file copy failed, failed count: " << copyFailedCount << std::endl;
            ret = ERR_PARTIAL_COPY_FAILED;
        }
    }

    if (ret == SUCCESS) {
        ret = deleteDirectory(srcDir);
        if (ret != SUCCESS) {
            std::cout << "delete src dir failed, srcDir: [" << srcDir << "]" << std::endl;
        }
    } else {
        std::cout << "copySingleDirectory failed, srcDir: [" << srcDir << "], dstDir: [" << dstDir
                  << "]" << std::endl;
    }

    if (ret == SUCCESS) {
        _copyDetails["step_status"] = STEP_STATUS_SUCCESS;

        if (_copyDetails["overall"].find("finished_dir_list") == _copyDetails["overall"].end()) {
            _copyDetails["overall"]["finished_dir_list"] = {srcDir.string()};
        } else {
            std::vector<std::string> dirList = _copyDetails["overall"]["finished_dir_list"];
            dirList.push_back(srcDir.string());
            _copyDetails["overall"]["finished_dir_list"] = dirList;
        }
    } else {
        _copyDetails["step_status"] = STEP_STATUS_FAILED;
        _copyDetails["step_err_msg"] = getErrMsg(ret);
    }
    _copyDetails["step_tag"] = STEP_TAG_COPY_SINGLE_DIRECTORY_FINISHED;
    // TODO upload to platform when single directory copy finished (YYYY_MM_DD_xx_xx_xx)
    std::cout << "copy single directory finished, details: " << _copyDetails.dump(4) << std::endl;
    current.clear();

    return ret;
}

bool DirectoryCopy::hasPrefix(const fs::path &path, std::string &pattern) {
    std::string fileName = path.filename().string();

    std::regex prefixRegex(pattern);
    std::smatch match;
    if (std::regex_search(fileName, match, prefixRegex)) {
//        std::string timePrefix = match.str();
//
//        std::time_t now = std::time(nullptr);
//        std::tm *localTime = std::localtime(&now);
//        char formattedTimePrefix[11];
//        std::strftime(formattedTimePrefix, sizeof(formattedTimePrefix), "%Y_%m_%d", localTime);
//
//        return timePrefix == formattedTimePrefix;
        //std::cout << path.string() << std::endl;
        return true;
    }

    return false;
}

void DirectoryCopy::ticker(std::atomic<bool> &stop) {
    std::cout << "start ticker" << std::endl;
    while (!stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(_copySingleTickerInterval));
        printCopyDetails();
    }
    std::cout << "stop ticker" << std::endl;
}

void DirectoryCopy::stopCopy() {
    _copyStopSignal = true;
}