#include <stdio.h>
#include <stdlib.h>   
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <unordered_set>
#include <cmath>
#include <iomanip>

void fireman(int sig){
    while(waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

 // My entropy calculation logic that is based on the Incremental Entropy calculation based on Dr.Rincon's algorithm
 // Code from PA1
std::vector<double> calculate_entropy(std::string task, std::vector<int> cpuCount)
{
    std::unordered_set<char> seen; // use of unordered_set to keep track of task char already seen before
    std::vector<char> unique;
    for( char c: task){// removing duplicate Task char without changing the order
        if(seen.insert(c).second){
            unique.push_back(c);
        }
    }  
    
    // creating and initalizing FreqArray with no duplicate Tasks char and 0 frequency
    std::vector<std::pair<char, int>> NewFreqArray;  
    for(auto c: unique){
        NewFreqArray.push_back(std::make_pair(c,0));
    }
   
    std::vector<double> H;
    int FreqOfSelectedTask = 0;
    double h = 0.0;
    int currFreq = 0;
    double currH = 0.0;
    double currentTerm = 0;
    int size = task.size(); // size is calculated based on the number of tasks present in the given string
    
    //entropy calculation
    for(int i = 0; i < size; i++){
        char selectedTask = task.at(i);
        int extraFreq = cpuCount[i];
        int NFreq = currFreq + extraFreq;
        
        if(NFreq == extraFreq){
            h = 0.00;
        }
        else{
            for(int i = 0; i < NewFreqArray.size(); i++){
                if(selectedTask == NewFreqArray.at(i).first){
                    FreqOfSelectedTask = NewFreqArray.at(i).second;
                }
            }
            if(FreqOfSelectedTask == 0){
                currentTerm = 0;
            }else{
                currentTerm = FreqOfSelectedTask * std::log2(FreqOfSelectedTask);
            }
            double newTerm = (FreqOfSelectedTask + extraFreq) * log2(FreqOfSelectedTask + extraFreq);
            if(NFreq != 0){
                h = log2(NFreq) - ((log2(currFreq) - currH) * currFreq - currentTerm + newTerm) / NFreq;
            }
            else{
                h = 0.00;
            }
        }
        //Updating the New Frequency array 
        for(int i = 0; i < NewFreqArray.size(); i++){
            if(NewFreqArray.at(i).first == selectedTask){
                NewFreqArray.at(i).second += extraFreq;
            }
        }
        //updating h and frequency variables
        currH = h;
        H.push_back(h); //adding the new entropy value to vector H
        currFreq = NFreq;
    }
    
    return H; // H is a vector<double> that has the entropy values
}

void handle_client_request(int newsockfd)
{
    int n, msgSize = 0;
    n = read(newsockfd, &msgSize, sizeof(int));
    if(n < 0){
        std::cerr << "Error reading from socket" << std::endl;
        exit(1);
    }
    char *tempBuffer = new char[msgSize + 1];
    bzero(tempBuffer, msgSize + 1);
    n = read(newsockfd, tempBuffer, msgSize+1);
    if(n < 0){
        std::cerr << "Error reading from socket" << std::endl;
        exit(1);
    }
    std::string buffer = tempBuffer;
    delete[] tempBuffer;
    
    // formating input paramaters that are needed to be passed in for my entropy_calculation function
    std::vector<int> cpuCount; // Define cpuCount based on your input format
    std::istringstream iss(buffer); // inputString contains the scheduling information
    std::string task;
    int count;
    std::string tasks = "";
    while (iss >> task >> count) {
        tasks += task;
        cpuCount.push_back(count);
    }
    
    // Call your entropy calculation function here
    std::vector<double> entropyValues = calculate_entropy(tasks, cpuCount);
    
    // Sending the calculated entropies to Client
    int entropies_size = entropyValues.size() * sizeof(double);
    n = write(newsockfd, &entropies_size, sizeof(int));
    if (n < 0){
        std::cerr << "Error writing size to socket" << std::endl;
        exit(1);
    }
    n = write(newsockfd, entropyValues.data(), entropies_size);
    if (n < 0){
        std::cerr << "Error writing to socket" << std::endl;
        exit(1);
    }
    close(newsockfd);
}

int main(int argc, char *argv[]){
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Check the commandline arguments
    if(argc != 2){
        std::cerr << "Port number not provided. Program terminated\n";
        exit(1);
    }

    // Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        std::cerr << "Error opening socket" << std::endl;
        exit(1);
    }
    int yes=1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        return 1;
    }
    // Populate the sockaddr_in structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = std::atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Bind the socket with the sockaddr_in structure
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        std::cerr << "Error binding" << std::endl;
        exit(1);
    }

    // Set the max number of concurrent connections
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // Accept a new connection
    signal(SIGCHLD, fireman);
    while (true){
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);

        if(fork() == 0){
            if(newsockfd < 0){
                std::cerr << "Error accepting new connections" << std::endl;
                exit(1);
            }
            handle_client_request(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }

    close(sockfd);
    return 0;   
}
