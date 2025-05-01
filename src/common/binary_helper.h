#pragma once 

#include <iostream>
#include <vector>  
#include "common/logging/log.h"

using u32 = uint32_t;

template <typename T>  
void writeBin(std::ostream& os, const T& v) {
    LOG_INFO(Render_Recompiler, "BinaryHelper: Pos: {}", static_cast<int64_t>(os.tellp()));
    os.write(reinterpret_cast<const char*>(&v), sizeof(T));  
}  

template <typename T>  
void readBin(std::istream& is, T& v) {  
    is.read(reinterpret_cast<char*>(&v), sizeof(T));  
}  

// Spezialfall für Arrays/Blöcke  
template <typename T>  
void writeBlock(std::ostream& os, const T* data, size_t count) {  
    os.write(reinterpret_cast<const char*>(data), sizeof(T) * count);  
}  

template <typename T>  
void readBlock(std::istream& is, T* data, size_t count) {  
    is.read(reinterpret_cast<char*>(data), sizeof(T) * count);  
}  

// Spezialfall für Container  
template <typename T>  
void writeContainer(std::ostream& os, const std::vector<T>& v) {  
    u32 n = static_cast<u32>(v.size());  
    writeBin(os, n);  
    if (n)  
        writeBlock(os, v.data(), n);  
}  

template <typename T>  
void readContainer(std::istream& is, std::vector<T>& v) {  
    u32 n;  
    readBin(is, n);  
    v.resize(n);  
    if (n)  
        readBlock(is, v.data(), n);  
}