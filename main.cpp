#include "data_management.h"
#include "constants.h"

int main() {
    fs::path dstDir = "/home/ecopia/data/mapping";
    fs::path srcDir = "/home/ecopia/external_data/mapping";
    int maxThread = 2;
    int ret = SUCCESS;
    int64_t userID = 108688500468;
    std::string agentID = "vehicle-liangyuxu-test-35";
    DataManagement dm(srcDir, userID, agentID);
    // TODO need to response copy_directory_start
    auto primaryEventID = dm.getPrimaryEventID();

    std::vector <std::string> dirList;
    dm.getSingleDirList(dirList);
    std::cout << "------------------ start print dir list ----------------" << std::endl;
    for (const auto &dir: dirList) {
        std::cout << dir << std::endl;
    }
    std::cout << "------------------ end print dir list -----------------" << std::endl;

    dm.setMaxCopyThread(maxThread);

    std::thread t([&dm, &ret, &dstDir]() { ret = dm.copyDirectory(dstDir); });

    // test stop copy
    std::thread t1([&dm]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "active copyStopSignal" << std::endl;
        dm.stopCopy();
    });

    t.join();
    t1.join();
    //int ret = dc.copyDirectory();
    if (ret != SUCCESS) {
        std::cout << "copyDirectory failed" << std::endl;
    }
//    auto start = std::chrono::high_resolution_clock::now();
//    auto size = dc.getUsedSpace("/data/alpaca-backend-cgi");
//    auto end = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//
//    std::cout << "getUsedSpace finished, size: [" << size << "], use ms: " << duration.count() << std::endl;

    return 0;
}