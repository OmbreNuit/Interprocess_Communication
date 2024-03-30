#include <unistd.h>
#include <iostream>
#include <string>   //I am using the client.cpp code provided by Dr.Rincon on Canvas as the basis for my program
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <string.h>
#include <vector>
#include <sstream>
#include <pthread.h>
#include <algorithm>
#include <cmath>
#include <iomanip>

struct ThreadData {
    std::string hostname;
    int port_number;
    std::string inputString;
    std::string* outStr;
    int num_threads;
};

// formating the output function
std::string create_output(const std::vector<double>& entropyValues, std::string inputStr, int iterations) {
    std::stringstream output_sstream;
    std::stringstream input_sstream(inputStr);
   
    std::vector<std::pair<char, int>> taskInfo;
    char task;
    int count;

    while (input_sstream >> task >> count) {
        taskInfo.push_back(std::make_pair(task, count));
    }
    
    output_sstream << "CPU " << iterations << std::endl;

    output_sstream << "Task scheduling information: ";
    for(int i = 0; i < taskInfo.size()-1; i++){
        output_sstream << taskInfo[i].first << "(" << taskInfo[i].second << "), ";
    }
    output_sstream << taskInfo[taskInfo.size()-1].first << "(" << taskInfo[taskInfo.size()-1].second << ")";
    output_sstream << std::endl;

    output_sstream << "Entropy for CPU " << iterations << std::endl;
    for (double entropy : entropyValues) {
        output_sstream << std::fixed << std::setprecision(2) << entropy << " ";
    }
    output_sstream << std::endl;

    return output_sstream.str();
}


void *child_thread(void *data){
    
    // Create a socket communication with the server program
    ThreadData *thread_data = (ThreadData *)data;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int portno = thread_data->port_number;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    std::string buffer = thread_data->inputString;

    if(sockfd < 0){
        std::cerr << "ERROR opening socket" << std::endl;
        exit(1);
    }
    server = gethostbyname(thread_data->hostname.c_str());
    if(server == NULL){
        std::cerr << "ERROR, no such host" << std::endl;
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        std::cerr << "ERROR connecting" << std::endl;
        exit(1);
    }
    
    //Send code to the server program
    int msgSize = buffer.size();
    if(write(sockfd, &msgSize, sizeof(int)) < 0){
        std::cerr << "ERROR writing size to socket" << std::endl;
        exit(1);
    }
    if(write(sockfd, buffer.c_str(), msgSize) < 0){
        std::cerr << "ERROR writing string to socket" << std::endl; 
        exit(1);
    }
    
    // Read the code from the server program
    int entropies_size;
    if(read(sockfd, &entropies_size, sizeof(int)) < 0){
        std::cerr <<"Error reading size from socket" <<std::endl;
        exit(1);
    }

    std::vector<double> entropies(entropies_size / sizeof(double));
    if (read(sockfd, entropies.data(), entropies_size) < 0){
        std::cerr << "ERROR reading entropies from socket" << std::endl;
        exit(1);
    }

    // calling function create_output and store it into memory loction shared by main thread
    *(thread_data->outStr) = create_output(entropies, buffer, thread_data->num_threads);

    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[]){
    // check if all arguments are passed
    if(argc != 3){
        std::cerr << "Usage: " << argv[0] << " hostname port_no < input_filename" << std::endl;
        exit(1);
    }

    std::string hostname = argv[1];
    int port_number = std::stoi(argv[2]);

    std::string input;
    std::vector<std::string> inputStr;
    int nStrings = 0;
    
    while(std::getline(std::cin, input)){
        inputStr.push_back(input);
        nStrings++;
    }
    
    // store information for child threads
    std::vector<pthread_t> threads(nStrings);
    std::vector<ThreadData> thread_data(nStrings);
    std::vector<std::string> outputString(nStrings);
    for(int i =0; i < nStrings; i++){
        thread_data[i].hostname = hostname;
        thread_data[i].port_number = port_number;
        thread_data[i].inputString = inputStr[i];
        thread_data[i].outStr = &outputString[i];
        thread_data[i].num_threads = i + 1;
    }
    
    // Create child threads
    for(int i =0; i < nStrings; i++){
        pthread_create(&threads[i], NULL, child_thread, (void *)&thread_data[i]);
    }
    // Join child threads
    for(int i = 0; i < nStrings; i++){
        pthread_join(threads[i], NULL);
    }
    // output from memory location accessible by the main thread
    for(int i =0; i < nStrings; i++){
        std::cout << *(thread_data[i].outStr) << std::endl;
    }

    return 0;
}
