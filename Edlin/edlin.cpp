#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>

// function to parse a user command for a line number and a Unix command
// and execute the Unix command on the line
void filter(const std::string &user_cmd, std::vector<std::string> &lines);

int main() {
    // declaring variables for user command and lines
    std::string user_cmd;
    std::vector<std::string> lines;

    // printing the welcome message and prompting the user for a command
    std::cout << "Line editor" << std::endl;
    std::cout << "edlin> ";

    // reading the user command
    std::getline(std::cin, user_cmd);

    // looping until the user enters 'q' or end of file for stdin
    while(!std::cin.eof()){
        // trimming the user command
        user_cmd.erase(0, user_cmd.find_first_not_of(" \t\n\r\f\v"));
        user_cmd.erase(user_cmd.find_last_not_of(" \t\n\r\f\v") + 1);

        // if the user command is 'q', exit the editor
        if(user_cmd == "q") {
            std::cout << "Exiting the editor" << std::endl;
            break;
        }
        else {
            // if the user command is 'l', print all the lines
            if(user_cmd == "l") {
                // looping through all the lines
                for (long unsigned int i = 0; i < lines.size(); i++) {
                    std::cout << i << " " << lines[i] << std::endl;
                }
            }
            // if the user command is 'r', erase current lines and 
            // read lines from a file
            else if(user_cmd.substr(0, 2) == "r ") {
                // using a string stream to get the filename
                std::stringstream ss(user_cmd.substr(2));
                std::string filename;
                ss >> filename;

                // using ifstream to read the file
                std::ifstream infile(filename);
                if(infile.is_open()) {
                    // if the file is opened successfully
                    std::string line;
                    lines.clear();
                    // reading the file line by line into the lines vector
                    while(std::getline(infile, line)) {
                        lines.push_back(line);
                    }
                }
                else {
                    // if the file is not opened successfully
                    std::cout << "Error: file not found" << std::endl;
                }
            }
            // if the user command is 's', write the lines to a file
            else if(user_cmd.substr(0, 2) == "s ") {
                // using a string stream to get the filename
                std::stringstream ss(user_cmd.substr(2));
                std::string filename;
                ss >> filename;

                // using ofstream to write to the file
                std::ofstream outfile(filename);
                if(outfile.is_open()) {
                    // if the file is opened successfully, 
                    // write the lines to the file
                    for (const auto &line : lines) {
                        outfile << line << std::endl;
                    }
                }
                // if the file is not opened successfully
                else {
                    std::cout << "Error: file not found" << std::endl;
                }
            }
            // if the user command is 'e', replace a line
            else if(user_cmd.substr(0, 2) == "e ") {
                // using a string stream to get the line number and 
                // the new line
                std::stringstream ss(user_cmd.substr(2));
                long unsigned int lno;
                if(!(ss >> lno)){
                    std::cout << "Error: invalid command" << std::endl;
                    continue;
                };
                std::string line;
                std::getline(ss, line);

                // trimming the new line
                line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
                line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

                // if the line number is greater than the number of lines or
                // negative, add the new line to the end of the lines vector
                if(lno >= lines.size()) {
                    lines.push_back(line);
                }
                // if the line number is a valid line, replace the line
                else if(lno >= 0){
                    lines[lno] = line;
                }
            }
            // if the user command is '!', call the filter function
            else if(user_cmd.substr(0, 1) == "!") {
                filter(user_cmd, lines);
            }
            // if the user command is none of the above, print an error message
            else {
                std::cout << "Error: invalid command" << std::endl;
            }
        }

        // prompting the user for a command
        std::cout << "edlin> ";
        std::getline(std::cin, user_cmd);
    }
    
    return 0;
}

void filter(const std::string &user_cmd, std::vector<std::string> &lines) {
    // creating a string stream from the user command
    std::stringstream ss(user_cmd.substr(1));

    // getting line number
    long unsigned int lno;
    if(!(ss >> lno)) {
        // if there is nothing after '!' or the line number is not an integer
        std::cout << "Error: invalid command" << std::endl;
        return;
    }
    if(lno >= lines.size()) {
        // if the line number is negative or greater than the number of lines
        std::cout << "Error: invalid line number" << std::endl;
        return;
    }

    // getting Unix command
    std::string ucmd;
    std::getline(ss, ucmd);
    if(ucmd.empty()) {
        // if no Unix command is given
        std::cout << "Error: no Unix command given" << std::endl;
        return;
    }

    // creating pipes
    int from_parent[2], to_parent[2];
    if(pipe(from_parent) == -1 || pipe(to_parent) == -1) {
        // if pipe creation fails
        fprintf(stderr, "Error: pipe failed");
        return;
    }

    // forking a child process
    pid_t pid;
    if((pid = fork()) == -1) {
        // if fork fails
        fprintf(stderr, "Error: fork failed");
        close(to_parent[0]);
        close(to_parent[1]);
        close(from_parent[0]);
        close(from_parent[1]);
        return;
    }

    // child process
    if(pid == 0) {
        // calling dup2 to redirect child input/output to corresponding pipes
        if(dup2(from_parent[0], 0) == -1 || dup2(to_parent[1], 1) == -1){
            // if dup2 fails
            fprintf(stderr, "Error: dup2 failed");
            close(to_parent[0]);
            close(to_parent[1]);
            close(from_parent[0]);
            close(from_parent[1]);
            return;
        }
        close(to_parent[0]);
        close(to_parent[1]);
        close(from_parent[0]);
        close(from_parent[1]);

        // calling execve to execute the Unix command
        const char *args[] = {"/bin/sh", "-c", ucmd.c_str(), 0};
        if(execve(args[0], (char *const *)args, 0) < 0) {
            // if execve fails
            fprintf(stderr, "Error: execve failed");
            exit(1);
        }
    }
    // parent process
    else if(pid > 0) {
        // closing the unused ends of the pipes
        close(from_parent[0]);
        close(to_parent[1]);

        // writing to the pipe
        FILE *write_end = fdopen(from_parent[1], "w");
        if(write_end == NULL) {
            // if fdopen fails
            fprintf(stderr, "Error: fdopen failed");
            close(to_parent[0]);
            close(to_parent[1]);
            close(from_parent[0]);
            close(from_parent[1]);
            return;
        }
        fprintf(write_end, "%s", lines[lno].c_str());
        fclose(write_end);

        // reading from the pipe
        FILE *read_end = fdopen(to_parent[0], "r");
        if(read_end == NULL) {
            // if fdopen fails
            fprintf(stderr, "Error: fdopen failed");
            close(to_parent[0]);
            close(to_parent[1]);
            close(from_parent[0]);
            close(from_parent[1]);
            return;
        }
        char buf[1024];
        if(fgets(buf, sizeof(buf), read_end) != NULL) {
            // if a line was successfully read
            lines[lno] = buf;
        }
        fclose(read_end);

        // waiting for the child process to terminate
        int status;
        waitpid(pid, &status, 0);
    }
}