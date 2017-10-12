//
//  p0.c
//  
//
//  Created by Connor Jennison on 10/2/17.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

void force_seg_fault()
{
    char* ptr = NULL;
    *ptr = 5;
}

void sig_handler(int signal_num)
{
    //Handle signal
    fprintf(stderr, "Caught signal %d.\n", signal_num);
    exit(4);
}

int main(int argc, char* argv[])
{
    
    //Struct for getopt_long containing possible arguments for program
    static struct option long_options[] =
    {
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"segfault", no_argument, 0, 's'},
        {"catch", no_argument, 0, 'c'}
    };
    
    //Set up file names and seg flag
    char* input_file = NULL;
    char* output_file = NULL;
    int segflag = 0;
    int opt;
    
    while( (opt = getopt_long(argc, argv, "i:o:sc", long_options, NULL)) >= 0)
    {
        switch (opt)
        {
            case 'i':
                input_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 's':
                segflag = 1;
                break;
            case 'c':
                //Alert signal hanlder
                signal(SIGSEGV, sig_handler);
                break;
            default:
                printf("Usage: lab0 --input input_filename --output output_filename\n");//Print the correct usage
                exit(1);
        }
    }

    //Do any file redirection
    
    //Redirect input
    if (input_file)
    {
        int input_fd = open(input_file, O_RDONLY);
        
        //If successful open, do the redirection
        if (input_fd >= 0)
        {
            close(0);
            dup(input_fd);
            close(input_fd);
        }
        
        //Open failed, report error and exit
        else
        {
            fprintf(stderr, "Error opening input file: %s\n", strerror(errno));
            exit(2);
        }
    }
    
    //Redirect output
    if (output_file)
    {
        int output_fd = creat(output_file, S_IRWXU);
        
        //Valid create
        if (output_fd >= 0)
        {
            close(1);
            dup(output_fd);
            close(output_fd);
        }
        
        //Create failed, report error and exit
        else
        {
            fprintf(stderr, "Error creating output file: %s\n", strerror(errno));
            exit(3);
        }
    }
    
    //Cause the segfault
    if (segflag)
    {
        force_seg_fault();
    }
    
    //If not segfault caused, copy STDIN to STDOUT
    char* buffer;
    buffer = (char*) malloc(sizeof(char));
    int status = read(0,buffer,1);
    while(status > 0)
    {
        write(1,buffer,1);
        status = read(0,buffer,1);
    }
    free(buffer);
    
    return 0;
}
