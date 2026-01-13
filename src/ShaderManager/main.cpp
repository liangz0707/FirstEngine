#include "FirstEngine/ShaderManager/ShaderManager.h"
#include <iostream>

int main(int argc, char* argv[]) {
    FirstEngine::ShaderManager::ShaderManager manager;
    
    if (!manager.ParseArguments(argc, argv)) {
        return 1;
    }
    
    return manager.Execute();
}
