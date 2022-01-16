all: scheduler.cpp hospitalA.cpp hospitalB.cpp hospitalC.cpp client.cpp
	g++ -o scheduler -std=c++11 scheduler.cpp

	g++ -o hospitalA -std=c++11 hospitalA.cpp

	g++ -o hospitalB -std=c++11 hospitalB.cpp

	g++ -o hospitalC -std=c++11 hospitalC.cpp
	
	g++ -o client -std=c++11 client.cpp

clean:
	rm ./scheduler
	rm ./hospitalA
	rm ./hospitalB
	rm ./hospitalC
	rm ./client

