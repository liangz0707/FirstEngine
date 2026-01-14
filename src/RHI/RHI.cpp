// RHI module implementation file
// This file exists to ensure the DLL is properly created on Windows

#include "FirstEngine/RHI/Export.h"

namespace FirstEngine {
    namespace RHI {
        // RHI is primarily an interface library
        // This file ensures proper DLL export on Windows
        
        // Export a dummy symbol to ensure import library is generated
        FE_RHI_API int GetRHIVersion() {
            return 1;
        }
    }
}
