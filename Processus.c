#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h> 
#include <fcntl.h>

#define I2C_FICHIER "/dev/i2c-1" // fichier Linux représentant le BUS #1
#define I2C_ADRESSE 0x29 // adresse du capteur VL6180X
#define VL6180X_ID 0xb4 // ID attendu pour le capteur VL6180X

// Define pipes
int pipe_measure[2]; // Pipe for communication between father and son
int pipe_display[2]; // Pipe for communication between son and grandson

// Function prototypes
void initPipes();
void closePipes();
void parentProcess();
void childProcess();
void grandchildProcess();
int lireDistance(int fd);

int main() {
    // Create pipes
    initPipes();
    
    pid_t child_pid = fork();

    if (child_pid < 0) 
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) 
    {
        // This is the child process
        childProcess();
    } 
    else 
    {
        // This is the parent process
        parentProcess();
        wait(NULL); // Wait for the child process to finish
    }

    closePipes();
    return 0;
}

void initPipes() 
{
    // Create the pipes
    if (pipe(pipe_measure) == -1 || pipe(pipe_display) == -1) 
    {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }
}

void closePipes() 
{
    close(pipe_measure[0]); // Close reading end of pipe_measure
    close(pipe_measure[1]); // Close writing end of pipe_measure
    close(pipe_display[0]); // Close reading end of pipe_display
    close(pipe_display[1]); // Close writing end of pipe_display
}

void parentProcess() {
    char command;
    printf("Peser sur 'm' pour mesurer, 'Q' pour quitter:\n");

    while (1) 
    {
        command = getchar();
        if (command == 'm')
        {
            write(pipe_measure[1], "1", 1); // Notify child to start measuring
        } 
        else if (command == 'Q' || command == 'q') 
        {
            write(pipe_measure[1], "0", 1); // Notify child to stop
            break;
        }
    }
}

void childProcess() 
{
    pid_t grandchild_pid = fork();

    if (grandchild_pid < 0) 
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (grandchild_pid == 0) 
    {
        // This is the grandchild process
        grandchildProcess();
    } 
    else 
    {
        // This is the child process
        char buffer[1];
        while (1) 
        {
            if (read(pipe_measure[0], buffer, 1) > 0) 
            {
                if (buffer[0] == '1') 
                {
                    // Notify grandchild to measure
                    write(pipe_display[1], "1", 1);
                }
                else if (buffer[0] == '0') 
                {
                    // Notify grandchild to stop measuring
                    write(pipe_display[1], "0", 1);
                    break;
                }
            }
        }
        wait(NULL); // Wait for the grandchild to finish
    }
}

void grandchildProcess() 
{
    // Open I2C device
    int fdPortI2C = open(I2C_FICHIER, O_RDWR);
    if (fdPortI2C == -1) 
    {
        perror("Error opening I2C port");
        exit(EXIT_FAILURE);
    }

    if (ioctl(fdPortI2C, I2C_SLAVE, I2C_ADRESSE) < 0) 
    {
        perror("Error configuring I2C address");
        close(fdPortI2C);
        exit(EXIT_FAILURE);
    }

    char buffer[1];
    while (1) 
    {
        if (read(pipe_display[0], buffer, 1) > 0) 
        {
            if (buffer[0] == '1') 
            {
                int distance = lireDistance(fdPortI2C);
                if (distance >= 0) 
                {
                    printf("Distance: %d mm\n", distance);
                }
            } 
            else if (buffer[0] == '0') 
            {
                break; // Stop measuring
            }
        }
        usleep(500000); // Sleep for a while before next measurement
    }

    close(fdPortI2C); // Close I2C device
    exit(0);
}

// Function to read distance from VL6180X
int lireDistance(int fd) 
{
    uint8_t start_range_cmd[3] = {0x00, 0x18, 0x01}; // Command to start measurement
    if (write(fd, start_range_cmd, 3) != 3) 
    {
        perror("Error writing start range command");
        return -1;
    }

    usleep(100000); // Wait for measurement to complete

    uint8_t range_register[2] = {0x00, 0x62}; // Register for distance result
    if (write(fd, range_register, 2) != 2) 
    {
        perror("Error writing range register");
        return -1;
    }

    uint8_t distance;
    if (read(fd, &distance, 1) != 1) 
    {
        perror("Error reading distance");
        return -1;
    }

    return distance; // Return the measured distance
}


