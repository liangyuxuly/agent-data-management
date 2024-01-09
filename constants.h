#ifndef DIRECTORYCOPY_CONSTANTS_H
#define DIRECTORYCOPY_CONSTANTS_H

constexpr int SUCCESS = 0;
constexpr int STOP_COPY_SIGNAL_RECEIVED = 1;
constexpr int ERR_UNKNOWN = -1;
constexpr int ERR_NO_ENOUGH_SPACE = -2;
constexpr int ERR_DELETE_DIRECTORY_FAILED = -3;
constexpr int ERR_DIFFERENT_FILE_SIZE = -4;
constexpr int ERR_DIFFERENT_MD5 = -5;
constexpr int ERR_CREATE_DIRECTORY_FAILED = -6;
constexpr int ERR_DIRECTORY_NOT_EXISTS = -7;
constexpr int ERR_STAT_FILEPATH_FAILED = -8;
constexpr int ERR_PARTIAL_COPY_FAILED = -9;

inline std::string getErrMsg(const int errCode) {
    switch (errCode) {
        case SUCCESS:
            return "SUCCESS";
        case ERR_UNKNOWN:
            return "unknown error";
        case ERR_NO_ENOUGH_SPACE:
            return "no enough space";
        case ERR_DELETE_DIRECTORY_FAILED:
            return "delete directory failed";
        case ERR_DIFFERENT_FILE_SIZE:
            return "different file size";
        case ERR_DIFFERENT_MD5:
            return "different md5";
        case ERR_CREATE_DIRECTORY_FAILED:
            return "create directory failed";
        case ERR_DIRECTORY_NOT_EXISTS:
            return "directory not exists";
        case ERR_STAT_FILEPATH_FAILED:
            return "get filepath stat failed";
        case ERR_PARTIAL_COPY_FAILED:
            return "some files failed to copy";
        default:
            return "UNKNOWN";
    }
}

const std::string STEP_TAG_COPY_DIRECTORY_START = "copy_directory_start";
const std::string STEP_TAG_COPY_DIRECTORY_FINISHED = "copy_directory_finished";
const std::string STEP_TAG_COPY_SINGLE_DIRECTORY_START = "copy_single_directory_start";
const std::string STEP_TAG_COPY_SINGLE_DIRECTORY_IN_PROGRESS = "copy_single_directory_in_progress";
const std::string STEP_TAG_COPY_SINGLE_DIRECTORY_FINISHED = "copy_single_directory_finished";

const std::string STEP_STATUS_SUCCESS = "success";
const std::string STEP_STATUS_STOPPED = "stopped";
const std::string STEP_STATUS_FAILED = "failed";

#endif //DIRECTORYCOPY_CONSTANTS_H
