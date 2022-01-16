// using g++ -o client -std=c++11 client.cpp
# include <iostream>
# include <cstring>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <sstream>
# include <vector>

using namespace std;
const int PORT_TCP = 34400;
const char* LOCAL_HOST = "127.0.0.1";

/**
 *  UDP & TCP connection creation learned from:
 *  1. Beej Tutorial: http://www.beej.us/guide/bgnet/
 *  2. Socket Programming turtorial: https://www.youtube.com/watch?v=cNdlrbZSkyQ&list=RDCMUC4LMPKWdhfFlJrJ1BHmRhMQ&index=1
 **/
int main(int argc, char* argv[])
{
    /**********************************       boot-up       ***************************************/
    cout << "The client is up and running." << endl;
    if (argc != 2) // location: argv[1]
    {
        cout << "Missing command line argument for client." << endl;
        return -1;
    }

    // create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        cerr << "Client can't create socket! Quitting..." << endl;
        return -2;
    }

    // create a hint for client, connect to the scheduler
    sockaddr_in hint;  // ipv4 add
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT_TCP);
    inet_pton(AF_INET, LOCAL_HOST, &hint.sin_addr);
    socklen_t hint_len = sizeof(hint);

    // connect to scheduler, id doesn't matter
    int connResult = connect(sockfd, (struct sockaddr *)&hint, hint_len);
    if (connResult == -1)
    {
        cerr << "Client had a connection issue when connecting to scheduler. Quitting..." << endl;
        return -3;
    }

    // Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure
    int getsock_check = getsockname(sockfd, (struct sockaddr *)&hint, &hint_len);
    if (getsock_check== -1)
    {
        perror("getsockname");
        return -3;
    }

    char buffer[4096];
    memset(buffer, 0, 4096);
    int bytesRecv;
    do
    {
        /*************************       send location to scheduler       *****************************/
        int sendResult = send(sockfd, argv[1], sizeof(argv[1])+1, 0);  // string has a tailing \0
        cout << "The client has sent query to Scheduler using TCP: client location ​<" << argv[1] << ">" << endl;
        if (sendResult == -1)  // check if failed
        {
            cout << "Client could not sent to Scheduler! Quitting..." << endl;
            return -4;
        }
        
        /**********************       recv response from scheduler      **************************/
        bytesRecv = recv(sockfd, buffer, 4096, 0);
        if (bytesRecv == -1)
        {
            cerr << "Client had a connection issue when receiving data from Scheduler." << endl;
            break;
        }
        if (bytesRecv == 0)
        {
            cout << "Client disconnected to the Scheduler." << endl;
            break;
        }
    }while (!bytesRecv);  // once received response, break


    /****************************       print out response      *******************************/
    if (buffer[0] == 'N')
    {
        cout << "The client has received results from the Scheduler: assigned to Hospital ​<None>" << endl;        
        cout << "Score = None, No assignment (location is legal, but all score are None because of no availability)" << endl;
    }
    else if (buffer[0] == 'I')
    {
        cout << "The client has received results from the Scheduler: assigned to Hospital ​<None>" << endl;        
        cout << "Location ​<" << argv[1] << ">​ not found (location is illegal)." << endl;
        // cout << "(1. location not in map. 2. client and one of the hospitals in the same location.)" << endl;
    }
    else
    {
        cout << "The client has received results from the Scheduler: assigned to Hospital ​<" << buffer << ">" << endl;
    }

    close(sockfd);
    return 0;
}