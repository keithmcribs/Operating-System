#include <iostream>
//#include <sys/types.h>
//#include <unistd.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include <queue>
#include <iomanip> 
using namespace std;

/*
	Assignment 1: Process Scheduling
	COSC 3360 - Fundamentals of Operating Systems
	University of Houston
	Yikchun Ng
	1558087
*/

/* 
	<Terminal Instruction>
	compile: g++ -std=c++11 hw1.cpp
	run: ./<output>.out < <input>.txt
*/

// Variables
int NCORES = 0; 			 // total number of cores 
int NPROCESS = 0;			 // number of processes remaining
int NCOMPLETED = 0;  		 // number of processes completed/terminated 
int CLOCK = 0;				 // Current time tracker
double CORETIME = 0; 		 // total core request time

struct Request				 // request structure
{
	int type;				 // 1:CORE, 2:SSD, 3:INPUT
	int parameter;			 // time requested
};

struct Process 				 // process structure
{
	int ID;					 // process id
	int firstLine;			 // indicate the line of the new process in the inputs
	double arrivalTime;		 // time of a new process ariives
	double eventTime;		 // process deadline tracker	
	int state = 0;			 // 0:READY, 1:RUNNING, 2:BLOCKED, 3:TERMINATED
	vector<Request> requestQueue; // list to keep track of all the requests. Process will be terminated if empty.
};
vector<Process> PROCESSES;	 // vector that contains all existing processes
vector<Process> priorityQueue; // vector that stores and sorts processes by deadline
queue<Process> readyQueue;   // queue that stores all waiting processes

struct Resource				 //	struture of a core
{
	int ID;					 // core id for multi-core identification
	Process currentProcess;	 // process currerntly being handled
	int nextProcessID;	   	 // id of the next process to be handled
	double releaseTime = -1; // completion time of the current process in this core
	bool available = true;	 // status of the core
};
vector<Resource> CORES;	 	 // Vector that contains all the cores

struct SolidStateDrive       // structure of SSD
{
	int count = 0;			 // total number of accesses to SSD
	double accessTime = 0;	 // total SSD time accessed/requested
	int releaseTime = -1;	 // completion time of the current process in SSD
	Process currentProcess;	 // process currerntly being handled
	bool available = true;	 // status of the SSD
};
SolidStateDrive SSD;		 // SSD constant
queue<Process> SSDQueue;	 // queue that stores all waiting processes

struct InputDevice			 // structure of user
{
	int releaseTime = -1;	 // completion time of the current process by USER
	Process currentProcess;	 // process currerntly being handled
	bool available = true;	 // status of user
};
InputDevice USER;			 // simulate user
queue<Process> inputQueue;	 // queue that stores all processes waiting for input

// Functions
void SimulationScheduler();
void CoreRequestRoutine(Process process, int crt);		// Core request routine
void CoreCompletionRoutine(Resource core);				// Core completion routine
void SSDRequestRoutine(Process process, int srt);		// SSD request routine
void SSDCompletionRoutine();							// SSD completion routine
void InputRequestRoutine(Process process, int irt);		// Input request routine
void InputCompletionRoutine();							// Input completion routine
void ProcessStateCheck(Process process);				// Print the state of processes when a process arrives or terminates
void ChangeProcessState(Process process, int num);		// Change the state of a process (0:READY, 1:RUNNING, 2:BLOCKED, 3:TERMINATED)
void SortProcessesbyDeadline(); 						// Sort processes in priority queue using selection sort

// Implementations
int main()
{
	// Start timer
	//auto start = timer::now();
	
	// I/O redirection
	int lineCount = 0;
	string *strArr = new string[10000];
	int *intArr = new int[10000];
	while(!cin.eof()){
		cin >> strArr[lineCount];
		cin >> intArr[lineCount];
		lineCount++;
	}
	string readStr[lineCount];
	int readNum[lineCount];
	for(int i=0; i < lineCount-1; i++){
		readStr[i] = strArr[i];
		readNum[i] = intArr[i];
	}
	delete[] strArr;
	delete[] intArr;
	
	
	// Read inputs and store processes and requests based on the key words
	Process currentProcess;
	for(int j=0; j < lineCount; j++){
		string inputStr = readStr[j];
		int inputNum = readNum[j];
		//cout << inputStr << " " << inputNum << endl; // removable
		if(inputStr == "NCORES"){
			for(int i=0; i < inputNum; i++){	
				Resource newCore;
				NCORES++;
				newCore.ID = i;
				CORES.push_back(newCore);
			}
		}
		
		if(inputStr == "NEW"){
			Process newProcess;
			NPROCESS++;
			newProcess.ID = NPROCESS - 1;
			newProcess.arrivalTime = inputNum;
			newProcess.eventTime = inputNum;
			newProcess.firstLine = j;
			PROCESSES.push_back(newProcess);
			currentProcess = newProcess;
			priorityQueue.push_back(currentProcess);
		}
		
		if(inputStr == "CORE"){
			Request newRequest;
			newRequest.type = 1;
			newRequest.parameter = inputNum;
			currentProcess.requestQueue.push_back(newRequest);
			PROCESSES.back() = currentProcess;
		}
		
		if(inputStr == "SSD"){
			Request newRequest;
			newRequest.type = 2;
			newRequest.parameter = inputNum;
			currentProcess.requestQueue.push_back(newRequest);
			PROCESSES.back() = currentProcess;
		}
		
				
		if(inputStr == "INPUT"){
			Request newRequest;
			newRequest.type = 3;
			newRequest.parameter = inputNum;
			currentProcess.requestQueue.push_back(newRequest);
			PROCESSES.back() = currentProcess;
		}
		
		if(priorityQueue.size() > 0){
			priorityQueue.pop_back();
			priorityQueue.push_back(currentProcess);
		}
	}
	
	SimulationScheduler();

	// Summary
	double avgSSDtime = SSD.accessTime/SSD.count;
	double coreUtilization = (CORETIME/CLOCK)*100;
	double SSDUtilization = (SSD.accessTime/CLOCK)*100; 
	
	cout << "SUMMARY:" << endl;
	cout << "Number of procsses that completed: " << NCOMPLETED << endl;
	cout << "Total number of SSD accesses: " << SSD.count << endl;
	cout << "Average SSD access time: "  << setprecision(2) << fixed << avgSSDtime << " ms" << endl; 
	cout << "Total elapsed time: " << CLOCK << " ms" << endl; //	End clock and display the compute time
	cout << "Core utilization: " << setprecision(2) << fixed << coreUtilization << " percent" << endl; 
	cout << "SSD utilization: "  << setprecision(2) << fixed << SSDUtilization << " percent" << endl; 
	
	return 0;
}

/* 
	SimulationScheduler():
	Sort priorityQueue, process the next event in the queue then remove until the queue is empty
	Update clock based on time of the event. If clock = time of event, trigger the event routine
	Process next request in the requestQueue in a process until it is empty, then terminate the process 
*/
void SimulationScheduler()
{
	while(!priorityQueue.empty()){
		if(priorityQueue.size() > 1){
			SortProcessesbyDeadline();
		}
		
		Process nextProcess = priorityQueue.front();
		CLOCK = nextProcess.eventTime;
		if(nextProcess.eventTime == nextProcess.arrivalTime){
			cout << "-- ARRIVAL event for process " << nextProcess.ID << " at time " << nextProcess.eventTime << " ms" << endl;
			cout << "\nProcess " << nextProcess.ID << " starts at time " << nextProcess.eventTime << " ms" << endl;
			for(int i=0; i < nextProcess.ID; i++){    // a simpler version of process state check
				string state;		
				if(PROCESSES[i].state == 0){
					state = "READY";
				}
				if(PROCESSES[i].state == 1){
					state = "RUNNING";
				}
				if(PROCESSES[i].state == 2){
					state = "BLOCKED";
				}
				if(PROCESSES[i].state == 3){
					state = "TERMINATED";	
				}
				cout << "Process " << PROCESSES[i].ID << " is " << state << endl;
			}
			cout << endl;
		}
		
		for(int i=0; i < CORES.size(); i++){
			if(nextProcess.eventTime == CORES[i].releaseTime){
				cout << "-- CORE completion event for process " << nextProcess.ID << " at time " << CORES[i].releaseTime << " ms" << endl;
				CoreCompletionRoutine(CORES[i]);
			}
		}
		
		if(nextProcess.eventTime == SSD.releaseTime){
			cout << "-- SSD completion event for process " << nextProcess.ID << " at time " << SSD.releaseTime << " ms" << endl;
			SSDCompletionRoutine();
		}
		
		if(nextProcess.eventTime == USER.releaseTime){
			cout << "-- INPUT completion event for process " << nextProcess.ID << " at time " << USER.releaseTime << " ms" << endl;
			InputCompletionRoutine();
		}
		
		if(!(nextProcess.requestQueue.empty())){
			Request temp = nextProcess.requestQueue.front();
			nextProcess.requestQueue.erase(nextProcess.requestQueue.begin());
			
			if(temp.type == 1){
				CoreRequestRoutine(nextProcess, temp.parameter);
			}
			if(temp.type == 2){
				SSDRequestRoutine(nextProcess, temp.parameter);
			}
			if(temp.type == 3){
				InputRequestRoutine(nextProcess, temp.parameter);
			}
		}else{
			cout << "\nProcess " << nextProcess.ID << " terminates at time " << CLOCK << " ms" << endl;
			for(int i=0; i < PROCESSES.size(); i++){
				if(PROCESSES[i].ID == nextProcess.ID){
					PROCESSES[i].state = 3;
					ProcessStateCheck(PROCESSES[i]);
					PROCESSES.erase(PROCESSES.begin()+i);
					NCOMPLETED++;
					break;
				}
			}
			cout << endl;
		}
		priorityQueue.erase(priorityQueue.begin());
	}
}

// Core request routine / availablity check
void CoreRequestRoutine(Process process, int crt)
{
	cout << "-- Process " << process.ID << " requests a core at time " << process.eventTime << " ms for " << crt << " ms" << endl;
	int i = 0;
	bool found;
	while(i < NCORES){
		if(CORES[i].available == true){
			found = true;
			break;
		}else{
			found = false;
		}
		i++;
	}
	if(found == true){
		CORES[i].available = false;  // mark core busy
		ChangeProcessState(process, 1);
		CORETIME += crt;
		CORES[i].releaseTime = CLOCK + crt;
		process.eventTime = CLOCK + crt;
		priorityQueue.push_back(process);
		cout << "-- Process " << process.ID << " will release a core at time " << process.eventTime << " ms" << endl;
	}else{
		cout << "-- Process " << process.ID << " must wait for a core " << endl;
		process.eventTime = crt;
		readyQueue.push(process); // enter process in ready queue
		ChangeProcessState(process, 0);
		cout << "-- Ready queue	now contains " << readyQueue.size() << " process(es) waiting for a core" << endl;
	}
	i = 0;
}

// Core request completion routine
void CoreCompletionRoutine(Resource core)
{
	int i = core.ID;
	if(readyQueue.empty()){
		CORES[i].available = true;
	}else{
		Process nextProcess = readyQueue.front();
		CORES[i].available = false;  // mark core busy
		CORES[i].currentProcess = nextProcess;
		ChangeProcessState(nextProcess, 1);
		double crt = nextProcess.eventTime;
		CORETIME += crt;
		CORES[i].releaseTime = CLOCK + crt;
		CORES[i].currentProcess.eventTime = CLOCK + crt;
		priorityQueue.push_back(CORES[i].currentProcess);
		cout << "-- Process " << nextProcess.ID << " will release a core at time " << CORES[i].releaseTime << " ms" << endl;
		readyQueue.pop();
	}
}

// SSD request routine
void SSDRequestRoutine(Process process, int srt)
{
	cout << "-- Process " << process.ID << " requests SSD access at time " << CLOCK << " ms for " << srt << " ms" << endl;
	SSD.count += 1;
	if(SSD.available = true){
		SSD.available = false; // mark SSD busy
		SSD.currentProcess = process;
		SSD.releaseTime = CLOCK + srt;
		ChangeProcessState(process, 1);
		SSD.currentProcess.eventTime = CLOCK + srt;
		priorityQueue.push_back(SSD.currentProcess);
		SSD.accessTime += srt;
		cout << "-- Process " << SSD.currentProcess.ID << " will release SSD at time " << SSD.releaseTime << " ms" << endl;
	}else{
		cout << "-- Process " << process.ID << " must wait for SSD " << endl;
		SSDQueue.push(process); // enter process in SSD queue
		ChangeProcessState(process, 0);
		cout << "-- SSD queue now contains " << SSDQueue.size() << " process(es) waiting for SSD" << endl;	
	}
}

// SSD request completion routine
void SSDCompletionRoutine()
{
	if(SSDQueue.empty()){
		SSD.available = true;
	}else{
		Process nextProcess = SSDQueue.front();
		SSD.available = false;  // mark SSD busy
		SSD.currentProcess = nextProcess; 
		double srt = nextProcess.eventTime;
		SSD.releaseTime = CLOCK + srt;	
		SSD.currentProcess.eventTime = CLOCK + srt;
		SSD.accessTime += srt;
		priorityQueue.push_back(SSD.currentProcess);
		ChangeProcessState(nextProcess, 1);
		cout << "-- Process " << SSD.currentProcess.ID << " will release SSD at time " << SSD.releaseTime << " ms" << endl;
		SSDQueue.pop();
	}
}

// Input request routine
void InputRequestRoutine(Process process, int irt)
{
	cout << "-- Process " << process.ID << " requests input from user at time " << CLOCK << " ms for " << irt << " ms" << endl;
	if(USER.available == true){
		USER.available = false;  // mark user busy
		USER.currentProcess = process;
		ChangeProcessState(process, 2);
		USER.releaseTime = CLOCK + irt;
		USER.currentProcess.eventTime = CLOCK + irt;
		priorityQueue.push_back(USER.currentProcess);
		cout << "-- Process " << USER.currentProcess.ID << " start input at time " << CLOCK << " ms" << endl;
		cout << "-- Process " << USER.currentProcess.ID << " will complete input at time " << USER.releaseTime << " ms" << endl;
	}else{
		cout << "-- Process " << process.ID << " must wait for user " << endl;
		process.eventTime = irt;
		inputQueue.push(process); // enter process in input queue
		ChangeProcessState(process, 2);
		cout << "-- Input queue	now contains " << inputQueue.size() << " process(es) waiting for user" << endl;
	}
}

// Input request completion routine
void InputCompletionRoutine()
{
	if(inputQueue.empty()){
		USER.available = true;
	}else{
		Process nextProcess = inputQueue.front();
		USER.available = false;  // mark user busy
		USER.currentProcess = nextProcess;
		double irt = nextProcess.eventTime;
		USER.releaseTime = CLOCK + irt;
		ChangeProcessState(nextProcess, 2);
		USER.currentProcess.eventTime = CLOCK + irt;
		priorityQueue.push_back(USER.currentProcess);
		cout << "-- Process " << USER.currentProcess.ID << " will complete input at time " << USER.releaseTime << " ms" << endl;
		inputQueue.pop();
	}
}

// Print the state of processes when an event arrives
void ProcessStateCheck(Process process)
{
	if(!PROCESSES.empty()){
		for(int i=0; i < PROCESSES.size(); i++){
			string state;
			if(PROCESSES[i].ID != process.ID){		
				if(PROCESSES[i].state == 0){
					state = "READY";
				}
				if(PROCESSES[i].state == 1){
					state = "RUNNING";
				}
				if(PROCESSES[i].state == 2){
					state = "BLOCKED";
				}
				cout << "Process " << PROCESSES[i].ID << " is " << state << endl;
			}
			
			if(PROCESSES[i].state == 3){
				state = "TERMINATED";
				cout << "Process " << PROCESSES[i].ID << " is " << state << endl;
			}
		}
	}
}

// Change the state of a process with number (0:READY, 1:RUNNING, 2:BLOCKED, 3:TERMINATED)
void ChangeProcessState(Process process, int num)
{
	if(!PROCESSES.empty()){
		for(int j=0; j < PROCESSES.size(); j++){
			if(PROCESSES[j].ID == process.ID){
				PROCESSES[j].state = num;
				break;
			}	
		}
	}
}

// Sort processes in priority queue using selection sort
void SortProcessesbyDeadline()
{
	Process temp;
	for(int i=0; i < (priorityQueue.size()-1); i++){
		int min = i;
		for(int j=i+1; j < priorityQueue.size(); j++){
			if(priorityQueue[min].eventTime > priorityQueue[j].eventTime){
				min = j;	
			}
			temp = priorityQueue[min];
			priorityQueue[min] = priorityQueue[i];
			priorityQueue[i] = temp;
		}
	}
}


