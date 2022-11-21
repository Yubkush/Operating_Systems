#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() { 
    char s[] = "A";
    // fork();
    int fd = open("file", O_RDWR|O_CREAT);  
    write(fd, "B", 1); 
    close(fd);  
    fd = open("file", O_RDWR|O_CREAT);  
    close(STDIN_FILENO);   
    close(STDOUT_FILENO); 
    dup(fd);  
    dup(fd); 
    if (fork() == 0) { 
        scanf("%s", s);  
        printf("C"); 
        write(STDERR_FILENO, s, 1); 
        return 0; 
    } 
    s[0] = 'D';
    wait(NULL); 
    printf("E"); 
    close(fd); 
    return 0; 
}

// output: B(son first),E(father close(fd) before child),A(printf(E) happens first)
// file: BCE(20,21,27,28), BEC(20,27,21,28), EC(27,20,21,28), C(27,20,28,21), EC(27,28,20,21), CE(20,27,28,21)
