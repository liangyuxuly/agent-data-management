### Build dependencies

```
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
DirectoryCopy dc(srcDir, dstDir, maxThread);
int ret = dc.copyDirectory();
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
