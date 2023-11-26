#include <iostream>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace std;

#define ZERO 0x0
#define RTYPE 0x33
#define ITYPE 0x13
#define LWORD 0x3
#define SWORD 0x23
#define BTYPE 0x63
#define JTYPE 0x67

#define ALUadd 0x0
#define ALUsub 0x1
#define ALUand 0x2
#define ALUxor 0x3
#define ALUsra 0x4

class instruction {
public:
	bitset<32> instr; // instruction
	instruction(bitset<32> fetch); // constructor
};

enum Op {
	ZE, ERROR, ADD, SUB, ADDI, XOR, ANDI, SRA, LW, SW, BLT, JALR,
};

class CPU {
private:
	int dmemory[4096]; // data memory byte addressable in little endian fashion;
	unsigned long PC; 
	int32_t registerFile[32];  // Registers x0-x31
	uint32_t Jump;
	uint32_t Branch;
	uint32_t MemRead;
	uint32_t MemToReg;
	uint32_t ALUOp;
	uint32_t MemWrite;
	uint32_t ALUSrc;
	uint32_t RegWrite;

	// Decode Stage
	Op operation;
	uint32_t opcode;
	uint32_t func3;
	uint32_t func7;
	uint32_t rs1;
	uint32_t rs2;
	uint32_t rd;
	int32_t immediate;

	// Execute Stage
	int32_t readData1;
	int32_t readData2;
	uint32_t LessThan;

	// Memory Stage
	int32_t dataMemoryResult;
	int32_t aluResult; // Added missing semicolon here

	// Statistics
	unsigned int clockCount;
	unsigned int rTypeCount;
	unsigned int iTypeCount;
	unsigned int swCount;
	unsigned int lwCount;
	unsigned int jCount;
	unsigned int bCount;
	unsigned int totalInsCount; // Corrected to 'unsigned int'
	double ipc;

public:
	CPU();
	unsigned long readPC();
	bitset<32> Fetch(bitset<8> *instmem);
	void Reset();
	bool Decode(instruction* instr);
	bool Execute();
	bool Memory();
	bool WriteBack();
	bool NextInstruction();
	void GetStats();
	std::pair<int, int> GetReturnRegisters();
};

// add other functions and objects here
