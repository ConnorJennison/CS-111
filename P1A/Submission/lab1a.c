//Include Files
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>

struct termios savedModes;
struct pollfd polls[2];
bool shell = false;
bool shell_closed = false;
bool terminal_closed = false;
pid_t shell_pid;

void restore_state()
{
    tcsetattr(0, TCSANOW, &savedModes);
}

void handler(int signum)
{
    if (signum == SIGPIPE)
    {
        fprintf(stderr, "SIGPIPE Error");
        exit(0);
    }
}
int main(int argc, char* argv[])
{
    //Option for shell
    static struct option long_options[] =
    {
        {"shell", no_argument, 0, 's'}
    };
    
    //Check to see if shell option
    int opt;
    while ( (opt = getopt_long(argc, argv, "s", long_options, NULL)) >= 0)
    {
        switch (opt)
        {
            case 's':
                shell= true;
                break;
            default:
                //Print correct usage
                printf("Usage: ./lab1 [--shell]\n");
                exit(1);
        }
    }
    
    /* GET AND SET NEW TERMINAL MODES */
    //Get saved modes
    
    if(tcgetattr(STDIN_FILENO, &savedModes)!= 0)
    {
        fprintf(stderr, "Unable to get terminal modes from fd %d, %s\n", STDIN_FILENO, strerror(errno));
        exit (1);
    }
    
    tcgetattr(STDIN_FILENO, &savedModes);
    atexit(restore_state);
    
    //Set raw mode
    
    struct termios rawModes = savedModes;
    rawModes.c_iflag = ISTRIP;
    rawModes.c_oflag = 0;
    rawModes.c_lflag = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &rawModes)!= 0)
    {
        fprintf(stderr, "Unable to set raw terminal modes for fd %d, %s\n", STDIN_FILENO, strerror(errno));
        exit(1);
    }
    /* Set up shell */
    if(shell)
    {
      signal(SIGPIPE, handler);
      //Create pipes
        int toshell[2];
        int fromshell[2];
        
        if (pipe(toshell) != 0)
        {
            fprintf(stderr, "Unable to create pipe to shell: %s\r\n", strerror(errno));
            exit(1);
        }
        
        if (pipe(fromshell) != 0)
        {
            fprintf(stderr, "Unable to create pipe from shell: %s\r\n", strerror(errno));
            exit(1);
        }
        //fork the shell process
        shell_pid = fork();
        if (shell_pid < 0)
        {
            fprintf(stderr, "Unable to fork shell process %s\r\n", strerror(errno));
            exit(1);
        }
        
        else if(shell_pid == 0)
        { //CHILD PROCESS
            
            // close the to pipe that recieves info from shell, and the from pipe that sends info
            close(toshell[1]);
            close(fromshell[0]);
            
            // use fd juggling to move stdin to the pipe
            close(0);
            dup(toshell[0]);
            close(toshell[0]);
            
            // make stdout/stderr get ouput to the pipe
            close(1);
            dup(fromshell[1]);
            close(2);
            dup(fromshell[1]);
            close(fromshell[1]);
            
            // execute a shell
            char* args[2];
            args[0] = "/bin/bash";
            args[1] = NULL;
            if (execvp("/bin/bash", args) == -1)
            {
                
                fprintf(stderr, "Error opening shell. %s\n", strerror(errno));
                exit(1);
            }
            
        }
        else //IN THE PARENT PROCESS
        { //PARENT PROCESS
            //close unecessary pipes in parent shell
            printf("Closing stuff");
            close(fromshell[1]);
            close(toshell[0]);
            
            struct pollfd keyboard;
            keyboard.fd = STDIN_FILENO;
            keyboard.events = POLLIN + POLLHUP + POLLERR;
            
            struct pollfd shell_out;
            shell_out.fd = fromshell[0];
            shell_out.events = POLLIN + POLLHUP + POLLERR;
            
            polls[0] = keyboard;
            polls[1] = shell_out;
            
            //Create buffer
            char* buffer = (char *) malloc(512 * sizeof(char));
            int num_chars;
            
            //Loop infinitely until shell exits
            for ( ; ; )
            {
                poll(polls, 2, 0);
                
                // Read data from keyboard, echo in STDOUT, forward to shell
                if (polls[0].revents & POLLIN)
                {
                    num_chars = read(STDIN_FILENO, buffer, sizeof(buffer));
                    if (num_chars > 0) //Process input
                    {
                        for (int i = 0; i < num_chars; i++)
                        {
                            // Carriage return or line feed
                            if (buffer[i] == '\r' || buffer[i] == '\n')
                            {
                                //Echo both to STDOUT
                                char* output = "\r\n";
                                write(STDOUT_FILENO, output, 2);
                                
                                //Send line feed to shell
                                if (shell_closed != true)
                                {
                                    write(toshell[1], "\n", 1);
                                }
                            }
                            
                            // Interrupt Character
                            else if (buffer[i] == 0x03)
                            {
                                //Output to console for easier debugging
                                char* output = "^C";
                                write(STDOUT_FILENO, output, 2);
                                
                                //Send SIGINT to Child process as specified
                                kill(shell_pid, SIGINT);
                            }
                            
                            // Process EOF
                            else if (buffer[i] == 0x04)
                            {
                                // Output to console for easier debugging
                                char* output = "^D";
                                write(STDOUT_FILENO, output, 2);
                                //Close pipe to shell but still recieve input
                                close(toshell[1]);
                            }
                            
                            // Normal characters, transfer to shell
                            else
                            {
                                write(STDOUT_FILENO, &buffer[i], 1);
                                if (shell_closed != true)
                                {
                                    write(toshell[1], &buffer[i], 1);
                                }
                            }
                        }
                    }
                }
                
                // Process output from child shell
                if (polls[1].revents & POLLIN)
                {
                    num_chars = read(fromshell[0], buffer, sizeof(buffer));
                    if (num_chars > 0)
                    {
                        for (int i = 0; i < num_chars; i++)
                        {
                            //RECIEVED EOF FROM SHELL, TIME TO TERMINATE
                            if (buffer[i] == 0x04)
                            {
                                //Need to get exit status from child process though
                                int exit_status;
                                if (waitpid(shell_pid, &exit_status, 0) == -1)
                                {
                                    fprintf(stderr, "Error with closing shell process. %s\n", strerror(errno));
                                    exit(1);
                                }
                                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(exit_status) ,WEXITSTATUS(exit_status));
                                exit(0);
                            }
                            
                            // TURN LINE FEED INTO LINE FEED AND CARRIAGE RETURN
                            else if(buffer[i] == '\n')
                            {
                                char* output = "\r\n";
                                write(STDOUT_FILENO, output, 2);
                            }
                            
                            // NORMAL OUTPUT
                            else
                            {
                                write(STDOUT_FILENO, &buffer[i], 1);
                            }
                        }
                    }
                }
                
                
                //LOOK TO SEE IF THE INPUT CLOSED ITS CONNECTION, I.E. WE DID ^D FROM TERMINAL
                if (polls[1].revents & POLLHUP)
                {
                    int exit_status;
                    if (waitpid(shell_pid, &exit_status, 0) == -1)
                    {
                        fprintf(stderr, "Error closing shell process. %s\n", strerror(errno));
                        exit(1);
                    }
                    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(exit_status) ,WEXITSTATUS(exit_status));
                    close(fromshell[0]);
                    exit(0);
                }
                
                //GET THE REST OF THE OUTPUT FROM THE SHELL BEFORE CLOSING
                if (polls[0].revents & POLLERR || polls[0].revents & POLLHUP)
                {
                    num_chars = read(fromshell[0], buffer, sizeof(buffer));
                    if (num_chars > 0)
                    {
                        for (int i = 0; i < num_chars; i++)
                        {
                            //RECIEVED EOF FROM SHELL, TIME TO TERMINATE
                            if (buffer[i] == 0x04)
                            {
                                //Need to get exit status from child process though
                                int exit_status;
                                if (waitpid(shell_pid, &exit_status, 0) == -1)
                                {
                                    fprintf(stderr, "Error with closing shell process. %s\n", strerror(errno));
                                    exit(1);
                                }
                                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(exit_status) ,WEXITSTATUS(exit_status));
                                exit(0);
                            }
                            
                            // TURN LINE FEED INTO LINE FEED AND CARRIAGE RETURN
                            else if(buffer[i] == '\n')
                            {
                                char* output = "\r\n";
                                write(STDOUT_FILENO, output, 2);
                            }
                            
                            // NORMAL OUTPUT
                            else
                            {
                                write(STDOUT_FILENO, &buffer[i], 1);
                            }
                        }
                    }
                    
                    //NO MORE OUTPUT FROM SHELL, WE CAN CLOSE NOW
                    else
                    {
                        int exit_status;
                        close(fromshell[0]);
                        if (waitpid(shell_pid, &exit_status, 0) == -1)
                        {
                            fprintf(stderr, "Error closing shell process. %s\n", strerror(errno));
                            exit(1);
                        }
                        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(exit_status) ,WEXITSTATUS(exit_status));
                        exit(0);
                    }
                }
            }
        }
    }
    
    // NO SHELL: READ FROM KEYBOARD INTO A BUFFER
    else
    {
        //Create buffer
        char* buffer = (char *) malloc(512 * sizeof(char));
        
        //Read into buffer
        int num_chars;
        while ((num_chars = read(STDIN_FILENO, buffer, 512)) != 0)
        {
            for (int i = 0; i < num_chars; i++)
            {
                //Terminating character
                if (buffer[i] == 0x04)
                {
                    exit(0);
                }
                //Carriage return characters or linefeed characters
                else if (buffer[i] == '\r' || buffer[i] == '\n')
                {
                    char* endline = (char *)malloc(2 * sizeof(char));
                    endline = "\r\n";
                    write(STDOUT_FILENO, endline, 2);
                }
                else //Ordinary
                    write(STDOUT_FILENO, &buffer[i], 1);
            }
        }
    }
    return 0;
}




