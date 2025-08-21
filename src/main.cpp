#include <iostream>
#include <chrono>
#include <thread>
#include <type_traits>

#include "updater.hpp"


int main(int argc, char* argv[]) {
    Updater updater;
    updater.joinThread();  

    return 0;
}