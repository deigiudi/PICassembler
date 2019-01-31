/*

This disassembler is made for pic16f627a.

	/// FILES REQUIRED ///
	In order to launch the program correctly, 2 files needs to be passed as argument:
	 - source file: 		a file containing hex coded instructions to translate.
	 - destination file: 	a file that will contain instructions translated.
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FMASK  0x007f				// mask used for f variable. 					Corresponds to (00)00 0000 0111 1111
#define DMASK  0x0001				// mask used for d variable. 					Corresponds to (00)00 0000 0000 0001
#define BMASK  0x0007				// mask used for b variable.					Corresponds to (00)00 0000 0000 0111
#define KMASKS 0x00ff				// mask used for k variable, short version. 	Corresponds to (00)00 0000 1111 1111
#define KMASKL 0x07ff 				// mask used for k variable, long version.  	Corresponds to (00)00 0111 1111 1111

int fprintInstr(int, FILE*);		// argument are instruction to disassemble and destination file
int swapData(short*);				// function used to swap bytes of data

union instrWord {					// each word of 2 bytes corresponds to an instruction
	short fullWord;					// fullWord is used to access to the full word
	char byteWord[2];				// byteWord[0] and byteWord[1] are used to access first byte and second byte of word
};


int main (int argc, char* argv[]){
	FILE *sourceFilePtr;
	FILE *destFilePtr;

	int i;
	char hexString[50];				// this will contain hex coded instructions
	char stringTemp[5];				// this will be used to store characters that needs to be converted in numbers
	int numBytes;					// this will contain first field of hex coded instruction (number of data bytes)
	int dataType;
	int numCharData;
	short oneData;					// this will contain one instruction uncoded (14-bit opcode)


/****************		Check if files could be opened		***************/

	if ((sourceFilePtr=fopen(argv[1], "r"))==NULL)	{
		printf("Insert a valid source file as FIRST argument.\nInsert a valid destination file as SECOND argument.\n");
		return 15;
	}

	if ((destFilePtr=fopen(argv[2], "w"))==NULL)	{
		printf("Insert a valid destination file as SECOND argument.\n");
		return 16;
	}


/****************		Instructions processing			*************/

	while(fscanf(sourceFilePtr, "%s", hexString) != EOF) {		// examine one hex coded instruction
		strncpy(stringTemp, hexString+1, 2);					// extract second and third hex numbers
		stringTemp[2]='\0';
        numBytes = (int)strtol(stringTemp, NULL, 16);			// numBytes acquired

		strncpy(stringTemp, hexString+7, 2);					// extract eigth and ninth hex numbers
		stringTemp[2]='\0';
		dataType = (int)strtol(stringTemp, NULL, 16);			// data type acquired

		if(dataType == 1) {
			fprintf(destFilePtr, "%s\n", "END");				// dataType equal to 01 corresponds to END
		}

		else if (dataType == 0) {								// dataType equal to 00 corresponds to data
			numCharData = numBytes*2;							// each data is made of 2 bytes, so each data is made of 4 hex characters
			for(i=9; i < (numCharData+9); i=i+4) {				// i starts from 9, which is when data field begins	:02400e00183f59
				// i ends at index numCharData+9
				// i is incremented by 4 because 1 data occupies 
				strncpy(stringTemp, hexString+i, 4);			// extract data words (4 bytes each)
				stringTemp[4]='\n';
				oneData = (short)strtol(stringTemp, NULL, 16);	// dataWord acquired (now is no more a string)
				swapData(&oneData);								// swapping bytes (required due to implementation in pic16f627a)
				fprintInstr(oneData, destFilePtr);
			}
		}
	}

	fclose(sourceFilePtr);
	fclose(destFilePtr);

	printf("Destination file %s created.\n", argv[2]);
	return 0;
}


// FUNCTIONS DEFINITION

int fprintInstr(int instr, FILE* filePtr){
	int f, d, b, k;				// the letters used for these variables are the same as in the data sheet
	// first I check if instr is without parameters (constant)
	if (instr == 0x0100) 		fprintf(filePtr, "%s\n", "clrw");	// CLEAR W
	else if(instr == 0x0000) 	fprintf(filePtr, "%s\n", "nop");	// NO OPERATION
	else if(instr == 0x0064)	fprintf(filePtr, "%s\n", "clrwdt");	// CLEAR WATCHDOG TIMER
	else if(instr == 0x0009)	fprintf(filePtr, "%s\n", "retfie");	// RETURN FROM INTERRUPT
	else if(instr == 0x0008)	fprintf(filePtr, "%s\n", "return");	// RETURN FROM SUBROUTINE
	else if(instr == 0x0063)	fprintf(filePtr, "%s\n", "sleep");	// GO IN STANDBY MODE
	else {
		switch (instr>>12) {
			case 0x0:			// byte-oriented register file operations
				if ((instr>>8) == 7) {			// ADD W and F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "addwf", f, d);
				}
				else if ((instr>>8) == 5) {		// AND W with F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "andwf", f, d);
				}
				else if ((instr>>8) == 1) {		// CLEAR F
					f = instr&(FMASK);
					fprintf(filePtr, "%s .%d\n", "clrf", f);
				}
				else if ((instr>>8) == 9) {		// COMPLEMENT F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "comf", f, d);					
				}
				else if ((instr>>8) == 3) {		// DECREMENT F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "decf", f, d);					
				}
				else if ((instr>>8) == 11) {	// DECREMENT F, SKIP IF 0
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "decfsz", f, d);					
				}
				else if ((instr>>8) == 10) {	// INCREMENT F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "incf", f, d);					
				}
				else if ((instr>>8) == 15) {	// INCREMENT F, SKIP IF 0
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "incfsz", f, d);					
				}
				else if ((instr>>8) == 4){		// INCLUSIVE OR W with F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "iorwf", f, d);					
				}
				else if ((instr>>8) == 8){		// MOVE F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "movf", f, d);					
				}
				else if ((instr>>8) == 0){		// MOVE W TO F
					f = instr&(FMASK);
					fprintf(filePtr, "%s .%d\n", "movwf", f);
				}
				else if ((instr>>8) == 13){		// ROTATE LEFT F TRHROUGH CARRY
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "rlf", f, d);					
				}
				else if ((instr>>8) == 12){		// ROTATE RIGHT F THROUGH CARRY
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "rrf", f, d);					
				}
				else if ((instr>>8) == 2){		// SUBTRACT W from F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "subwf", f, d);					
				}
				else if ((instr>>8) == 14){		// SWAP NIBBLES IN F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "swapf", f, d);					
				}
				else if ((instr>>8) == 6){		// EXCLUSIVE OR W with F
					f = instr&(FMASK);
					d = (instr>>7)&(DMASK);
					fprintf(filePtr, "%s .%d,.%d\n", "xorwf", f, d);					
				}
				break;
			case 0x1:			// bit-oriented register file operations
				if ((instr>>10) == 4){		// BIT CLEAR F
					f = instr&(FMASK);
					b = (instr>>7)&(BMASK);	
					fprintf(filePtr, "%s .%d,.%d\n", "bcf", f, b);
				}
				else if((instr>>10) == 5){	// BIT SET F
					f = instr&(FMASK);
					b = (instr>>7)&(BMASK);	
					fprintf(filePtr, "%s .%d,.%d\n", "bsf", f, b);
				}
				else if((instr>>10) == 6){	// BIT TEST F, SKIP IF CLEAR
					f = instr&(FMASK);
					b = (instr>>7)&(BMASK);	
					fprintf(filePtr, "%s .%d,.%d\n", "btfsc", f, b);
				}
				else if((instr>>10) == 7){	// BIT TEST F, SKIP IF SET
					f = instr&(FMASK);
					b = (instr>>7)&(BMASK);	
					fprintf(filePtr, "%s .%d,.%d\n", "btfss", f, b);
				}
				break;
			case 0x2:			// literal and control operations

				if ((instr>>11) == 4){		// CALL SUBROUTINE
					k = instr&(KMASKL);
					fprintf(filePtr, "%s .%d\n", "call", k);
				}
				else if ((instr>>11) == 5){	// GO TO ADDRESS
					k = instr&(KMASKL);
					fprintf(filePtr, "%s .%d\n", "goto", k);
				}
				break;
			case 0x3:			// literal and control operations
				if ((instr>>9) == 0x1f){	// ADD LITERAL and W
					k = instr&(KMASKS);
					fprintf(filePtr, "%s .%d\n", "addlw", k);
				}
				else if((instr>>8) == 0x39){	// AND LITERAL with W
					k = instr&(KMASKS);
					fprintf(filePtr, "%s .%d\n", "andlw", k);
				}
				else if((instr>>8) == 0x38){	// INCLUSIVE OR LITERAL with W
					k = instr&(KMASKS);
					fprintf(filePtr, "%s .%d\n", "iorlw", k);
				}
				else if((instr>>10) == 0xc){	// MOVE LITERAL to W
					k = instr&(KMASKS);
					fprintf(filePtr, "%s .%d\n", "movlw", k);
				}
				else if((instr>>10) == 0xd){	// RETURN WITH LITERAL in W
					k = instr&(KMASKS);
					fprintf(filePtr, "%s .%d\n", "retlw", k);
				}
				else if((instr>>9) == 0x1e){	// SUBTRACT W from LITERAL
					k = instr&(KMASKS);
					fprintf(filePtr, "%s .%d\n", "sublw", k);
				}
				else if((instr>>8) == 0x3a){	// EXCLUSIVE OR LITERAL with W
					k = instr&(KMASKS);
					fprintf(filePtr, "%s .%d\n", "xorlw", k);
				}
				break;
			default:
				break;
		}
	}
	return 0;
}


int swapData(short* hexToSwap) {
	union instrWord swapper;			// swapper is a temp variable used to swap bytes (2 with 2)
	char byteTemp;

	swapper.fullWord = *hexToSwap;				// loading entire word in swapper
	byteTemp = swapper.byteWord[1];				// swap first 2 bytes with other 2 bytes
	swapper.byteWord[1] = swapper.byteWord[0];
	swapper.byteWord[0] = byteTemp;

	*hexToSwap = swapper.fullWord;
	return 0;
}