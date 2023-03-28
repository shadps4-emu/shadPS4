#include <iostream>
#include <sys/stat.h>
#include <direct.h> 
#include "PKG.h"
int main()
{
    PKG pkg;
    if (!pkg.open("test.pkg"))
    {
        std::cout << "Error reading test.pkg\n";
        return 0;
    }
    pkg.printPkgHeader();
    std::string gamedir = "game/" + pkg.getTitleID();
    struct stat sb;
    if (stat(gamedir.c_str(), &sb) != 0)
    {
        _mkdir(gamedir.c_str());
    }
    std::string extractpath = "game/" + pkg.getTitleID() + "/";
    std::string failreason;
    if (!pkg.extract("test.pkg", extractpath, failreason))
    {
        std::cout << "Error extraction " << failreason;
    }
}

