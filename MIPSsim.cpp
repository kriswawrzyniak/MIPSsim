
//
//  MIPSsim.cpp
//  MIPSsim
//
//  Created by Kris Wawrzyniak on 2/20/19.
//  Copyright © 2019 Kris Wawrzyniak. All rights reserved.
//
//	"On my honor, I have neither given nor received unauthorized aid on this assignment"
//	PROJECT 1: Petri Net Simulator for a Simplified MIPS Processor
//
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <map>

using namespace std;

//GLOBAL VARIABLES
static int step = 0;    //STEP COUNTER
static int instIndex = 1; //INSTRUCTION ORDER
static int currIndexREB = 0; //CURRENT HIGHEST INDEXED INST IN REB
bool INMF, INBF, AIBF, LIBF, ADBF, REBF, DAMF = true; //FLAGS TO CONTROL OPERATION FLOW

//STRUCTS
struct Token{		//INSTRUCTION TOKEN (SRC REGISTERS ARE NOT VALUES)
	string opcode;
	string dest, src1, src2;
};
struct TokenVal{	//INSTRUCTION VALUE TOKEN (SRC REGISTERS ARE VALUES)
	string opcode, dest;
	int src1, src2, index;
};
struct TokenADB{	//TOKEN FOR ADB
	string dest;
	int val;
	int index;
};

//PLACE/BUFFERS (DATA STRUCTURES)
static deque<Token>             INM;    //INSTRUCTION MEMORY (16)
static map<string,int>          RGF;    //REGISTER FILE (8)
static map<int,int>             DAM;    //DATA MEMORY (8)

static TokenVal                 INB;    //INSTRUCTION BUFFER (1)
static TokenVal                 AIB;    //ARITHMETIC BUFFER (1)
static TokenVal                 LIB;    //LOAD INSTRUCTION BUFFER (1)
static TokenADB 				ADB;    //ADDRESS BUFFER (1)
static deque<pair<string,int> > REB;    //RESULT BUFFER (2) - USE DEQUE BECUASE 2 SPACES AVAILABLE

//TRANSITIONS (FUNCTIONS)
void read();
void decode();
void issue1();
void issue2();
void alu();
void addr();
void load();
void write();

//HELPER FUNCTINOS
void readFile();        				//Reads input files
void writeFile(ofstream& outputFile);	//Writes current step to output

//********************************************************************************//
//******************************FUNCTION DEFINITIONS******************************//
//********************************************************************************//

//****************HELPER FUNCTIONS****************//
//***readFile***//
void readFile(){
	//TEMP VARIABLES
	string temp;
	string delimiter = ",";
	string token;
	int tokenInt;
	size_t pos = 0;
	int cnt = 0;
	
	//OPEN FILES
	ifstream instructionFile("instructions.txt");
	ifstream registerFile("registers.txt");
	ifstream dataMemoryFile("datamemory.txt");
	//ERROR CHECK
	if(!instructionFile || !registerFile || !dataMemoryFile){ cout<<"Error opening files"<< endl; exit(0);}
	
	//INSTRUCTION FILE READ
	while (instructionFile >> temp){
		cnt = 0;
		Token inTemp;
		unsigned long first = temp.find("<")+1;
		unsigned long last = temp.find(">");
		temp = temp.substr (first,last-first);
		//REFERENCE: PARSE ALGORITHM https://stackoverflow.com/a/14266139
		while ((pos = temp.find(delimiter)) != std::string::npos) {
			token = temp.substr(0, pos);
			//std::cout << token << std::endl;
			if (cnt == 0){
				inTemp.opcode = token;
			}
			else if (cnt ==1){
				inTemp.dest = token;
			}
			else if (cnt ==2){
				inTemp.src1 = token;
			}
			temp.erase(0, pos + delimiter.length());
			cnt ++;
		}
		inTemp.src2 = temp;
		temp.erase();
		token.erase();
		INM.push_back(inTemp);
	}
	
	//REGISTER FILE READ
	pos = 0;
	while (registerFile >> temp){
		unsigned long first = temp.find("<")+1;
		unsigned long last = temp.find(">");
		temp = temp.substr (first,last-first);
		//CLEAN THIS UP DONT NEED A WHIE LOOP
		while ((pos = temp.find(delimiter)) != string::npos) {
			token = temp.substr(0,pos);
			temp.erase(0, pos + delimiter.length());
		}
		//REFERENCE: STORING STRING AS INT https://stackoverflow.com/a/7664227
		RGF[token] = stoi(temp);
		token.erase();
		temp.erase();
	}
	
	//DATA MEMORY FILE READ
	pos = 0;
	while(dataMemoryFile >> temp){
		unsigned long first = temp.find("<")+1;
		unsigned long last = temp.find(">");
		temp = temp.substr (first,last-first);
		//CLEAN THIS UP DONT NEED A WHIE LOOP
		while ((pos = temp.find(delimiter)) != string::npos) {
			tokenInt = stoi(temp.substr(0,pos));
			temp.erase(0, pos + delimiter.length());
		}
		DAM[tokenInt] = stoi(temp);
		token.erase();
		temp.erase();
	}
}
//***writeFile***//
void writeFile(ofstream& outputFile){
	//PRINT STEP
	outputFile << "STEP " << step << ":" << endl;
	//cout<<"STEP "<<step<<":"<<endl;
	
	//PRINT INM
	outputFile << "INM:";
	//cout << "INM: ";
	for( deque<Token>::iterator i = INM.begin(); i != INM.end(); i++){
		outputFile << "<" << i->opcode << "," << i->dest << "," << i->src1 << "," << i->src2 << ">";
		//cout << "<" << i->opcode << "," << i->dest << "," << i->src1 << "," << i->src2 << ">";
		if(i != INM.end()-1){
			outputFile << ",";
			//cout << ",";
		}
	}
	outputFile << endl;
	//cout << endl;
	
	//PRINT INB
	outputFile << "INB:";
	//cout << "INB: ";
	if(!INB.opcode.empty()){
		outputFile << "<" << INB.opcode << "," << INB.dest << "," << INB.src1 << "," << INB.src2 << ">";
		//cout << "<" << INB.opcode << "," << INB.dest << "," << INB.src1 << "," << INB.src2 << ">";
	}
	outputFile << endl;
	//cout << endl;
	
	//PRINT AIB
	outputFile << "AIB:";
	//cout << "AIB: ";
	if(!AIB.opcode.empty()){
		outputFile << "<" << AIB.opcode << "," << AIB.dest << "," << AIB.src1 << "," << AIB.src2 << ">";
		//cout << "<" << AIB.opcode << "," << AIB.dest << "," << AIB.src1 << "," << AIB.src2 << ">";
	}
	outputFile << endl;
	//cout << endl;
	
	//PRINT LIB
	outputFile << "LIB:";
	//cout << "LIB: ";
	if(!LIB.opcode.empty()){
		outputFile << "<" << LIB.opcode << "," << LIB.dest << "," << LIB.src1 << "," << LIB.src2 << ">";
		//cout << "<" << LIB.opcode << "," << LIB.dest << "," << LIB.src1 << "," << LIB.src2 << ">";
	}
	outputFile << endl;
	//cout << endl;
	
	//PRINT ADB
	outputFile << "ADB:";
	//cout << "ADB: ";
	if(!ADB.dest.empty()){
		outputFile << "<" << ADB.dest << "," << ADB.val << ">";
		//cout << "<" << ADB.dest << "," << ADB.val << ">";
	}
	outputFile << endl;
	//cout << endl;
	
	//PRINT REB
	outputFile << "REB:";
	//cout << "REB: ";
	if(!REB.empty()){
		for( deque<pair<string,int> >::iterator i = REB.begin(); i != REB.end(); i++){
			outputFile << "<" << i->first << "," << i->second  << ">";
			//cout << "<" << i->first << "," << i->second  << ">";
			if(next(i) != REB.end()){
				outputFile << ",";
				//cout << ",";
			}
		}
	}
	outputFile << endl;
	//cout << endl;
	
	//PRINT RGF
	outputFile << "RGF:";
	//cout << "RGF: ";
	//REFERENCE: WAS GETTING AN ERROR USING AUTO https://stackoverflow.com/a/31526856
	for( map<string,int>::iterator i = RGF.begin(); i != RGF.end(); i++){
		outputFile << "<" << i->first << "," << i->second  << ">";
		//cout << "<" << i->first << "," << i->second  << ">";
		if(next(i) != RGF.end()){
			outputFile << ",";
			//cout << ",";
		}
	}
	outputFile << endl;
	//cout << endl;
	//PRINT DAM
	outputFile << "DAM:";
	//cout << "DAM: ";
	for( map<int,int>::iterator i = DAM.begin(); i != DAM.end(); i++){
		outputFile << "<" << i->first << "," << i->second  << ">";
		//cout << "<" << i->first << "," << i->second  << ">";
		if(next(i) != DAM.end()){
			outputFile << ",";
			//cout << ",";
		}
	}
	outputFile  << endl;
	//cout << endl;
}

//****************TRANSITION FUNCTIONS****************//
//***read***//
void read(){
	string src1 = INM.front().src1;			//GET SRC REGISTERS
	string src2 = INM.front().src2;
	//REFERENCE: CHECK IF A MAP CONTAINS A KEY https://stackoverflow.com/a/1940020
	if(RGF.count(src1)>0 && RGF.count(src2)>0){
		INB.src1 = RGF[src1];				//PUSH SRC VALS TO INB
		INB.src2 = RGF[src2];
		return;
	}
	else{
		//cout << "Registers not available for read" << endl;
	}
}
//***decode***//
void decode(){
	if(!INM.empty()){
		INB.opcode = INM.front().opcode; 	//PUSH OPCODE TO INB
		INB.dest = INM.front().dest;		//PUSH DEST TO INB
		INB.index = instIndex;				//SAVE INST INDEX TO INB
		instIndex++;						//INC INST INDEX
		read();								//READ SRC VALUES
		INM.pop_front();					//CLEAR INST FROM INM
		INBF = false; 						//LOCK INB
	}
	else{
		//cout << "No instructions in INM for decode/read" << endl;
	}
}
//***issue1***//
void issue1(){
	if(INBF && INB.opcode != "LD" && (INB.opcode == "ADD" || INB.opcode == "SUB" || INB.opcode == "AND" || INB.opcode == "OR" || INB.opcode == "LD")){
		/*
		 AIB.opcode = INB.opcode;
		 AIB.dest = INB.dest;
		 AIB.src1 = INB.src1;
		 AIB.src2 = INB.src2;
		 */
		//CHANGED TO THIS
		AIB = INB; 			//PUSH TO AIB
		INB = TokenVal(); 	//CLEAR INB
		INBF = true; 		//UNLOCK INB
		AIBF = false; 		//LOACK AIB
	}
	else{
		//cout << "No instruction in INB for issue1" << endl;
	}
}
//***issue2***//
void issue2(){
	if(INBF && INB.opcode == "LD" && (INB.opcode != "ADD" || INB.opcode != "SUB" || INB.opcode != "AND" ||INB.opcode != "OR" )){
		/*
		 LIB.opcode = INB.opcode;
		 LIB.dest = INB.dest;
		 LIB.src1 = INB.src1;
		 LIB.src2 = INB.src2;
		 */
		//CHANGED TO THIS
		LIB = INB;			//PUSH TO LIB
		INB = TokenVal();	//CLEAR INB
		INBF = true; 		//UNLOCK INB
		LIBF = false; 		//LOCK LIB
	}
	else{
		//cout << "No instruction in INB for issue2" << endl;
	}
}
//***alu***//
void alu(){
	if(AIBF && !AIB.opcode.empty()){
		int temp;
		//ALU OPERATIONS
		if(AIB.opcode == "ADD"){
			temp = AIB.src1 + AIB.src2;
		}
		if(AIB.opcode == "SUB"){
			temp = AIB.src1 - AIB.src2;
		}
		if(AIB.opcode == "AND"){
			temp = AIB.src1 & AIB.src2;
		}
		if(AIB.opcode == "OR"){
			temp = AIB.src1 | AIB.src2;
		}
		else {
			//cout << "Inalid instruction in AIB buffer for ALU" << endl;
		}
		//CHECK INST INDEX PUSH TO REB IN CORRECT OREDER
		if(AIB.index > currIndexREB){
			REB.push_back(make_pair(AIB.dest, temp));	//PUSH TO END OF REB
			currIndexREB = AIB.index;					//UPDATE REB CURR INDEX
		}
		else {
			REB.push_front(make_pair(AIB.dest, temp));	//PUSH TO BEGIN OF REB

		}
		AIB = TokenVal();								//CLEAR AIB
		AIBF = true; 	//UNLOCK AIB
		REBF = false; 	//LOCK REB - NOT REALLY IMPORTANT BECAUSE WRITE HAPPENS FIRST (WOULD NEED TO FIX THIS IF IT DIDNT BECAUSE REB CAN HAVE 2 ENTRIES)
	}
	else{
		//cout << "No instrucitons in AIB for ALU" << endl;
	}
}
//***addr***//
void addr(){
	if(LIBF && !LIB.opcode.empty()){
		ADB.dest = LIB.dest; 			//PUSH LIB DEST TO ADB
		ADB.val = LIB.src1 + LIB.src2;	//PUSH LIB VAL TO ADB
		LIB = TokenVal();				//CLEAR LIB
		LIBF = true; 					//UNLOCK LIB
		ADBF = false; 					//LOCK ADB
	}
	else {
		//cout << "No instructions in LIB for addr" << endl;
	}
}
//***load***//
void load(){
	if(ADBF && !ADB.dest.empty()){
		//CHECK INST INDEX PUSH TO REB IN CORRECT OREDER
		//REB <- ADB dest string , DAM value at ADB mem address
		if(ADB.index > currIndexREB){
			REB.push_back(make_pair(ADB.dest, DAM[ADB.val]));
			currIndexREB = ADB.index;
		}
		else {
			REB.push_front(make_pair(ADB.dest, DAM[ADB.val]));
		}
		ADB = TokenADB();	//CLEAR ADB
		ADBF = true; 		//UNLOCK ADB
		REBF = false; 		//LOCK REB - NOT REALLY IMPORTANT BECAUSE WRITE HAPPENS FIRST (WOULD NEED TO FIX THIS IF IT DIDNT BECAUSE REB CAN HAVE 2 ENTRIES)
	}
	else{
		//cout << "No instruction in ADB for load" << endl;
	}
}
//***write***//
void write(){
	if(REBF && !REB.empty()){
		RGF[REB.front().first] = REB.front().second;	//PUSH REB TO RGF
		REB.pop_front();								//REMOVE REB ENTRY
		REBF = true; 									//UNLOCK REB
	}
	//cout << "No instructions in REB for write" << endl;
}

//********************************************************************************//
//******************************** MAIN FUNCTION *********************************//
//********************************************************************************//
int main(int argc, const char * argv[]) {
	//INITILIZATION
	ofstream outputFile;
	outputFile.open("simulation.txt");
	readFile();
	while(1){
		//SIMULATE
		writeFile(outputFile);
		outputFile << endl;
		write();
		issue2();
		issue1();
		alu();
		load();
		addr();
		decode();
		//FIX ORDERING HERE TO MAKE MORE READABLE
		if(INMF && INBF && AIBF && LIBF && ADBF && REBF && DAMF && REB.empty()){
			step++;
			writeFile(outputFile);
			break;
		}
		INMF = true;
		INBF = true;
		AIBF=true;
		LIBF=true;
		ADBF=true;
		REBF=true;
		DAMF = true;
		step++;
	}
	return 0;
}
/*
 FUTURE WORK:
 	FIX REB AND RGF FLAGS SO THAT ORDER OF TRANSITIONS WOULDNT MATTER
 	CLEAN UP RGF AND DAM READ TO GET RID OF WHILE LOOP
 	IMPLEMENT CONTINUOUS READING OF INM SO THAT NEW INSTRUCTIONS CAN BE ADDED CONTINUOUSLY
 	POSSIBLY USE SEMAPHORES AND THREADS INSTEAD OF GLOBAL VARS
 	BETTER ABSTRACTION WITH CLASSES TO REDUCE NEED FOR GLOBAL VARS/AVOID PROBLEMS
 		-USE OF CLASSES
 	POSSIBLY CREATE GUI TO ILLUSTRATE THE EXECUTION STEPS
 */
