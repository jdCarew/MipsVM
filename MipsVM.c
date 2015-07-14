#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define s_mask(instr) ((instr >> 21) & 0x1f)
#define t_mask(instr) ((instr >> 16) & 0x1f)
#define d_mask(instr) ((instr >> 11) & 0x1f)
#define half_mask(instr) (instr & 0xffff)
#define top_mask(instr) ((instr >>26) &0x3f)
#define bot_mask(instr) (instr & 0x7ff)

#define $s registers[s]
#define $t registers[t]
#define $d registers[d]

#define STACK_SIZE 256
#define gp 28
#define sp 29
#define fp 30
#define ra 31
#define GP registers[28]
#define SP registers[29]
#define FP registers[30]
#define RA registers[31]


static int stack[STACK_SIZE]; //contains the stack, starting at the bottom and building up?

uint32_t registers[32]; //contains the register states
uint32_t hi;
uint32_t lo;
uint32_t pc;//program counter, will point to next instruction
uint32_t *instructions; //contains the current instruction
uint32_t running=1;

//prints the state of the VM registers
void printReg()
{
	printf("pc: %08x\n",pc*4);
	printf("$00: 0x%08x $01: 0x%08x $02: 0x%08x $03: 0x%08x\n",registers[0],registers[1], registers[2], registers[3]);
	printf("$04: 0x%08x $05: 0x%08x $06: 0x%08x $07: 0x%08x\n",registers[4],registers[5], registers[6], registers[7]);
	printf("$08: 0x%08x $09: 0x%08x $10: 0x%08x $11: 0x%08x\n",registers[8],registers[9], registers[10], registers[11]);
	printf("$12: 0x%08x $13: 0x%08x $14: 0x%08x $15: 0x%08x\n",registers[12],registers[13], registers[14], registers[15]);
	printf("$16: 0x%08x $17: 0x%08x $18: 0x%08x $19: 0x%08x\n",registers[16],registers[17], registers[18], registers[19]);
	printf("$20: 0x%08x $21: 0x%08x $22: 0x%08x $23: 0x%08x\n",registers[20],registers[21], registers[22], registers[23]);
	printf("$24: 0x%08x $25: 0x%08x $26: 0x%08x $27: 0x%08x\n",registers[24],registers[25], registers[26], registers[27]);
	printf("$28: 0x%08x $29: 0x%08x $30: 0x%08x $31: 0x%08x\n",registers[28],registers[29], registers[30], registers[31]);
	printf("Hi: 0x%08x Lo: 0x%08x\n", hi, lo);
}

//prints the state of the VM stack
void printStack()
{
	for (int i=0; i<STACK_SIZE; i++){
		printf("%04d: %08x\n",i*4,stack[i]);
	}
}


int main(int argc, char** argv)
{
	if (argc <2){
		printf("usage: MipsVM source [$2] [$3]\n source must be a mips binary file\n optionally inlclude start values for 3 and 2\n");
		return 0;
	}
	if (argc >2){
		registers[2]=atoi(argv[2]);
	}
	if (argc >3){
		registers[3]=atoi(argv[3]);
	}
	FILE * file = fopen(argv[1], "rb");
	if (file ==NULL)
		return -1;
	
	fseek(file, 0L, SEEK_END);
	size_t instructionCount=ftell(file)/4;
	fseek(file, 0L, SEEK_SET);
	size_t n=0;
	uint32_t c;
	instructions=malloc(instructionCount);
	while ((c = fgetc(file)) != EOF){
		instructions[n] = (uint32_t)c <<24;
		c=fgetc(file);
		instructions[n] += (uint32_t)c <<16;
		c=fgetc(file);
		instructions[n] += (uint32_t)c <<8;
		c=fgetc(file);
		instructions[n] += (uint32_t)c;
		n++;
	}
	while (running){
		uint32_t s=s_mask(instructions[pc]);
        uint32_t t=t_mask(instructions[pc]);
        uint32_t d=d_mask(instructions[pc]);
        uint32_t i=half_mask(instructions[pc]);
		if (bot_mask(instructions[pc]) == 0x20 && top_mask(instructions[pc]) == 0){
			//add
			if (d!=0)
				$d=$s+$t;
		}
		else if (bot_mask(instructions[pc]) == 0x22 && top_mask(instructions[pc])== 0){
			//sub
            if (d!=0)
				$d=$s-$t;
		}
		else if (half_mask(instructions[pc]) == 0x18 && top_mask(instructions[pc]) == 0){
			//mult
			int64_t ans = (int32_t) $s * (int32_t) $t;
			hi = (uint32_t)(ans>>32);
			lo = (uint32_t)(ans & 0xffffffff);
		}
		else if (half_mask(instructions[pc]) == 0x19 && top_mask(instructions[pc]) == 0){
			//multu
			uint64_t ans = $s * $t;
			hi=(uint32_t)(ans>>32);
			lo=(uint32_t)(ans &0xffffffff);
		}
		else if (half_mask(instructions[pc]) == 0x1A && top_mask(instructions[pc]) == 0){
			//div
			lo=(uint32_t) ((int32_t)$s / (int32_t)$t);
			hi=(uint32_t) ((int32_t)$s % (int32_t)$t);
		}
		else if (half_mask(instructions[pc]) == 0x1B && top_mask(instructions[pc]) == 0){
			//divu
			lo=$s / $t;
			hi=$s % $t;
		}
		else if (bot_mask(instructions[pc]) == 0x10 && (instructions[pc] & 0xffff0000) == 0){
			//mfhi
			if (d!=0)
				$d=hi;
		}
		else if (bot_mask(instructions[pc]) == 0x12 && (instructions[pc] & 0xffff0000) == 0){
			//mflo
			if (d!=0){
				$d=lo;
			}
		}
		else if (bot_mask(instructions[pc]) == 0x14 && (instructions[pc] & 0xffff0000) == 0){
			//lis
			pc++;
			$d=instructions[pc];
		}
		else if (top_mask(instructions[pc]) == 0x2A){
			//lw
			i=half_mask(instructions[pc]);
			$t=stack[$s+i];
		}
        else if (top_mask(instructions[pc]) == 0x2B){
            //sw
            stack[$s+i]=$t;
        }
        else if (bot_mask(instructions[pc]) == 0x2A && top_mask(instructions[pc]) == 0){
            //slt
			if ((int32_t)$s< (int32_t)$t){
				$d=1;
			}else{
				$d=0;	
			}
        }
        else if (bot_mask(instructions[pc]) == 0x2B && top_mask(instructions[pc]) == 0){
            //sltu
            if ($s< $t){
                $d=1;
            }else{
                $d=0;
            }

        }
        else if (top_mask(instructions[pc]) == 0x4){
            //beq
			if ($s == $t){
				pc+=i;//not that this is not multiplied by 4 because my pc is only incrementing by 1
			}
        }
        else if (top_mask(instructions[pc]) == 0x5){
            //bne
            if ($s != $t){
                pc+=i;//not that this is not multiplied by 4 because my pc is only incrementing by 1
            }
        }
        else if ((instructions[pc] & 0x1fffff) == 0x8 && top_mask(instructions[pc]) == 0){
            //jr
			pc = $s/4;
			if (pc == 0){
				running=0;
			}
        }
        else if ((instructions[pc] & 0x1fffff) == 0x9 && top_mask(instructions[pc]) == 0){
            //jalr
			uint32_t temp=$s;
			registers[31]=pc*4;
			pc=temp;
        }
		pc++;
		if (pc > instructionCount){
			running=0;
		} 
	}

	
	printReg();
//	printStack(); //This is just here for debugging
	return 0;
}


