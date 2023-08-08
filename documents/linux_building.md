
## Linux

Generate the build directory in the shadPS4 directory:
```
cmake -S . -B build/
```

Enter the directory:
```
cd build/
```

Use make to build the project:
```
make -j$(nproc)
```
