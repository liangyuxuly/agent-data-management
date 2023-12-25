### Build dependencies

```
$ sudo apt-get install libssl-dev
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
