### Build dependencies

```
$ sudo apt-get install build-essential
$ sudo apt-get install libssl-dev
```

ubuntu18.04

```
$ sudo apt update 
$ sudo apt install nlohmann-json-dev
```

ubuntu20.04

```
$ sudo apt update 
$ sudo apt install nlohmann-json3-dev
```

### usage

```
    fs::path dstDir = "/home/ecopia/data/mapping";
    fs::path srcDir = "/home/ecopia/external_data/mapping";
    int maxThread = 2;
    int ret = SUCCESS;
    DataManagement dm(srcDir);

    // get dir list
    std::vector <std::string> dirList;
    dm.getSingleDirList(dirList);
    std::cout << "------------------ start print dir list ----------------" << std::endl;
    for (const auto &dir: dirList) {
        std::cout << dir << std::endl;
    }
    std::cout << "------------------ end print dir list -----------------" << std::endl;

    // set copy single dir thread
    dm.setMaxCopyThread(maxThread);

    // start copy directory
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
```

### Build instructions

```
$ mkdir build && cd build
$ cmake .. && make
```

### Code flow description:

```
1. Check whether the source folder exists (/data/ecopia/data/mapping)
2. Check whether the target folder exists (/data/ecopia/external_data/mapping)
3. Traverse the source folder
4. When traversing to the folder with the date prefix (regular match ^\\d{4}_\\d{2}_\\d{2}), start the copy process
5. While copying, it will check whether the size of the copied file and the md5 value are the same as the source file.
6. When the copy is completed, count the number and size of copied files and report them to the platform (TODO)
7. After completion, delete the folder with this time prefix
8. Continue traversing
```
