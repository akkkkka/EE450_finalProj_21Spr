// using g++ -o scheduler -std=c++11 scheduler.cpp
# include <iostream>
# include <cstring>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <sstream>
# include <vector>
# include <list>
# include <map>
# include <algorithm>
# include <set>

using namespace std;
const int PORT_UDP = 33400;
const int PORT_TCP = 34400;
const int PORT_A = 30400;
const int PORT_B = 31400;
const int PORT_C = 32400;
const char* LOCAL_HOST = "127.0.0.1";
const int CLIENT_BUF = 10;

/**
 *  create TCP/UDP sockets for client, also bind address and listen to the channel.
 *  UDP & TCP connection creation learned from:
 *  1. Beej Tutorial: http://www.beej.us/guide/bgnet/
 *  2. Socket Programming turtorial: https://www.youtube.com/watch?v=cNdlrbZSkyQ&list=RDCMUC4LMPKWdhfFlJrJ1BHmRhMQ&index=1
 **/
int UDP_socket()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        cerr << "Scheduler can't create UDP socket! Quitting..." << endl;
        exit(1);
    }

    // bind the socket to an IP / port
    sockaddr_in hint;  // ipv4 add
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT_UDP);  // host to network short, little -> big endian
    inet_pton(AF_INET, LOCAL_HOST, &hint.sin_addr);  // pass in the buffer(pointer)
    if (::bind(sockfd, (struct sockaddr *) &hint, sizeof(hint)) == -1)  // change the scope
    {
        cerr << "Scheduler can't bind UDP socket to IP/port! Quitting..." << endl;
        exit(2);
    }
    return sockfd;
}

int TCP_parent_socket()
{
    int parent;
    parent = socket(AF_INET, SOCK_STREAM, 0);
    if (parent == -1)
    {
        cerr << "Scheduler can't create TCP parent socket! Quitting..." << endl;
        exit(1);
    }

    // bind the parent socket to an IP / port
    sockaddr_in hint;  // ipv4 add, not sockaddr, have to cast
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT_TCP);  // host to network short
    inet_pton(AF_INET, LOCAL_HOST, &hint.sin_addr);  // pass in the buffer(pointer)
    if (::bind(parent, (struct sockaddr *)&hint, sizeof(hint)) == -1)
    {
        cerr << "Scheduler can't bind TCP parent socket to IP/port! Quitting..." << endl;
        exit(2);
    }

    // parent socket listen to the channel
    if (listen(parent, SOMAXCONN) == -1)  // maximum number of connections we could have
    {
        cerr << "Scheduler's TCP parent can't listen! Quitting..." << endl;
        exit(3);
    }
    return parent;
}

int TCP_accept_client(int parent)
{
    sockaddr_in client;
    socklen_t clientSize =  sizeof(client);
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    int child = accept(parent, (struct sockaddr *)&client, (socklen_t *)&clientSize);
    if (child == -1)
    {
        cerr << "Client can't connect to Scheduler! Quitting..." << endl;
        exit(4);
    }
    memset(host, 0, NI_MAXHOST);
    memset(service, 0, NI_MAXSERV);

    // get the name of the computer and display it
    int result = getnameinfo((struct sockaddr *)&client, sizeof(client), 
                                host, NI_MAXHOST, service, NI_MAXSERV, 0);
    if (!result)  // if fails, result = 0, do things manually
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);  // client.sin_addr pass in the host buffer(string)
    }
    return child;
}

string UDP_recvfrom_hospital(sockaddr_in hospital, int sockfd)
{
    // create space for hospitals
    int hospital_len = sizeof(hospital);
    char buffer_hos[1024];
    memset(&buffer_hos, 0, 1024);

    // wait for message, decide which socket need to recv
    int bytesIn = recvfrom(sockfd, buffer_hos, sizeof(buffer_hos), 0, 
                            (struct sockaddr *)&hospital, 
                            (socklen_t *)&hospital_len);
    if (bytesIn == -1)
    {
        cout << "Error receiving from hospital. Quitting..." << endl;
    }
    return buffer_hos;
}

/***
 *  compare all scores and distances of hospitals,
 *  compare for the highest score, if scores are tied, choose the shorter distance hospital.
 ***/
bool myComparison(const pair<uint32_t, pair<string, string> >&a, const pair<uint32_t, pair<string, string> > &b)
{
    float afirst, bfirst;
    float asecond, bsecond;
    if (a.second.first == "None") {afirst = 0.0;}
    else {afirst = stof(a.second.first);}
    if (b.second.first == "None") {bfirst = 0.0;}
    else {bfirst = stof(b.second.first);}
    if (a.second.second == "None") {asecond = 0.0;}
    else {asecond = stof(a.second.second);}
    if (b.second.second == "None") {bsecond = 0.0;}
    else {bsecond = stof(b.second.second);}

    if (afirst > bfirst) {return true;}
    else if (afirst == bfirst) {if (asecond < bsecond){return true;}}
    return false;
}

/***
 *  decide which hospital the message is from based on the port number,
 *  return a string of the given hospital's name.
 ***/
string hospital_decision(int portnum)
{
    portnum = ntohs(portnum);
    if (portnum == PORT_A) {return "A";}
    else if (portnum == PORT_B) {return "B";}
    else {return "C";}
}

void respond_cli_hos(int child, int sockfd, string h, map<uint32_t, sockaddr_in> hosIndex, bool still_avail)
{
    // response to client the hospital, or None
    int response_cli = send(child, h.c_str(), sizeof(h.c_str()) + 1, 0);  // echo
    if (response_cli == -1)  // check if failed
    {
        cout << "Scheduler could not respond to Client! Quitting..." << endl;
        exit(3);
    }
    cout << "The Scheduler has sent the result to client using TCP over port <" << PORT_TCP << ">" << endl;
    
    // sending results to the assigned hospital
    if (still_avail)
    {
        for (auto x : hosIndex)
        {
            string re = "0";
            int response_hos = sendto(sockfd, re.c_str(), re.size()+1, 0, 
                (const sockaddr *)&x.second, sizeof(x.second));
            if (response_hos == -1)
            {
                cout << "Scheduler failed to send error result to hospital! Quitting..." << endl;
                exit(4);
            }
        }
    }
}

int main()
{
    /**********************************       boot-up       ***************************************/
    /******************       create sockets for hospitals and clients       **********************/
    int sockfd = UDP_socket();
    int parent = TCP_parent_socket();
    int child;
    cout << "The Scheduler is up and running." << endl;

    // create space for client
    char buffer_cli[CLIENT_BUF];
    vector<pair<sockaddr_in, float> > hosInfo;  // for saving hospitals' availability
    vector<pair<uint32_t, pair<string, string> > > hosScore;
    map<uint32_t, sockaddr_in> hosIndex;
    set<uint32_t> notAvail;

    ssize_t response_cli;  // responde to client
    ssize_t response_hos;

    // while loop for waiting the client and hospitals' message
    do
    {
        /******************************      HOSPITALS    ************************************/
        /*****************      recv hospitals' innitial initial status    *******************/
        sockaddr_in hospital;
        hospital.sin_family = AF_INET;
        inet_pton(AF_INET, LOCAL_HOST, &hospital.sin_addr);
        string buffer_hos = UDP_recvfrom_hospital(hospital, sockfd);
        istringstream iss;
        vector<float> data;
        float it;
        iss.str (buffer_hos);
        for (int n=0; n<4; n++)
        {
            iss >> it;
            data.push_back(it);
        }  // a + port + capacity + occupancy
        hospital.sin_port = htons((int)data[1]);
        // save the initial capacity and occupancy of each hospital
        hosInfo.push_back(make_pair(hospital, data[0]));
        cout << "The Scheduler has received information from Hospital " << hospital_decision(htons((int)data[1])) << 
            ": total capacity is ​<" << data[2] << ">​ and initial occupancy is ​<" << data[3] << ">" << endl;
    } while (hosInfo.size() < 3);


    do
    {
        /******************************      CLIENT    ************************************/
        child = TCP_accept_client(parent);
        // clear the buffer for client
        memset(buffer_cli, 0, CLIENT_BUF);
        // wait for a message
        int bytesTCP = recv(child, buffer_cli, CLIENT_BUF, 0);
        if (bytesTCP == -1)
        {
            cerr << "Scheduler had a connection issue when receiving data from client." << endl;
            continue;
        }
        if (bytesTCP == 0)
        {
            cout << "Client disconnected to the Scheduler." << endl;
            continue;
        }
        // display the message
        cout << "The Scheduler has received client at location ​<" << string(buffer_cli, 0, bytesTCP) << ">​ from the client using TCP over port <" << PORT_TCP << ">" << endl;
        

        /******************************      HOSPITALS    ************************************/
        // send to available hospital and get responses: score, d/ None
        bool dis_none = false;
        bool stillAvail = false;
        if (buffer_cli)
        {
            for (int i = 0; i < 3; i++)
            {
                if (hosInfo[i].second > 0 || hosInfo[i].second == -2)  // only send to available hospital or None availability hospital(as stated in pdf)
                {
                    stillAvail = true;
                    cout << "The Scheduler has sent client location to Hospital " << hospital_decision(hosInfo[i].first.sin_port) << " using UDP over port ​<" << PORT_UDP << ">" << endl;
                    // create space for server
                    sockaddr_in hospital_2;  // ipv4 add
                    hospital_2.sin_family = AF_INET;
                    hospital_2.sin_port = htons(hosInfo[i].first.sin_port);  // host to network short, little -> big endian
                    inet_pton(AF_INET, LOCAL_HOST, &hosInfo[i].first.sin_addr);  // pass LOCAL_HOST in the buffer(pointer)

                    // use the previous UDP socket for communication
                    // write things to socket
                    string s = string(buffer_cli, 0, bytesTCP);
                    int scheSend = sendto(sockfd, s.c_str(), s.size()+1, 0, 
                        (const sockaddr *)&hosInfo[i].first, sizeof(hosInfo[i].first));
                    if (scheSend == -1)
                    {
                        cout << "Scheduler failed to send information! Quitting..." << endl;
                        return -2;
                    }

                    string buffer_hos2 = UDP_recvfrom_hospital(hospital_2, sockfd);  // score, d, index
                    istringstream iss;
                    vector<string> data;
                    string it;
                    iss.str (buffer_hos2);
                    for (int n=0; n<3; n++)
                    {
                        iss >> it;
                        data.push_back(it);
                    }  // data[0]: score(None,float), data[1]: d(None,float), data[2]: index(uint32_t)
                    hosIndex[stoul(data[2])] = hosInfo[i].first;

                    int si = hosScore.size();
                    bool find = false;
                    for (int i = 0; i < si; i++)
                    {
                        if (hosScore[i].first == stoul(data[2]))
                        {
                            find = true;
                            hosScore[i].second = make_pair(data[0], data[1]);
                            // cout << "!!!!change " << hosScore[i].second.first << " " << hosScore[i].second.second << endl;
                        }
                    }
                    if (!find)
                    {
                        // vector<pair<uint32_t, pair<string, string> > > hosScore;
                        hosScore.push_back(make_pair(stoul(data[2]), make_pair(data[0], data[1])));
                        // cout << "!!!!added " << hosScore[i].second.first << " " << hosScore[i].second.second << endl;
                    }
                    

                    if (data[1] == "None")
                    {
                        dis_none = true;
                    }
                    cout << "The Scheduler has received map information from Hospital " << hospital_decision(hosInfo[i].first.sin_port) << ", the score = ​<" << data[0] << ">​ and the distance = ​<" << data[1] << ">." << endl;
                }
            }
            // deal with None situation, myComparison will assign None to 0
            if (stillAvail)
            {
                sort(hosScore.begin(), hosScore.end(), myComparison);
            }

            /******************************      RESPONSE      ************************************/
            // response to client and hospital the error message
            if (dis_none)  // client location is illegal(not in map / at hospital)
            {
                string h = "I";
                respond_cli_hos(child, sockfd, h, hosIndex, stillAvail);
                continue;
            }
            else if (hosScore[0].second.first == "None" || !stillAvail)  // d legal, all scores are None
            {
                string h = "N";
                respond_cli_hos(child, sockfd, h, hosIndex, stillAvail);
                continue;
            }
            uint32_t chosenInd = hosScore[0].first;
            sockaddr_in chosenHos = hosIndex.find(chosenInd)->second;
            string h = hospital_decision(chosenHos.sin_port);
            cout << "The Scheduler has assigned Hospital <" << h << ">​ to the client" << endl;
            
            // respond to CLIENT the assigned hospital
            response_cli = send(child, h.c_str(), sizeof(h.c_str()) + 1, 0);
            if (response_cli == -1)  // check if failed
            {
                cout << "Scheduler could not respond to Client! Quitting..." << endl;
                return -4;
            }
            cout << "The Scheduler has sent the result to client using TCP over port <" << PORT_TCP << ">" << endl;

            // respond to HOSPITALS the result
            for (auto x : hosIndex)
            {
                if (x.first == chosenInd)
                {
                    string re = "1";  // send "1" to the chosen hospital
                    sockaddr_in hospital_3;  // ipv4 add
                    hospital_3.sin_family = AF_INET;
                    hospital_3.sin_port = htons(x.second.sin_port);  // host to network short, little -> big endian
                    inet_pton(AF_INET, LOCAL_HOST, &x.second.sin_addr);

                    response_hos = sendto(sockfd, re.c_str(), re.size()+1, 0, 
                        (const sockaddr *)&x.second, sizeof(x.second));
                    if (response_hos == -1)
                    {
                        cout << "Scheduler failed to send decision to the chosen hospital! Quitting..." << endl;
                        return -2;
                    }

                    // get response from the chosen hospital and update hosInfo
                    string buffer_hos3 = UDP_recvfrom_hospital(hospital_3, sockfd);  // score, d, index
                    istringstream iss;
                    vector<string> data;
                    string it;
                    iss.str (buffer_hos3);
                    for (int n=0; n<3; n++)
                    {
                        iss >> it;
                        data.push_back(it);
                    }  // a + port + index
                    hospital_3.sin_port = htons(stoi(data[1]));
                    for (int i = 0; i < 3; i++)
                    {
                        // cout << "[hosInfo] " << ntohs(hosInfo[i].first.sin_port) << " " << ntohs(hospital_3.sin_port) << endl;
                        if (hosInfo[i].first.sin_port == hospital_3.sin_port)
                        {
                            hosInfo[i].second = stof(data[0]);
                            // cout << "[update in scheduler] " << ntohs(hosInfo[i].first.sin_port) << " " << hosInfo[i].second << endl;
                            if (hosInfo[i].second == 0)
                            {
                                int si = hosScore.size();
                                for (int i = 0; i < si; i++)
                                {
                                    if (hosScore[i].first == stoul(data[2]))
                                    {
                                        hosScore[i].second = make_pair(to_string(0), to_string(INFINITY));  // score + d
                                        notAvail.insert(stoul(data[2]));
                                        // cout << "[hosScore] " << hosScore[i].second.first << " " << hosScore[i].second.second << endl;
                                    }
                                }
                            }
                        }
                    }
                    
                }
                else
                {
                    if (notAvail.count(x.first) == 0)  // if hospital is already not available, not send
                    {
                        // cout << "[send '0' to] " << x.first << endl;
                        string re = "0";  // send "0" to the other hospitals
                        response_hos = sendto(sockfd, re.c_str(), re.size()+1, 0, 
                            (const sockaddr *)&x.second, sizeof(x.second));
                        if (response_hos == -1)
                        {
                            // cout << "Scheduler failed to send decision to the unchosen hospital! Quitting..." << endl;
                            return -2;
                        }
                    }
                }
            }
            cout << "The Scheduler has sent the result to Hospital ​<" << h << ">​ using UDP over port ​<" << PORT_UDP << ">" << endl;
        }
    } while (true);

    // close all sockets
    close(child);  // TCP child socket for client 
    close(parent);  // TCP parent socket for client 
    close(sockfd);  // UDP socket for hospitals
    return 0;
}