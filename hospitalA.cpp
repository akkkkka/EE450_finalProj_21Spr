// using g++ -o hospitalA -std=c++11 hospitalA.cpp
# include <iostream>
# include <sstream>
# include <iomanip>
# include <list>
# include <set>
# include <vector>
# include <unordered_map>
# include <cstring>
# include <fstream>
# include <queue>
# include <limits>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>
# include <netdb.h>
# include <arpa/inet.h>

using namespace std;

// define all of the constant variable
typedef pair<float, uint32_t> pi;
const float infinity = std::numeric_limits<float>::max();
const string INPUT = "map.txt";

const int PORT_UDP = 33400;
const int PORT_A = 30400;
const char* LOCAL_HOST = "127.0.0.1";  // running on local host

// global variable
float availability;
float occupation;

/***
 *      Graph Adjacency List Representation learned from:
 *      https://www.youtube.com/watch?v=drpdVQq5-mk&list=PLl4Y2XuUavmtTOvFcW3HfI1oQ3hsgkB3a&index=5
 ***/
class Graph
{
    unordered_map<int, set<pi> > l;
    set<uint32_t> all_nodes;

public:
    void addEdge(int x, float wt, int y) // bidirectional link
    {
        all_nodes.insert(x);
        all_nodes.insert(y);
        l[x].insert(make_pair(wt, y));
        l[y].insert(make_pair(wt, x));
    }

    void load_map()
    {
        ifstream information(INPUT);  // file pointer, at the beginning of a file
        int x; int y; float wt;
        while(information >> x >> y >> wt)
        {
            addEdge(x, wt, y);
        }
    }

    bool contains_node(int home)
    {
        return all_nodes.count(home);
    }

    /**
     *  return the shortest distance between source(home) and hospital, not the path.
     *  Dijkstra Algorithm pseudocode: https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm
     **/
    float dijkstra(uint32_t home, uint32_t hospital)
    {
        unordered_map<uint32_t, float> distance;
        set<uint32_t> all_nodes_copy = all_nodes;
        priority_queue<pi, vector<pi>, greater<pi> > pq;
        for (auto i : all_nodes)
        {
            if (i == hospital)
            {
                pq.push(make_pair(0.0, i));
                distance.insert(make_pair(i, 0.0));
            }
            else
            {
                pq.push(make_pair(infinity, i));
                distance.insert(make_pair(i, infinity));
            }
        }
        while (!all_nodes_copy.empty())
        {
            pi top = pq.top();
            pq.pop();  // already traversed, delete
            all_nodes_copy.erase(top.second);  // second is index, first is distance
            // adding neighbours to pq, update distance
            auto next_it = l.find(top.second);
            set<pi> next_edge = (*next_it).second;  // get all of the neighbours
            for (auto next_end : next_edge)
            {
                next_end.first += top.first;
                if (next_end.first < distance[next_end.second])
                {
                    distance[next_end.second] = next_end.first;
                    pq.push(make_pair(next_end.first, next_end.second));
                }
            }
        }
        return distance[home];
    }

    void printAdjList()  // print out the adjacent list, just for testing
    {
        for(auto p : l)
        {
            int start = p.first;
            set<pi> nbrs = p.second;
            cout << start << "->";
            for(auto nbr : nbrs)
            {
                int end = nbr.second;
                float weight = nbr.first;
                cout << "(" << weight << setprecision(14) << "," << end << ")" << ", ";
            }
            cout << endl;
        }
    }
};

/**
 *  UDP & TCP connection creation learned from:
 *  Beej Tutorial: http://www.beej.us/guide/bgnet/
 *  Socket Programming turtorial: https://www.youtube.com/watch?v=cNdlrbZSkyQ&list=RDCMUC4LMPKWdhfFlJrJ1BHmRhMQ&index=1
 **/
string UDP_recvfrom_scheduler(sockaddr_in scheduler, int sockfd)
{
    // create space for hospitals
    int scheduler_len = sizeof(scheduler);
    char buffer_sch[1024];
    memset(&buffer_sch, 0, 1024);

    // wait for message, decide which socket needs to recv
    int bytesIn = recvfrom(sockfd, buffer_sch, sizeof(buffer_sch), 0, 
                            (struct sockaddr *)&scheduler, 
                            (socklen_t *)&scheduler_len);
    if (bytesIn == -1)
    {
        cout << "Error receiving from scheduler. Quitting..." << endl;
    }
    return buffer_sch;
}


int main(int argc, char* argv[])
{
    if (argc != 4)  // location: argv[1], capacity: argv[2], occupancy: argv[3]
    {
        cout << "Missing command line argument for hospital A." << endl;
        return 1;
    }

    /**********************************       boot-up       ***************************************/
    cout << "Hospital A is up and running using UDP on port <" << PORT_A << ">​." << endl;
    
    /**********************       read map and construct the graph       **************************/
    Graph map;
    map.load_map();
    // map.printAdjList();

    /*******************       send initial status to scheduler via UDP       *********************/
    cout << "Hospital A has total capacity ​<" << argv[2] << ">​ and initial occupancy ​<" << argv[3] << ">​." << endl;

    // create space for server
    sockaddr_in scheduler;  // ipv4 add
    int scheduler_len = sizeof(scheduler);
    scheduler.sin_family = AF_INET;
    scheduler.sin_port = htons(PORT_UDP);  // host to network short, little -> big endian
    inet_pton(AF_INET, LOCAL_HOST, &scheduler.sin_addr);  // pass LOCAL_HOST in the buffer(pointer)

    // create a UDP socket for communication
    int sockHosA = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockHosA == -1)
    {
        cerr << "Hospital A can't create UDP socket! Quitting..." << endl;
        return -1;
    }
    // bind the socket to an IP / port for future communication with scheduler
    sockaddr_in hint;  // ipv4 add
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT_A);  // host to network short, little -> big endian
    inet_pton(AF_INET, LOCAL_HOST, &hint.sin_addr);  // pass in the buffer(pointer)
    if (::bind(sockHosA, (struct sockaddr *) &hint, sizeof(hint)) == -1)  // change the scope
    {
        cerr << "Hospital A can't bind UDP socket to IP/port! Quitting..." << endl;
        return -2;
    }

    // write things to hospital socket
    bool is_None = false;  // score is none
    bool ais_None = false;  // availability is none
    bool dis_None = false;  // distance is none
    occupation = stof(argv[3]);
    availability = (stof(argv[2]) - occupation) / (float)stoi(argv[2]);
    if (availability < 0 || availability > 1)
    {
        is_None = true;
        ais_None = true;
        availability = -2;  // if availability is None, have to calculate score: None, so dedicated "None" cases to -2 to be different with a=0
        cout << "Hospital A has capacity = ​<" << argv[2] << ">, occupation = <" << argv[3] << ">​, availability = <None>" << endl;
    }
    else
    {
        cout << "Hospital A has capacity = ​<" << argv[2] << ">, occupation = <" << argv[3] << ">​, availability = <" << availability << ">" << endl;
    }
    string a = to_string(availability);
    string port = to_string(PORT_A);
    string a_str = a + " " + port + " " + argv[2] + " " + argv[3];  // capacity: argv[2], occupancy: argv[3]
    
    // send initial state to scheduler
    int hosSend = sendto(sockHosA, a_str.c_str(), a_str.size()+1, 0, 
                    (const sockaddr *)&scheduler, scheduler_len);
    if (hosSend == -1)
    {
        cout << "Hospital A failed to send information! Quitting..." << endl;
        return -2;
    }

    /*************************       calculate score of client       ********************************/
    int hosSend_2;
    do
    {
        string buffer_sch = UDP_recvfrom_scheduler(scheduler, sockHosA);
        cout << "Hospital A has received input from client at location ​<" << buffer_sch << ">." << endl;

        // calculate score of the client
        uint32_t home = stoul(buffer_sch);  // all index can use uint32_t
        // if index not contains, d = None
        float d;
        string score;
        // calculate d
        if (!map.contains_node(home))
        {
            is_None = true;
            dis_None = true;
            d = NULL;
            cout << "Hospital A does not have the location ​<" << home << ">​ in map" << endl;
            string fail = string("None") + " " + string("None") + " " + argv[1];
            int hosSend_2 = sendto(sockHosA, fail.c_str(), fail.size()+1, 0, 
                    (const sockaddr *)&scheduler, sizeof(scheduler));
            if (hosSend_2 == -1)
            {
                cout << "Hospital A failed to send \"location not found\" to the Scheduler! Quitting..." << endl;
                return -2;
            }
            cout << "Hospital A has sent \"location not found\" to the Scheduler" << endl;
            continue;
        }
        else if (home == stoul(argv[1]))  // else == hospital index
        {
            is_None = true;
            dis_None = true;
            d = NULL;
        }
        else
        {
            d = map.dijkstra(home, stoul(argv[1]));  // first arg: home, second arg: hospital
        }
        
        // calculate score
        if (is_None)
        {
            score = "None";
        }
        else
        {
            score = to_string(1 / (d * (1.1 - availability)));
        }

        // send score to scheduler
        string c1;  // distance
        if (dis_None)
        {
            c1 = "None";
        }
        else
        {
            c1 = to_string(d);
        }
        cout << "Hospital A has found the shortest path to client, distance = <" << c1 << ">" << endl;
        cout << "Hospital A has the score = ​<" << score << ">" << endl;

        string c2 = argv[1];  // index
        string c = score + " " + c1 + " " + c2;  // score, distance, index, current availability
        hosSend_2 = sendto(sockHosA, c.c_str(), c.size()+1, 0, 
                    (const sockaddr *)&scheduler, sizeof(scheduler));
        if (hosSend_2 == -1)
        {
            cout << "Hospital A failed to send score! Quitting..." << endl;
            return -2;
        }
        cout << "Hospital A has sent score = ​<" << score << ">​ and distance = ​<" << c1 << ">​ to the Scheduler" << endl;

        /*************************      recv the scheduler's response      ***************************/
        string buffer_end = UDP_recvfrom_scheduler(scheduler, sockHosA);
        is_None = false;  // update all flags before the next query
        ais_None = false;
        dis_None = false;
        if (buffer_end == "1")
        {
            // update hospital's availability, capacity: argv[2], occupancy: argv[3]
            occupation = occupation + 1;
            availability = (stof(argv[2]) - occupation) / (float)stoi(argv[2]);
            if (availability <= 0)  // if the updated availability <= 0, then just keep it 0
            {
                availability = 0;
            }
            cout << "Hospital A has been assigned to a client, occupation is updated to ​<" << 
                    occupation << ">​, availability is updated to ​<" << availability << ">​" << endl;

            // tell the scheduler to update its hospital info
            string a = to_string(availability);
            string port = to_string(PORT_A);
            string a_str = a + " " + port + " " + argv[1];
            int hosSend = sendto(sockHosA, a_str.c_str(), a_str.size()+1, 0, 
                            (const sockaddr *)&scheduler, scheduler_len);
            if (hosSend == -1)
            {
                cout << "Hospital A failed to send updated information! Quitting..." << endl;
                return -2;
            }
        }
    } while(true);
    
    // close hospital's socket
    close(sockHosA);
    return 0;
}