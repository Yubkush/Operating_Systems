#include "hw2_test.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#include <wait.h>


int main() { // A
    pid_t A = getpid();
    set_weight(10);
    if(fork() == 0) { // B 
        set_weight(40);
        pid_t B_parent = getppid(); // A pid
        if(fork() == 0) { // C
            set_weight(100);
            pid_t C_parent = getppid(); // B pid
            if(fork() == 0) { // D
                set_weight(60);
                pid_t D_parent = getppid(); // C pid
                if(fork() == 0) { // E
                    set_weight(25);
                    pid_t E_parent = getppid(); // D pid
                    pid_t ret = get_heaviest_ancestor();
                    assert(ret == D_parent);
                    std::cout << "===== SUCCESS =====" << std::endl;
                }
                else{
                    wait(nullptr);
                }
            }
            else{
                wait(nullptr);
            }
        }
        else{
            wait(nullptr);
        }
    }
    else{
        wait(nullptr);
    }    
    return 0;
}