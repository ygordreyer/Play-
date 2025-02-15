#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"
#include "../Source/Log.h"
#include "Types.h"

int main(int argc, char* argv[])
{
    // Get logger instance (singleton handles initialization)
    CLog::GetInstance();
    
    // Run tests
    int result = Catch::Session().run(argc, argv);
    
    return result;
}