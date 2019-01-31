/*

This program assembles an instruction set with the following requirements:
	- Only operations that involves max 2 operands can be implemented;
	- each instruction has to be made of 14 bits.

	/// FILES REQUIRED ///
	In order to launch the program correctly, 2 files need to be passed as argument:
	 - instruction set file;
	 - file to assemble.

The instruction set has to be provided in a file formatted as following:
[begin of file]
#numberOfInstructions
#numberOfOperands instructionName firstOperandMask firstOperandShift secondOperandMask secondOperandShift opCode opCodeShift
#numberOfOperands ...
...
[end of file]

First element of FILE is a number representing how many instructions are listed below.

Next, each row corresponds to an instruction.

	- 1st element of row is how many operands will be used. It can be 0, 1 or 2.
	- 2nd element of row is instruction's name.
	- 3rd and 5th elements of row are 1° and 2° operands' masks.  They are integers corresponding to operand bit size (ex: operand is 1011011, so number of bits is 7).
	- 4th and 6th elements of row are 1° and 2° operands' shifts. They corresponds to the 'position' of each operand in the 14-bit word counting from right to left.
	- 7th element of row is opCode.
	- 8th element of row is opCode's shift, corresponding to the 'position' of opCode in the 14-bit word counting from right to left.
	IMPORTANT NOTE: 3rd and 4th elements are required only if the instruction requires 1 or 2 operands (5th and 6th elements only if the instr. requires 2 operands);
					so if they are not required, they must be omitted.


An instruction set file for pic16f672a is provided. It is named: pic16f672a_InS.txt

	/// OUTPUT ///
	Output of this program will be a file named hexFormatProgram.txt, in which there will be the assembled code (hex format).

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG		// uncomment to debug
#define HELP "You need to pass 2 files as arguments:\n 1st file must contain the instruction set;\n 2nd file has to be assembly code to translate in hex format.\n"

struct instruction {
	char name[10];				// name of instruction, ex: movf
	int numOperands;			// how many operands the instruction needs. Ex: movf f,d => 2 operands
	int maskOperand[2];			// each element is a mask of a number of '1' corresponding to operand size -- ex: 1010010 => mask is: 111 1111 => number of ones is: 7
	int shiftOperand[2];		// each element is how many right shifts are required to use the corresponding mask
	int opCode;					// it represents opcode
	int shiftOpCode;			// how many right shifts are required to extract only opcode from instruction
};

int acquireInstruction(FILE*, struct instruction *);
int printInstruction(struct instruction);
int swapData(short*);

union instrWord	{				// each word of 2 bytes corresponds to an instruction
	short fullWord;				// fullWord is used to access to the full word
	char byteWord[2];			// byteWord[0] and byteWord[1] are used to access first byte and second byte of word
};


int main (int argc, char* argv[]) {
	FILE *filePtr;
	int i,j = 0, numInstructions;
	char c, acquiredOpName[10];			// temp variable to store instruction acquired from asm file
	int index;							// variable in which index of current instruction from instructionSet will be stored
	int operands[2] = {0,0};			// vector containing operands values obtained from asm file
	int numWords = 0;
	unsigned short hexInstruction;
	unsigned short *finalProgramWord;
	unsigned int checksum=0;

	/***********		Check if the file could be opened		************/
	if ((filePtr=fopen(argv[1], "r"))==NULL) {		// argv[1] will be the instruction set
		printf("Invalid file.\n" HELP);
		return 0;
	}

	/************		load instruction Set		**************/
	fscanf(filePtr, "%d", &numInstructions);				// first element in file is number of instructions contained
	struct instruction instructionSet[numInstructions];
	finalProgramWord = (short *)malloc(1*sizeof(short));	// allocate space for one 14-bit instruction
	// (it wil be updated basing on how many words are needed for the full program)

	for (i=0; i<numInstructions; i++) {
		acquireInstruction(filePtr, &instructionSet[i]);	// call acquireInstruction to load the instruction in my struct.
	}	// (instructionSet address is passed so I can modify it)


	/***********		close instruction set file, open file to convert into HEX		***************/
	fclose(filePtr);
	if ((filePtr=fopen(argv[2], "r"))==NULL) {				// argv[2] will be instrToHex.asm
		printf("Invalid file.\n" HELP);
		return 0;
    }


	/************		read each line of file		***************/
	while (fscanf(filePtr, "%s", acquiredOpName) != EOF) {
		operands[0]=0;
		operands[1]=0;
		index = -1;							// index = -1 stands for instruction not valid.

		for (i=0; i<numInstructions; i++) {	// search in array of struct instructionSet the data associated to instruction name acquired
			if ( strcmp(acquiredOpName, instructionSet[i].name)==0 ) {
				#ifdef DEBUG
					printf("Acquired op is: %s\n", instructionSet[i].name);
				#endif

				index = i;					// instruction found in instructionSet at position "i"
				break;
			}
		}

		if (index != -1) {					// if index is not -1 (so it means that i found a correct instruction)
		// the code below extracts all operands from file and puts them into array operands[], erasing characters like ',' ' ' and '0x'
			fscanf(filePtr, "%c", &c);      // eliminate ' ' after name. Example: movwl' '.4
			for (j=0; j<instructionSet[index].numOperands; j++) {		// acquire data from operands followed by ','
				fscanf(filePtr, "%c", &c);						// acquire . or 0 (decimal or hex)
				if (c == '.') {									// if the character is '.', the following is a decimal number
					fscanf(filePtr, "%d", &operands[j]);		// get OPERAND as decimal number
					if (j<instructionSet[index].numOperands-1)	// if it's not the last operand (so it is followed by ',')
						fscanf(filePtr, "%c", &c);				// remove ','
				}
				else if (c == '0') {							// if the character is '0', the following is a hex number
					fscanf(filePtr, "%c", &c);					// remove 'x' (0x now removed)
					fscanf(filePtr, "%x", &operands[j]);		// get OPERAND as hex number
					if (j<instructionSet[index].numOperands-1)	// if it's not the last operand (so it is followed by ',')
						fscanf(filePtr, "%c", &c);				// remove ','
				}
			}

			hexInstruction = ( instructionSet[index].opCode << instructionSet[index].shiftOpCode );	// puts opCode in hexInstruction
			for(j=0; j<instructionSet[index].numOperands; j++) {// puts operands in hexInstruction
				hexInstruction += ( operands[j] << instructionSet[index].shiftOperand[j] );
			}

			#ifdef DEBUG
				printf("INSTRUCTION IS: %04x\n\n", hexInstruction);
			#endif
			
				// reallocate space to contain one more instruction
			finalProgramWord = (short*)realloc( finalProgramWord, (numWords+1)*sizeof(short) );
			swapData(&hexInstruction);	// after this function call the hexInstruction will be swapped like: 3003 => 0330 or 1234 => 3412
			finalProgramWord[numWords] = hexInstruction;		// load new hexInstruction swapped
			checksum += (hexInstruction)&(0xff);				// adding first 8 bits to checksum
			#ifdef DEBUG
				printf("Checksum1: %x\n", checksum);
			#endif
			checksum += (hexInstruction>>8)&(0xff);				// adding second 8 bits to checksum
			#ifdef DEBUG
				printf("Checksum2: %x\n", checksum);
			#endif
			numWords++;
		}
	}
	checksum += numWords*2;				// adding data field to checksum (in this implementation address is always 0x0 so it won't be added)
	#ifdef DEBUG
		printf("checksum: %x\n", checksum);
	#endif
	checksum = ~checksum;				// negate checksum 	(first step to get the final value)
	checksum += 1;						// add 1		(second step to get checksum -- "not" and "+1" are needed to get 2s complement)
	checksum = checksum & 0xff;			// extract a byte	(last step to get checksum, we take only first byte)
	#ifdef DEBUG
		printf("Final value of checksum: %02x\n", checksum);
	#endif

	fclose(filePtr);					// close instrToHex.asm


	/*******		Write program in hex format in a file		*********/
	if((filePtr=fopen("hexFormatProgram.txt", "w"))==NULL)	{
			printf("Can't create destination file.\n");
			return 18;
	}

	fprintf(filePtr, ":020000040000fa\n");		// print on file extended linear address
	fprintf(filePtr, ":%02x", numWords*2);		// print on file number of bytes of data field (each instr word is made of 2 bytes)
	fprintf(filePtr, "000000");					// print on file program memory start address "0000" and data type "00"

	for (i=0; i<numWords; i++)	{
		fprintf(filePtr, "%04x", finalProgramWord[i]);	// print on file data field
	}

	fprintf(filePtr, "%02x\n", checksum);		// print on file checksum
	fprintf(filePtr, ":00000001ff\n");			// end of file string.
	printf("File hexFormatProgram.txt created.\n");
	fclose(filePtr);
	return 0;
}


// FUNCTIONS DEFINITION
int acquireInstruction(FILE* filePtr, struct instruction *instructionToSet)	{	// -> will be used instead of . to dereference the pointer and access it
	fscanf(filePtr, "%d", &instructionToSet->numOperands);			// first parameter in file is number of operands
	fscanf(filePtr, "%s", instructionToSet->name);					// second parameter in file is name of instruction
	for (int i=0; i< instructionToSet->numOperands; i++) {
		fscanf(filePtr, "%d", &instructionToSet->maskOperand[i]);	// next parameters are masks and shifts relative to operands
        fscanf(filePtr, "%d", &instructionToSet->shiftOperand[i]);
	}
    fscanf(filePtr, "%d", &instructionToSet->opCode);				// next parameter is opcode
	fscanf(filePtr, "%d", &instructionToSet->shiftOpCode);			// last parameter is shift of opCode
	return 0;
}


int printInstruction(struct instruction instructionToSet) {
	printf("%u ", instructionToSet.numOperands);                // first parameter in file is number of operands
    printf("%s ", instructionToSet.name);						// second parameter in file is name of instruction
    for (int i=0; i<instructionToSet.numOperands; i++) {
        printf("%u ", instructionToSet.maskOperand[i]);     	// next parameters are masks and shifts relative to operands
        printf("%u ", instructionToSet.shiftOperand[i]);
        }
	printf("%u ", instructionToSet.opCode);                     // next parameter is opcode
	printf("%u ", instructionToSet.shiftOpCode);                // last parameter is shift of opCode
	return 0;
}


int swapData(short* hexToSwap) {
	union instrWord swapper;						// swapper is a temp variable used to swap bytes (2 with 2)
	char byteTemp;

	swapper.fullWord = *hexToSwap;					// loading entire word in swapper
	byteTemp = swapper.byteWord[1];					// swap first 2 bytes with other 2 bytes
	swapper.byteWord[1] = swapper.byteWord[0];
	swapper.byteWord[0] = byteTemp;
	*hexToSwap = swapper.fullWord;
	return 0;
}