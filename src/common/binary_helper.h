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