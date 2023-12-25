#include <iostream>
#include "directory_copy.h"
#include <chrono>

int main() {
    fs::path dstDir = "/home/ecopia/data/mapping";
    fs::path srcDir = "/home/ecopia/external_data/mapping";
    DirectoryCopy dc(srcDir, dstDir, 2);
    dc.copyDirectory();
//    auto start = std::chrono::high_resolution_clock::now();
//    auto size = dc.getUsedSpace("/data/alpaca-backend-cgi");
//    auto end = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//
//    std::cout << "getUsedSpace finished, size: [" << size << "], use ms: " << duration.count() << std::endl;

    return 0;
}
