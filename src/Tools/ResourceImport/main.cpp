#include "FirstEngine/Tools/ResourceImport.h"
#include <iostream>

int main(int argc, char* argv[]) {
    FirstEngine::Tools::ResourceImport importer;
    
    if (!importer.ParseArguments(argc, argv)) {
        return 1;
    }
    
    return importer.Execute();
}
