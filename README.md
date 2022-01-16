# EE450-Socket-Project


Name: Yuhang Hu

USC NetID: 6062735400


## What I have done in the assignment
* In this project, I have implemented an application for assigning resourse based on client's location.
* Whenever client send its location to scheduler using command line arguments, scheduler can assign hospital to client through TCP connection. The assignment criteria is based on the scores and distance calculated by hospitals, which are sending through UDP connection between hospitals and scheduler. Hospitals' initial information are designated through command line arguments too.


## Code files:
    1. client.cpp
            - Get client's location through command line arguments
            - Create TCP socket with scheduler
            - Connect with scheduler
            - Send client location to scheduler
            - Get assigned hospital from scheduler
            - Once get back assignment result, terminate

    2. scheduler.cpp
            - Create UDP socket with hospital
            - Create TCP socket with client
            - Connect with client and hospitals
            - Receive the initial availability from each hospital
            - Receive location from client and forward to available hospitals
            - Receive calculation results from hospitals
            - Compare the most suitable hospital and forward the result to client and hospitals
            - Receive the updated information and update the availability of the assigned hospital
            - Keep listening until hit ctrl+C

    3. hospitalA/B/C.cpp
            - Get hospital's initial information through command line arguments
            - Read map.txt into memory
            - Create UDP socket with scheduler
            - Receive client's location from scheduler
            - Calculate the distance (using dijkstra algorithm) and score of client and forward the result to scheduler
            - Receive the allocation result from scheduler, update its availability and send back the updated information to scheduler
            - Keep listening until hit ctrl+C


## The format of all the messages exchanged
* In general, when one host sends or receives message, I put each data the destination host needs together in a string and separate them by a whitespace.
* Belows are messages sent and received from each host with parameters inside buffer:


    1. client:
            - Send location to scheduler: char* argv[1](client location index)
            - Returned result from scheduler: char 'I' for illegal location, char 'N' for none score, char 'A' or 'B' or 'C' for assigned hospital

    2. scheduler:
            - Receive location from client: char buffer_cli[CLIENT_BUF](client location index)
            - Receive initial information from hospital: string availability(if illegal is -2) port argv[2](capacity) argv[3](occupancy)
            - Send location to available hospital: char buffer_cli[CLIENT_BUF](client location index)
            - Receive result from hospital: string score distance index(hospital location index) availability
            - Send result to client: char 'I' for illegal location, char 'N' for no assignment, char 'A' or 'B' or 'C' for assigned hospital
            - Send result to hospital: string "0"(not chosen) or "1"(chosen)
            - Recv the updated information from the chosen hospital for updating scheduler's recorded hospital infomation: string availability port index

    3. hospitalA/B/C:
            - Send initial information to scheduler:  string availability(if illegal is -2) port argv[2](capacity) argv[3](occupancy)
            - Receive location from scheduler: string buffer_sch(client location index)
            - Send result to scheduler: string score distance index(hospital location index)
            - Receive result from scheduler: string "0"(not chosen) or "1"(chosen)
            - Send the updated information to scheduler: string availability port index


## Idiosyncrasy
        Based on my test results on Ubuntu, there was no idiosyncrasy found.


## Reused code(edited, not directly used):
[Beej Tutorial](http://www.beej.us/guide/bgnet/) & [Socket Programming turtorial](https://www.youtube.com/watch?v=cNdlrbZSkyQ&list=RDCMUC4LMPKWdhfFlJrJ1BHmRhMQ&index=1)
* Create sockets (TCP / UDP);
* Bind a socket;
* Send & receive data;
        

[Graph Adjacency List Representation](https://www.youtube.com/watch?v=drpdVQq5-mk&list=PLl4Y2XuUavmtTOvFcW3HfI1oQ3hsgkB3a&index=5)
* Create adjacency list for map.txt in hospitalA/B/C.cpp
        

[Dijkstra Algorithm pseudocode](https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm)
* Search for shortest path in hospitalA/B/C.cpp
