#include <iostream>
#include <chrono>
#include <thread>
#include <type_traits>
#include "updater.hpp"
#include <chrono>
#include <iostream>
#include "version.h"

#include "utils/logger.hpp"


int main(int argc, char* argv[]) {
    Logger::getLoggerInst()->log(Logger::LOG_LVL_INFO, "RC Car Update Server Version: %u.%u.%u\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
    Updater updater;
    
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }  

    return 0;
}

