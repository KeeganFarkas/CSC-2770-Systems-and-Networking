#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <cstring>
#include <pthread.h>
#include <iostream>
#include <map>
#include <string>  
#include <stdexcept>
#include <regex>
#define PORT 8007
#define MAX_FILE_SIZE 20*1024

void  ASErrorHandler(int errorNumber, const char* errorMessage);
char*  ASMemoryAlloc(unsigned long memoryNeeded);
extern "C" char*  AStyleMain(const char* sourceIn,
                                    const char* optionsIn,
                                    void (* fpError)(int, const char*),
                                    char* (* fpAlloc)(unsigned long));

void *handle_connect(void *arg);
void parse(FILE *fp,
           std::map<std::string, std::string> &options,
           std::string &doc);

bool Req_Error;
std::string Req_Estr;

int main(int argc, char* argv[]) {
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        std::cout << "Error connecting\n"; 
    }

    bzero((void *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { 
        std::cout << "Error connecting\n"; 
    }

    listen(sockfd, 5);
    
    for (;;) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) { 
            std::cout << "Error connecting\n"; 
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_connect, (void *) (long) newsockfd);
    }
}

void *handle_connect(void *arg) {
    // detach the thread so that it will be cleaned up when it exits
    pthread_detach(pthread_self());

    // convert the argument to an int
    intptr_t nsptr = reinterpret_cast<intptr_t>(arg);
    int ns = static_cast<int>(nsptr);

    // create a file pointer from the socket
    FILE *fp = fdopen(ns, "r+");

    // set the file pointer to be unbuffered
    setvbuf(fp, NULL, _IONBF, 0);

    // initialize the options map and the document string
    std::string doc = "";
    std::map<std::string, std::string> options;

    // parse the file
    try {
        parse(fp, options, doc);
    } catch (std::exception &e) {
        // if there is an error, write the error to the file
        std::string err_msg("Error\nSIZE=" + std::to_string(strlen(e.what())) + 
                            "\n\n" + std::string(e.what()) + "\n"); 
        fprintf(fp, err_msg.c_str());
        fclose(fp);
        return NULL;
    }

    // initialize the parameter string
    std::string parameter = "";

    // add the options to the parameter string
    for (auto const& [key, value] : options) {
        if (key != "SIZE") {
            parameter += key + "=" + value + "\n";
        }
    }

    // set the error handler and memory allocator
    Req_Error = false;
    Req_Estr = "";

    // call the AStyleMain function
    char* textOut = AStyleMain(doc.c_str(), parameter.c_str(), ASErrorHandler, 
                               ASMemoryAlloc);
    if (Req_Error) {
        // if there is an error, write the error to the file
        std::string ret_message("ERROR\nSIZE=" + std::to_string(Req_Estr.size()) + 
                                "\n\n" + Req_Estr.c_str() + "\n");
        fprintf(fp, ret_message.c_str());
    } else {
        // if there is no error, write the formatted code to the file
        std::string ret_message("OK\nSIZE=" + std::to_string(strlen(textOut)) + 
                                "\n\n" + textOut + "\n");
        fprintf(fp, ret_message.c_str());
    }

    // close the file pointer and return
    fclose(fp);
    return NULL;
}

void parse(FILE *fp,
           std::map<std::string, std::string> &options,
           std::string &doc) {
    char line[1024];
    std::string key;
    std::string value;
    int size;

    // read the header
    if(fgets(line, sizeof(line), fp) == nullptr) {
        throw std::runtime_error("Unexpected end of file when reading header");
    }

    // trim the newline
    line[strlen(line) - 1] = '\0';


    // make sure the header is correct
    std::string header = line;
    if (header != "ASTYLE") {
        throw std::runtime_error("Expected header ASTYLE, but got " + header);
    }

    for(;;) {
        // read the next line
        if(fgets(line, sizeof(line), fp) == nullptr) {
            throw std::runtime_error("Unexpected end of file when reading options");
        }

        // if the line is a newline then we are done reading options
        if(line[0] == '\n') {
            break;
        }

        // trim the newline
        line[strlen(line) - 1] = '\0';

        if(std::regex_match(line, std::regex("[a-zA-Z0-9]+=[a-zA-Z0-9]+"))) {
            // finding the position of the '='
            int pos = std::string(line).find('=');

            // split the string into key and value
            key = std::string(line).substr(0, pos);
            value = std::string(line).substr(pos + 1);

            // if key is not "SIZE", "mode", or "style" then throw an error
            if (key != "SIZE" && key != "mode" && key != "style") {
                throw std::runtime_error("Bad option");
            }
        }
        else {
            // if the line does not match the regex then throw an error
            throw std::runtime_error("Bad option");
        }

        if (key == "SIZE") {
            // if the key is "SIZE" then convert the value to an int
            try{
                size = std::stoi(value);
            } catch (const std::exception &e) {
                // if the value is not an int then throw an error
                throw std::runtime_error("Bad code size");
            }
        }

        // add the option to the options map
        options[key] = value;        
    }

    if (size < 0 || size > MAX_FILE_SIZE) {
        // if the size is not in the correct range then throw an error
        throw std::runtime_error("Bad code size");
    }

    // resize the document string and read the code into it
    doc.resize(size);
    fread(&doc[0], 1, size, fp);
}

void ASErrorHandler(int errorNumber, const char* errorMessage) {   
    Req_Error = true;
    Req_Estr += errorMessage + std::string("\n");
}

char* ASMemoryAlloc(unsigned long memoryNeeded) {
    char* buffer = new (std::nothrow) char [memoryNeeded];
    return buffer;
}