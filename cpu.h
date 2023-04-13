#ifndef _CPU_H_
#define _CPU_H_
#include <stdbool.h>
#include <assert.h>

typedef struct Register
{
    int value;          // contains register value

} Register;

/* Model of CPU */
typedef struct CPU
{
	/* Integer register file */
	Register *regs;
	int PC;
	int *memory;
    int total_instructions;
} CPU;

typedef enum {
    SET, ADD, SUB, MUL, DIV, LD, ST, BEZ, BGEZ, BLEZ, BGTZ, BLTZ, RET
} Opcode;

typedef struct {
    Opcode opcode;
    int dest;
    int left_operand;
    int right_operand;
} Instruction;


typedef struct {
    Instruction instruction;
    int state;
} Stage;

typedef struct {
    Stage IF;
    Stage ID;
    Stage IA;
    Stage RR;
    Stage Add;
    Stage Mul;
    Stage Div;
    Stage BR;
    Stage Mem1;
    Stage Mem2;
    Stage WB;
} Pipeline;

typedef struct {
    int btb[16];
    int pt[16];
} Branch_Predictor;


CPU*
CPU_init();

Register*
create_registers(int size);

int
CPU_run(CPU* cpu);

void
CPU_stop(CPU* cpu);

void initialize_pipeline(Pipeline* pipeline);
void initialize_branch_predictor(Branch_Predictor* bp);
void execute_WB_stage(CPU* cpu, Pipeline* pipeline);
void execute_Mem2_stage(CPU* cpu, Pipeline* pipeline);
void execute_Mem1_stage(CPU* cpu, Pipeline* pipeline);
void execute_BR_stage(CPU* cpu, Pipeline* pipeline, Branch_Predictor* bp);
void execute_Div_stage(CPU* cpu, Pipeline* pipeline);
void execute_Mul_stage(CPU* cpu, Pipeline* pipeline);
void execute_Add_stage(CPU* cpu, Pipeline* pipeline);
void execute_RR_stage(CPU* cpu, Pipeline* pipeline);
void execute_IA_stage(CPU* cpu, Pipeline* pipeline);
void execute_ID_stage(CPU* cpu, Pipeline* pipeline);
int execute_IF_stage(CPU* cpu, Pipeline* pipeline, Branch_Predictor* bp);
int is_pipeline_empty(Pipeline* pipeline);
void clear_pipeline_stages(Pipeline* pipeline);
void load_instruction_file(CPU *cpu, const char *filename);
void load_memory_map_file(CPU *cpu, const char *filename);

#endif
