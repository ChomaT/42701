#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}



/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/

	uint32_t instr = mem_read_32(CURRENT_STATE.PC);
	uint32_t msb_6_mask = 0xFC000000; 	// 1111 1100 0000 0000 0000 0000 0000 0000
	uint32_t lsb_6_mask = 0x0000003F; 	// 0000 0000 0000 0000 0000 0000 0011 1111
	uint32_t rs_5_mask = 0x03E00000;  	// 0000 0011 1110 0000 0000 0000 0000 0000
	uint32_t rt_5_mask = 0x001F0000;	// 0000 0000 0001 1111 0000 0000 0000 0000
	uint32_t immediate_mask = 0x0000FFFF; 	//0000 0000 0000 0000 1111 1111 1111 1111	
	uint32_t rd_mask = 0x0000F800; 		//0000 0000 0000 0000 1111 1000 0000 0000
	uint32_t sa_mask = 0x000007C0; 		//0000 0000 0000 0000 0000 0111 1100 0000
	uint32_t sign_mask = 0x80000000; //1000 0000 0000 0000 0000 0000 0000 0000
    uint32_t sign_mask_2 = 0x00008000; //0000 0000 0000 0000 1000 0000 0000 0000
    uint32_t offset_mask = 0x0000FFFF; //0000 0000 0000 0000 1111 1111 1111 1111
    uint32_t base_mask = 0x03E00000; //0000 0011 1110 0000 0000 0000 0000 0000

	uint32_t top6 = (instr & msb_6_mask) >> 26;
	uint32_t low6 = instr & lsb_6_mask;
	uint32_t rs = (instr & rs_5_mask) >> 21;
	uint32_t rt = (instr & rt_5_mask) >> 16;
	uint32_t rd = (instr & rd_mask) >> 11;
	uint32_t sa = (instr & sa_mask) >> 6;
	uint32_t immediate = instr & immediate_mask;
	uint32_t temp = 0;
	uint32_t sign = (CURRENT_STATE.REGS[rt] & sign_mask) >> 31;
    uint32_t offset = instr & offset_mask;
    uint32_t sign_2 = (offset & sign_mask_2) >> 15;
    uint32_t base = (instr & base_mask) >> 21;
	uint32_t op1;
	uint32_t op2;
	
	printf("top6: %x\n", top6);
	printf("reg: %x\n", CURRENT_STATE.REGS[2]);

	if(top6 == 0) { 

		if(0x08 == low6) {						// JR Jump Reg.
			// p548

		} 
		
		if(0x09 == low6) { 						// JALR 
			// Jump and Link Register p547
		}
		
		if(low6 == 0xC) {						//SYSCALL
			if(CURRENT_STATE.REGS[2] == 0xA)
			{
				RUN_FLAG = FALSE;
			}
		}

		if(0x18 == low6) { 						//MULT
			// p.586
			// {HI, LO} = rs*rt
			// Is this right? Shift the high order bits over?
			uint64_t mul_tmp = CURRENT_STATE.REGS[rs] * CURRENT_STATE.REGS[rt];
			NEXT_STATE.HI = (mul_tmp & 0xFFFF0000) >> 16;
			NEXT_STATE.LO = (mul_tmp & 0x0000FFFF);
		}
		
		if(0x19 == low6) {						//MULTU p588
			// Same as MULT (for now?)
			uint64_t mul_tmp = CURRENT_STATE.REGS[rs] * CURRENT_STATE.REGS[rt];
			NEXT_STATE.HI = (mul_tmp & 0xFFFF0000) >> 16;
			NEXT_STATE.LO = (mul_tmp & 0x0000FFFF);
		}

		if(low6 == 0x1A){						//DIV
			if(CURRENT_STATE.REGS[rt] == 0x0)
			{
				printf("CANNOT DIVIDE BY 0\n");
				//Flag "Z"
			}
			else
			{
				op1 = CURRENT_STATE.REGS[rs]; 	
				op2 = CURRENT_STATE.REGS[rt]; 

				temp = op1 / op2;
				NEXT_STATE.LO = temp; 		//place result in LO				
				
				temp = op1 % op2;
				NEXT_STATE.HI = temp;		//place remainder in HI
			}
		}

		if(low6 == 0x1B){						//DIVU
			if(CURRENT_STATE.REGS[rt] == 0x0)
			{
				printf("CANNOT DIVIDE BY 0\n");
				//Flag "Z"
			}
			else
			{
				op1 = CURRENT_STATE.REGS[rs]; 	
				op2 = CURRENT_STATE.REGS[rt]; 

				temp = op1 / op2;
				NEXT_STATE.LO = temp; 		//place result in LO				
				
				temp = op1 % op2;
				NEXT_STATE.HI = temp;		//place remainder in HI
			}
		}

		if(0x20 == low6) { 					  	//ADD
			// b10 0000 
			// ADD rd = rs+rt
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
		}

		if(0x21 == low6) { 						//ADDU
			// rd = rs + rt 
			// No overflow
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
		}

		if(0x22 == low6) { 						//SUB
			// rd = rs - rt
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
		}

		if(0x23 == low6) { 						//SUBU
			// rd = rs - rt p.619
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
		}
		
		if(low6 == 0x24){						//AND
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];

		}
		
		if(low6 == 0x25){						//OR
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
		}

		if(low6 == 39){							//NOR
			NEXT_STATE.REGS[rd] = ~ (CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
		}
		
		if(low6 == 0x26){						//XOR
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ (0 | CURRENT_STATE.REGS[rt]);
		}

		if(low6 == 42){							//SLT
			if(CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt])
			{
				NEXT_STATE.REGS[rd] = 1;
			}
			else 
			{
				NEXT_STATE.REGS[rd] = 0;
			}
		}

		if(low6 == 0){							//SLL
			NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rt] << sa) | 0;
		}

		if(low6 == 2){							//SRL
			NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rt] >> sa) | 0;
		}
		
		if(low6 == 3){							//SRA
			if (sign == 1){
                		uint32_t value = (0xFFFFFFFF << (32 - sa)) | 0;
				NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rt] >> sa) | value;
            		}
		    	else 
		   	{
		        	NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rt] >> sa) | 0;
		    	}
		}

		if(low6 == 0x10){						//MFHI
			NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
		}

		if(low6 == 0x12){						//MFLO
			NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
		}

		if(low6 == 0x11){						//MTHI
			NEXT_STATE.HI = CURRENT_STATE.REGS[rs];
		}

		if(low6 == 0x13){						//MTLO
			NEXT_STATE.LO = CURRENT_STATE.REGS[rs];
		}
	}

	if(0x01 == top6) {							//

		if(0x00 == rt) {	// BLTZ Branch Less Than Zero
			//p503
		}
		else if(0x01 == rt) {	// BGEZ Branch Greater Than or Eq Zero
			//p495

		}
	}

	if(0x02 == top6) {		// J Jump
		// P545


	}

	if(0x03 == top6) {		// JAL Jump and Link
		// P546
	}


	if(0x07 == top6) { 		// BGTZ Branch On Greater than Zero
		// p499
	}

	if(0x08 == top6) { 							// ADDI
		// b00 1000
		// rt = rs + immediate
		// Sign extend - move 15th bit (sign bit) to MSB of 32-bit
		uint32_t immediate  = (instr & 0x00008000) << 16; // 0000 0000 0000 0000 1000 0000 0000 0000
		immediate = immediate & (instr & 0x0000EFFF); // Get everything but sign bit
		NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + immediate;
	}

	if(0x09 == top6) { 							// ADDIU
		// rt = immediate + rs 
		uint32_t immediate  = (instr & 0x00008000) << 16; // 0000 0000 0000 0000 1000 0000 0000 0000
		immediate = immediate & (instr & 0x0000EFFF); // Get everything but sign bit
		NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + immediate;
	}

	if(top6 == 10){								//SLTI
		if(CURRENT_STATE.REGS[rs] < immediate)
		{
			NEXT_STATE.REGS[rt] = 1;
		}
		else 
		{
			NEXT_STATE.REGS[rt] = 0;
		}	

	}

	if(top6 == 14){								//XORI
		NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] ^ (0 | immediate);
	}

	if(top6 == 0x0C){							//ANDI
		NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & immediate;
	}
	
	if(top6 == 0x0D){							//ORI
		NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | immediate;
	}
    
    if(top6 == 15) {                            //LUI
				NEXT_STATE.REGS[rt] = (immediate << 16) | 0;
	}
    
    if(top6 == 32) {                            //LB
        if (sign_2 == 1){
            offset = (offset | 0xFFFF0000) + CURRENT_STATE.REGS[base];
            NEXT_STATE.REGS[rt] = mem_read_32(offset) | 0xFF;
        }    
        if (sign_2 == 0){
            offset = (offset | 0) + CURRENT_STATE.REGS[base];
            NEXT_STATE.REGS[rt] = mem_read_32(offset) | 0xFF;
        }    
    }
    
    if(top6 == 33) {                            //LH
        if (sign_2 == 1){
            offset = (offset | 0xFFFF0000) + CURRENT_STATE.REGS[base];
            NEXT_STATE.REGS[rt] = mem_read_32(offset) | 0xFFFF;
        }    
        if (sign_2 == 0){
            offset = (offset | 0) + CURRENT_STATE.REGS[base];
            NEXT_STATE.REGS[rt] = mem_read_32(offset) | 0xFFFF;
        }    
    }
    
    if(top6 == 35) {                            //LW
        if (sign_2 == 1){
            offset = (offset | 0xFFFF0000) + CURRENT_STATE.REGS[base];
            NEXT_STATE.REGS[rt] = mem_read_32(offset);
        }    
        if (sign_2 == 0){
            offset = (offset | 0) + CURRENT_STATE.REGS[base];
            NEXT_STATE.REGS[rt] = mem_read_32(offset);
        }    
    }
    
    if(top6 == 40) {                            //SB
        if (sign_2 == 1){
            offset = (offset | 0xFFFF0000) + CURRENT_STATE.REGS[base];
            uint32_t temp = CURRENT_STATE.REGS[rt] | 0xFF;
            mem_write_32(offset, temp);
        }    
        if (sign_2 == 0){
            offset = (offset | 0) + CURRENT_STATE.REGS[base];
            uint32_t temp = CURRENT_STATE.REGS[rt] | 0xFF;
            mem_write_32(offset, temp);
        }    
    }
    
    if(top6 == 41) {                            //SH
        if (sign_2 == 1){
            offset = (offset | 0xFFFF0000) + CURRENT_STATE.REGS[base];
            uint32_t temp = CURRENT_STATE.REGS[rt] | 0xFFFF;
            mem_write_32(offset, temp);
        }    
        if (sign_2 == 0){
            offset = (offset | 0) + CURRENT_STATE.REGS[base];
            uint32_t temp = CURRENT_STATE.REGS[rt] | 0xFFFF;
            mem_write_32(offset, temp);
        }    
    }
	
    if(top6 == 43) {                            //SW
        if (sign_2 == 1){
            offset = (offset | 0xFFFF0000) + CURRENT_STATE.REGS[base];
            mem_write_32(offset, CURRENT_STATE.REGS[rt]);
        }    
        if (sign_2 == 0){
            offset = (offset | 0) + CURRENT_STATE.REGS[base];
            mem_write_32(offset, CURRENT_STATE.REGS[rt]);
        }    
    }

	NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	/*IMPLEMENT THIS*/
	int i;
	for(i=0; i<PROGRAM_SIZE*4; i+=4) {
		uint32_t instr = mem_read_32(MEM_TEXT_BEGIN + i);
		printf("%x\n", instr); 
	}
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
