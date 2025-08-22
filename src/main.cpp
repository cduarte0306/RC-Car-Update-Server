#include <iostream>
#include <chrono>
#include <thread>
#include <type_traits>

#include "updater.hpp"
#include <chrono>


int main(int argc, char* argv[]) {
    Updater updater;
    
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }  

    return 0;
}