#include "CPU.h"

instruction::instruction(bitset<32> fetch)
{
	//cout << fetch << endl;
	instr = fetch;
	//cout << instr << endl;
}

CPU::CPU()
{
	for(int i = 0; i < 32; i++) {
   		registerFile[i] = 0;
	}
	PC = 0; //set PC to 0
	for (int i = 0; i < 4096; i++) //copy instrMEM
	{
		dmemory[i] = (0);
	}
	CPU::Reset();

}

void CPU::Reset() {
	Jump = 0;
	Branch = 0;
	MemRead = 0;
	MemToReg = 0;
	ALUOp = 0;
	MemWrite = 0;
	ALUSrc = 0;
	RegWrite = 0;

	// Decode Stage
	operation = ZE;
	opcode = 0;
	func3 = 0;
	func7 = 0;
	rs1 = 0;
	rs2 = 0;
	rd = 0;
	immediate = 0;

	// Execute Stage
	readData1 = 0;
	readData2 = 0;
	LessThan = 0;

	// Memory Stage
	aluResult = 0;
	dataMemoryResult = 0;
}

bitset<32> CPU::Fetch(bitset<8> *instmem) {
	//cout << "Fetching" << endl;
	bitset<32> instr = ((((instmem[PC + 3].to_ulong()) << 24)) + ((instmem[PC + 2].to_ulong()) << 16) + ((instmem[PC + 1].to_ulong()) << 8) + (instmem[PC + 0].to_ulong()));  //get 32 bit instruction
	//PC += 4;//increment PC
	return instr;
}


bool CPU::Decode(instruction* curr)
{
	//cout<<curr->instr<<endl;
	//cout << "Decoding" << endl;
	opcode = curr->instr.to_ulong() & 0x7f; //0b1111111
	func3 = (curr->instr.to_ulong() >> 12) & 0x7;  //0b111
	func7 = (curr->instr.to_ulong() >> 25) & 0x7f; //0b1111111
	rs1 = (curr->instr.to_ulong() >> 15) & 0x1f; //0b11111
	rs2 = (curr->instr.to_ulong() >> 20) & 0x1f; //0b11111
	rd = (curr->instr.to_ulong() >> 7) & 0x1f; //0b11111

	int32_t instruction = static_cast<int32_t>(curr->instr.to_ulong());

	if (opcode == RTYPE) {
		if (func3 == 0x0) {
			if (func7 == 0x0) {ALUOp = ALUadd ; operation = ADD;}
			else if (func7 == 0x20) {ALUOp = ALUsub; operation = SUB;}
			else {operation = ERROR;}
		} 
		else if (func3 == 0x4) {ALUOp = ALUxor; operation = XOR;}
		else if (func3 == 0x5) {ALUOp = ALUsra; operation = SRA;}
		RegWrite = 1;
		ALUSrc = 1;
	} else if (opcode == ITYPE) {
		if (func3 == 0x0) {ALUOp = ALUadd; operation = ADDI;}
		else if (func3 == 0x7) {ALUOp = ALUand; operation = ANDI;}
		immediate = (instruction >> 20);
		RegWrite = 1;
	} else if (opcode == LWORD && func3 == 0x2) {
		ALUOp = ALUadd; 
		operation = LW;
		immediate = (instruction >> 20);
		MemRead = 1;
		RegWrite = 1;
		MemToReg = 1;
	} else if (opcode == SWORD && func3 == 0x2) {
		ALUOp = ALUadd;
		operation = SW;
		int32_t imm11_5 = instruction & 0xfe000000;
		int32_t imm4_0 = (instruction & 0xf80) << 13;
		immediate = (imm11_5 + imm4_0) >> 20; //Redefined for SW
		MemWrite = 1;
	} else if (opcode == JTYPE && func3 == 0x0) {
		//cout << "Jump Instruction" << endl;
		ALUOp = ALUadd;
		operation = JALR;
		immediate = instruction >> 20;		
		Jump = 1;
		RegWrite = 1;
	} else if (opcode == BTYPE && func3 == 0x4) {
		//cout << "Branch Instruction" << endl;
		ALUOp = ALUsub;
		operation = BLT;
		int32_t imm12 = (instruction & 0x80000000) ? 0xFFFFF800 : 0x0;// Bit 31 (sign extend for negative values)
		int32_t imm10_5 = (instruction & 0x7E000000) >> 20; //bits 30-25
		int32_t imm4_1 = (instruction & 0xF00) >> 7; // bits 11-8
		int32_t imm11 = (instruction & 0x80) << 4; //bit 7
		immediate = imm12 | imm11 | imm10_5 | imm4_1;
		Branch = 1;
		ALUSrc = 1;
	}
	else if (opcode == ZERO) {
		operation = ZE;
		return false;
	}
	//cout << opcode << endl;
	// Still need BLT and JALR
	readData1 = registerFile[rs1];
	readData2 = registerFile[rs2];
	return true;
}

bool CPU::Execute() {
	//cout << "Executing" << endl;
	aluResult = 0;
	int32_t input1, input2;
	input1 = readData1;
	input2 = (ALUSrc == 1) ? registerFile[rs2] : immediate;
	switch(ALUOp) {
		case ALUadd:
			aluResult = input1 + input2;
			break;
		case ALUsub:
			aluResult = input1 - input2;
			break;
		case ALUand:
			aluResult = input1 & input2;
			break;
		case ALUxor:
			aluResult = input1 ^ input2;
			break;
		case ALUsra:
			aluResult = input1 >> input2;
			break;
		default:
			return false;
	}
	if (aluResult < 0) {
		LessThan = 1;
	}
	
	return true;
}

bool CPU::Memory() {
	//cout << "Memory" << endl;
	if (MemWrite == 1) { //Store Word
		int32_t sByte1, sByte2, sByte3, sByte4;
		sByte1 = (readData2 >> 24) & 0xff;
		sByte2 = (readData2 >> 16) & 0xff;
		sByte3 = (readData2 >> 8) & 0xff;
		sByte4 = (readData2) & 0xff;

		//cout << "Store Bytes " << sByte1 << sByte2 << sByte3 << sByte4 << endl;
		
		// Storing the bytes in dmemory
		dmemory[aluResult] = sByte4; //LSB
		dmemory[aluResult + 1] = sByte3;
		dmemory[aluResult + 2] = sByte2;
		dmemory[aluResult + 3] = sByte1;

		//cout << "Storing: " << readData2 << " at location " << aluResult << endl;
	}

	if (MemRead == 1) { //Load Word
		int32_t lByte1, lByte2, lByte3, lByte4;
		lByte1 = dmemory[aluResult];
		lByte2 = dmemory[aluResult + 1];
		lByte3 = dmemory[aluResult + 2];
		lByte4 = dmemory[aluResult + 3]; 

		dataMemoryResult = (lByte4 << 24) + (lByte3 << 16) + (lByte2 << 8) + lByte1;

		//cout << "Loading result at: " << aluResult << ", which has value of " << dataMemoryResult << " into register " << rd << endl;
	}
	return true;

}

bool CPU::WriteBack() {
	//cout << "WriteBack" << endl;
	if (RegWrite == 1 and rd != ZERO) {
		if (MemToReg == 1) {
			//cout << "should never run";
			registerFile[rd] = dataMemoryResult;
		} else if (Jump == 1) {
			registerFile[rd] = PC + 4;
		}
		else {
			//cout << "Writing result: " << aluResult << " to register " << rd << endl;
			registerFile[rd] = aluResult;
		}
	}
	return true;
}

bool CPU::NextInstruction() {
	//cout << "Next Instruction" << endl;
	if (Jump == 1) {
		PC = aluResult;
	} else if (Branch == 1 && LessThan == 1) {
		PC = PC + immediate;
	} else {
		PC = PC + 4;
	}
	CPU::Reset();
	return true;
}

std::pair<int32_t, int32_t> CPU::GetReturnRegisters() {
    return std::make_pair(registerFile[10], registerFile[11]);
}


unsigned long CPU::readPC()
{
	return PC;
}

// Add other functions here ... 