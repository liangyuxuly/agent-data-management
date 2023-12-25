#include "directory_copy.h"
#include "constants.h"
#include "thread_pool.h"
#include <iostream>
#include <cstdio>

DirectoryCopy::DirectoryCopy(fs::path &srcDir, fs::path &dstDir, int maxThread = 5)
        : _srcDir(srcDir), _dstDir(dstDir), _maxThreads(maxThread), _dir_pattern("^\\d{4}_\\d{2}_\\d{2}") {}

DirectoryCopy::~DirectoryCopy() {}

int DirectoryCopy::copyDirectory() {
    int ret = checkDirectory(_srcDir);
    if (ret == SUCCESS) {
        ret = checkDirectory(_dstDir);
    }

    if (ret == SUCCESS) {
        ret = traverseDirectory(_srcDir);
    }

    return ret;
}

int DirectoryCopy::traverseDirectory(const fs::path &path) {
    int ret = SUCCESS;
    if (fs::is_directory(path)) {
        for (const auto &entry: fs::directory_iterator(path)) {
            if (fs::is_directory(entry)) {
                if (hasPrefix(entry, _dir_pattern)) {
                    auto srcDir = entry.path();
                    auto relativePathStr = srcDir.string();
                    auto destination = _dstDir.string() + relativePathStr.substr(_srcDir.string().length());
                    ret = copySingleDirectory(srcDir, destination);
                    if (ret == SUCCESS) {
                        ret = deleteDirectory(srcDir);
                        if (ret != SUCCESS) {
                            std::cout << "delete src dir failed, srcDir: [" << srcDir << "]" << std::endl;
                            break;
                        }
                    } else {
                        std::cout << "copySingleDirectory failed, srcDir: [" << srcDir << "], dstDir: [" << destination
                                  << "]" << std::endl;
                        break;
                    }
                    std::cout << "copy: srcDir[" << srcDir << "], destination: [" << destination << "]" << std::endl;
                    continue;
                }
                ret = traverseDirectory(entry);
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

    unsigned long long freeSize = stat.f_bfree * stat.f_frsize;
    //std::cout << "src dir Free size: " << freeSize << " bytes\n";
    //std::cout << "src dir Free size: " << freeSize / 1024.0 / 1024 / 1024 << " GB\n";

    unsigned long long usedSize = getUsedSpace(srcDir);
    //std::cout << "dst dir used size: " << usedSize << " bytes\n";
    //std::cout << "dst dir used size: " << usedSize / 1024.0 / 1024 / 1024 << " GB\n";

    if (usedSize > freeSize) {
        std::cerr << "no enough space to copy, srcDir: [" << srcDir.string() << "], dstDir: [" << dstDir.string()
                  << "], src size: " << usedSize << ", dst free size: " << freeSize << std::endl;
        return ERR_NO_ENOUGH_SPACE;
    }

    return SUCCESS;
}

//uintmax_t DirectoryCopy::getUsedSpace(const fs::path &dir) {
//    uintmax_t size = 0;
//    for (auto &p: fs::recursive_directory_iterator(dir))
//        if (p.is_regular_file()) size += p.file_size();
//    return size;
//}

uintmax_t DirectoryCopy::getUsedSpace(const fs::path &dir) {
    uintmax_t size = 0;
    char command[1024];
    sprintf(command, "du -sb %s", dir.string().c_str());
    // std::cout << command << std::endl;
    FILE *stream = popen(command, "r");
    if (stream) {
        fscanf(stream, "%ju", &size);
        pclose(stream);
    }
    return size;
}

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

int DirectoryCopy::copySingleDirectory(const fs::path &sourceDir, const fs::path &destinationDir) {
    if (fs::exists(destinationDir)) {
        std::cout << "Destination directory " << destinationDir.string() << " already exists, delete it" << std::endl;
        auto deleteErr = this->deleteDirectory(destinationDir);
        if (deleteErr != SUCCESS) {
            std::cout << "delete directory failed!" << std::endl;
            return ERR_DELETE_DIRECTORY_FAILED;
        }
    }

    if (!fs::create_directories(destinationDir)) {
        std::cout << "Could not create destination directory" << std::endl;
        return ERR_CREATE_DIRECTORY_FAILED;
    }
    int ret = checkSpace(sourceDir, destinationDir);
    if (ret != SUCCESS) {
        std::cout << "check space failed" << std::endl;
        return ret;
    }

    //std::cout << "_maxThreads: " << _maxThreads << std::endl;
    ThreadPool pool(_maxThreads);

    for (const auto &dirEntry: fs::recursive_directory_iterator(sourceDir)) {
        const auto &sourcePath = dirEntry.path();
        auto relativePathStr = sourcePath.string();
        auto destination = destinationDir.string() + relativePathStr.substr(sourceDir.string().length());
        //std::cout << "sourcePath: " << sourcePath.string() << ", relativePathStr: " << relativePathStr << ", destination: " << destination << std::endl;
        if (fs::is_directory(sourcePath)) {
            if (!fs::create_directories(destination)) {
                std::cout << "create directories failed" << std::endl;
                return ERR_CREATE_DIRECTORY_FAILED;
            }
        } else {
            pool.enqueue([sourcePath, destination]() {
                copy_file(sourcePath, destination);
            });
        }
    }
    return SUCCESS;
}

bool DirectoryCopy::hasPrefix(const fs::path &path, std::string &pattern) {
    std::string fileName = path.filename().string();

    // 使用正则表达式匹配时间前缀
    std::regex prefixRegex(pattern);
    std::smatch match;
    if (std::regex_search(fileName, match, prefixRegex)) {
        // 提取时间前缀
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