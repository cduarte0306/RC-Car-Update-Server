#include <iostream>
#include <chrono>
#include <thread>
#include <type_traits>

#include "updater.hpp"
#include <chrono>

#include <iostream>

#include "version.h"


int main(int argc, char* argv[]) {
    
    std::cout << "Updater started\r\n";
    std::cout << "Version: " << VERSION_MAJOR << "." << VERSION_MINOR<< "." << VERSION_BUILD << "\r\n";
    
    Updater updater;
    
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }  

    return 0;
}

