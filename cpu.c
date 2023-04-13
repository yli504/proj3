
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cpu.h"

#define REG_COUNT 16
 
CPU*
CPU_init()
{
    CPU* cpu = malloc(sizeof(*cpu));
    if (!cpu) {
        return NULL;
    }

    /* Create register files */
    cpu->regs= create_registers(REG_COUNT);
    cpu->PC = 0;
    cpu->total_instructions = 0;
    cpu->memory = (int*)malloc(65536 * sizeof(int));

    return cpu;
}

/*
 * This function de-allocates CPU cpu.
 */
void
CPU_stop(CPU* cpu)
{
    free(cpu->regs);
    free(cpu->memory);
    free(cpu);
}

/*
 * This function prints the content of the registers.
 */
void
print_registers(CPU *cpu){
    
    
    printf("================================\n\n");

    printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n\n");

    printf("--------------------------------\n");
    for (int reg=0; reg<REG_COUNT; reg++) {
        printf("REG[%2d]   |   Value=%d  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n\n");
}

void print_display(CPU *cpu, int cycle){
    printf("================================\n");
    printf("Clock Cycle #: %d\n", cycle);
    printf("--------------------------------\n");

   for (int reg=0; reg<REG_COUNT; reg++) {
       
        printf("REG[%2d]   |   Value=%d  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n");
    printf("\n");

}

/*
 *  CPU CPU simulation loop
 */
int
CPU_run(CPU* cpu)
{
    

    Pipeline pipeline;
    Branch_Predictor bp;
    int cycle = 0;
    int total_instructions = 0;
    int stalled_cycles = 0;
    bool instructions_loaded = false; // Add this line

    cpu->total_instructions = 0;

    initialize_pipeline(&pipeline);
    initialize_branch_predictor(&bp);

    while (!instructions_loaded || !is_pipeline_empty(&pipeline)) {
        cycle++;
        printf("Cycle: %d\n", cycle);
        execute_WB_stage(cpu, &pipeline);
        execute_Mem2_stage(cpu, &pipeline);
        execute_Mem1_stage(cpu, &pipeline);
        execute_BR_stage(cpu, &pipeline, &bp);
        execute_Div_stage(cpu, &pipeline);
        execute_Mul_stage(cpu, &pipeline);
        execute_Add_stage(cpu, &pipeline);
        execute_RR_stage(cpu, &pipeline);
        execute_IA_stage(cpu, &pipeline);
        execute_ID_stage(cpu, &pipeline);
        int stall = execute_IF_stage(cpu, &pipeline, &bp);
        stalled_cycles += stall;

        clear_pipeline_stages(&pipeline);

        total_instructions += (pipeline.WB.state == 1);

        if (cpu->PC >= cpu->total_instructions) {
            instructions_loaded = true; // Set to true when all instructions are loaded into the pipeline
        }

        print_display(cpu, cycle);
    }

    

    printf("Stalled cycles due to data hazard: %d\n", stalled_cycles);
    printf("Total execution cycles: %d\n", cycle);
    printf("Total instruction simulated: %d\n", total_instructions);
    printf("IPC: %f\n", (float) total_instructions / cycle);

    return 0;
}
//print_display(cpu,0);

Register*
create_registers(int size){
    Register* regs = malloc(sizeof(*regs) * size);
    if (!regs) {
        return NULL;
    }
    for (int i=0; i<size; i++){
        regs[i].value = 0;
    }
    return regs;
}


void initialize_pipeline(Pipeline* pipeline) {
    // Initialize pipeline stages
    memset(pipeline, 0, sizeof(Pipeline));
    pipeline->IF.state = 1;
    pipeline->ID.state = 1;
    pipeline->IA.state = 1;
    pipeline->RR.state = 1;
    pipeline->Add.state = 1;
    pipeline->Mul.state = 1;
    pipeline->Div.state = 1;
    pipeline->BR.state = 1;
    pipeline->Mem1.state = 1;
    pipeline->Mem2.state = 1;
    pipeline->WB.state = 1;
}

void initialize_branch_predictor(Branch_Predictor* bp) {
    // Initialize branch predictor
    memset(bp->btb, 0, sizeof(bp->btb));
    memset(bp->pt, 0, sizeof(bp->pt));
}

void execute_WB_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Writeback stage
    if (pipeline->WB.state == 1) {
        Instruction *instr = &pipeline->WB.instruction;
        cpu->regs[instr->dest].value = instr->right_operand;

    }
}

void execute_Mem2_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Memory stage 2
    if (pipeline->Mem2.state == 1) {
        Instruction *instr = &pipeline->Mem2.instruction;
        if (instr->opcode == LD) {
            instr->right_operand = cpu->memory[instr->right_operand];
        }
    }
    pipeline->WB = pipeline->Mem2;
}

void execute_Mem1_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Memory stage 1
    if (pipeline->Mem1.state == 1) {
        Instruction *instr = &pipeline->Mem1.instruction;
        if (instr->opcode == ST) {
            cpu->memory[instr->left_operand] = instr->right_operand;
        }
    }
    pipeline->Mem2 = pipeline->Mem1;
}

void execute_BR_stage(CPU* cpu, Pipeline* pipeline, Branch_Predictor* bp) {
    // Execute Branch stage
    if (pipeline->BR.state == 1) {
        Instruction *instr = &pipeline->BR.instruction;
        switch (instr->opcode) {
            case BEZ:
                if (cpu->regs[instr->dest].value == 0) {
                    cpu->PC = instr->left_operand;
                }
                break;
            case BGEZ:
                if (cpu->regs[instr->dest].value >= 0) {
                    cpu->PC = instr->left_operand;
                }
                break;
            case BLEZ:
                if (cpu->regs[instr->dest].value <= 0) {
                    cpu->PC = instr->left_operand;
                }
                break;
            case BGTZ:
                if (cpu->regs[instr->dest].value > 0) {
                    cpu->PC = instr->left_operand;
                }
                break;
            case BLTZ:
                if (cpu->regs[instr->dest].value < 0) {
                    cpu->PC = instr->left_operand;
                }
                break;
            default:
                break;
        }
    }
    pipeline->Mem1 = pipeline->BR;
}

void execute_Div_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Division stage
    if (pipeline->Div.state == 1) {
        Instruction *instr = &pipeline->Div.instruction;
        if (instr->opcode == DIV) {
            instr->right_operand = instr->left_operand / instr->right_operand;
        }
    }
    pipeline->BR = pipeline->Div;
}

void execute_Mul_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Multiplication stage
    if (pipeline->Mul.state == 1) {
        Instruction *instr = &pipeline->Mul.instruction;
        if (instr->opcode == MUL) {
            instr->right_operand = instr->left_operand * instr->right_operand;
        }
    }
    pipeline->Div = pipeline->Mul;
}

void execute_Add_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Addition stage
    if (pipeline->Add.state == 1) {
        Instruction *instr = &pipeline->Add.instruction;
        if (instr->opcode == ADD) {
            instr->right_operand = instr->left_operand + instr->right_operand;
        } else if (instr->opcode == SUB) {
            instr->right_operand = instr->left_operand - instr->right_operand;
        }
    }
    pipeline->Mul = pipeline->Add;
}

void execute_RR_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Register Read stage
    if (pipeline->RR.state == 1) {
        Instruction *instr = &pipeline->RR.instruction;
        if (instr->opcode != LD && instr->opcode != ST && instr->opcode != SET) {
            instr->left_operand = cpu->regs[instr->left_operand].value;
            instr->right_operand = cpu->regs[instr->right_operand].value;

        }
    }
    pipeline->Add = pipeline->RR;
}

void execute_IA_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Instruction Alignment stage
    if (pipeline->IA.state == 1) {
        Instruction *instr = &pipeline->IA.instruction;
        if (instr->opcode == SET || instr->opcode == ADD || instr->opcode == SUB || instr->opcode == MUL || instr->opcode == DIV) {
            instr->right_operand = instr->left_operand;
        }
    }
    pipeline->RR = pipeline->IA;
}

void execute_ID_stage(CPU* cpu, Pipeline* pipeline) {
    // Execute Instruction Decode stage
    if (pipeline->ID.state == 1) {
        Instruction instr;
        int opcode = pipeline->ID.instruction.opcode;
        instr.dest = pipeline->ID.instruction.dest;
        instr.left_operand = pipeline->ID.instruction.left_operand;
        instr.right_operand = pipeline->ID.instruction.right_operand;

        switch (opcode) {
            case SET:
                instr.opcode = SET;
                break;
            case ADD:
                instr.opcode = ADD;
                break;
            case SUB:
                instr.opcode = SUB;
                break;
            case MUL:
                instr.opcode = MUL;
                break;
            case DIV:
                instr.opcode = DIV;
                break;
            case LD:
                instr.opcode = LD;
                break;
            case ST:
                instr.opcode = ST;
                break;
            case BEZ:
                instr.opcode = BEZ;
                break;
            case BGEZ:
                instr.opcode = BGEZ;
                break;
            case BLEZ:
                instr.opcode = BLEZ;
                break;
            case BGTZ:
                instr.opcode = BGTZ;
                break;
            case BLTZ:
                instr.opcode = BLTZ;
                break;
            case RET:
                instr.opcode = RET;
                break;
            default:
                printf("Invalid opcode\n");
                break;
        }
        pipeline->IA.instruction = instr;
    }
    pipeline->IA.state = pipeline->ID.state;
}

int is_control_instruction(int instr) {
    Instruction *instruction = (Instruction *)&instr;
    switch (instruction->opcode) {
        case BEZ:
        case BGEZ:
        case BLEZ:
        case BGTZ:
        case BLTZ:
            return 1;
        default:
            return 0;
    }
}

int check_and_update_branch_predictor(Branch_Predictor *bp, int pc, int instr) {
    int index = (pc / 4) % 16; // Simple modulo indexing for the branch predictor

    if (bp->pt[index] > 1) {
        // Branch is predicted as taken
        return 1;
    } else {
        // Branch is predicted as not taken
        return 0;
    }
}

int get_target_address(int pc, int instr) {
    Instruction *instruction = (Instruction *)&instr;
    int offset = instruction->left_operand;
    return pc + 4 + offset * 4;
}

int is_data_hazard(CPU *cpu, Pipeline *pipeline, int instr) {
    Instruction *instruction = (Instruction *)&instr;

    if (instruction->opcode == LD || instruction->opcode == ST) {
        // Check for data hazard between LD/ST and the instruction in RR stage
        if (pipeline->RR.state == 1 && pipeline->RR.instruction.dest == instruction->left_operand) {
            return 1;
        }
    }

    return 0;
}

int execute_IF_stage(CPU* cpu, Pipeline* pipeline, Branch_Predictor* bp) {
    int stall = 0;

    if (pipeline->IF.state == 0) {
        int next_instr = cpu->memory[cpu->PC / 4];

        if (is_control_instruction(next_instr)) {
            if (check_and_update_branch_predictor(bp, cpu->PC, next_instr)) {
                cpu->PC = get_target_address(cpu->PC, next_instr);
            } else {
                cpu->PC += 4;
            }
        } else if (!is_data_hazard(cpu, pipeline, next_instr)) {
            memcpy(&(pipeline->IF.instruction), &next_instr, sizeof(Instruction));
            pipeline->IF.state = 1;
            cpu->PC += 4;
        } else {
            stall = 1;
        }
    } else {
        stall = 1;
    }

    pipeline->ID = pipeline->IF;
    return stall;
}



int is_pipeline_empty(Pipeline* pipeline) {
    return pipeline->IF.state == 0 &&
           pipeline->ID.state == 0 &&
           pipeline->IA.state == 0 &&
           pipeline->RR.state == 0 &&
           pipeline->Add.state == 0 &&
           pipeline->Mul.state == 0 &&
           pipeline->Div.state == 0 &&
           pipeline->BR.state == 0 &&
           pipeline->Mem1.state == 0 &&
           pipeline->Mem2.state == 0 &&
           pipeline->WB.state == 0;
}

void clear_pipeline_stages(Pipeline* pipeline) {
    pipeline->IF.state = 0;
    pipeline->ID.state = 0;
    pipeline->IA.state = 0;
    pipeline->RR.state = 0;
    pipeline->Add.state = 0;
    pipeline->Mul.state = 0;
    pipeline->Div.state = 0;
    pipeline->BR.state = 0;
    pipeline->Mem1.state = 0;
    pipeline->Mem2.state = 0;
    pipeline->WB.state = 0;
}


Instruction parse_instruction(const char *line)
{
    Instruction instr;
    char opcode_str[10];
    int op1, op2, op3;

    //sscanf(line, "%s %*s%d %*s%d %*s%d", opcode_str, &op1, &op2, &op3);
    if (sscanf(line, "%s %*s%d %*s%d %*s%d", opcode_str, &op1, &op2, &op3) != 4) {
        if (sscanf(line, "%s %*s%d %*s%d", opcode_str, &op1, &op2) != 3) {
            sscanf(line, "%s %*s%d", opcode_str, &op1);
            op2 = 0;
        }
        op3 = 0;
    }

    if (strcmp(opcode_str, "set") == 0)
    {
        instr.opcode = SET;
    }
    else if (strcmp(opcode_str, "add") == 0)
    {
        instr.opcode = ADD;
    }
    else if (strcmp(opcode_str, "sub") == 0)
    {
        instr.opcode = SUB;
    }
    else if (strcmp(opcode_str, "mul") == 0)
    {
        instr.opcode = MUL;
    }
    else if (strcmp(opcode_str, "div") == 0)
    {
        instr.opcode = DIV;
    }
    else if (strcmp(opcode_str, "ld") == 0)
    {
        instr.opcode = LD;
    }
    else if (strcmp(opcode_str, "st") == 0)
    {
        instr.opcode = ST;
    }
    else if (strcmp(opcode_str, "bez") == 0)
    {
        instr.opcode = BEZ;
    }
    else if (strcmp(opcode_str, "bgez") == 0)
    {
        instr.opcode = BGEZ;
    }
    else if (strcmp(opcode_str, "blez") == 0)
    {
        instr.opcode = BLEZ;
    }
    else if (strcmp(opcode_str, "bgtz") == 0)
    {
        instr.opcode = BGTZ;
    }
    else if (strcmp(opcode_str, "bltz") == 0)
    {
        instr.opcode = BLTZ;
    }
    else if (strcmp(opcode_str, "ret") == 0)
    {
        instr.opcode = RET;
    }
    else
    {
        fprintf(stderr, "Invalid opcode: %s\n", opcode_str);
        exit(1);
    }

    
    instr.dest = op1;
    instr.left_operand = op2;
    instr.right_operand = op3;

    return instr;
}

void load_instruction_file(CPU *cpu, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {

        fprintf(stderr, "Error opening file: %s\n", filename);
        exit(1);
    }

    char line[256];
    int address;
    int instruction_count = 0; // Add this line to count the number of instructions

    while (fgets(line, sizeof(line), file))
    {
        Instruction instr;
        sscanf(line, "%x %s", &address, line);
        instr = parse_instruction(line);
        cpu->memory[address / 4] = *((int *)&instr);
        instruction_count++; // Increment the instruction count
        cpu->total_instructions++;
    }

    fclose(file);

    cpu->total_instructions = instruction_count; // Set the total_instructions in the CPU struct
}


void load_memory_map_file(CPU *cpu, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", filename);
        exit(1);
    }

    int value;
    int index = 0;

    while (fscanf(file, "%d", &value) != EOF)
    {
        cpu->memory[index++] = value;
    }

    fclose(file);
}

void run_cpu_fun(){

    CPU *cpu = CPU_init();
    CPU_run(cpu);
    CPU_stop(cpu);
}

int main(int argc, const char * argv[]) {
    if (argc<=1) {
        fprintf(stderr, "Error : missing required args\n");
        return -1;
    }
    
    char* memory_map_filename = (char*)argv[1];
    char* instruction_filename = (char*)argv[2];

    CPU *cpu = CPU_init();

    load_memory_map_file(cpu, memory_map_filename);
    load_instruction_file(cpu, instruction_filename);
    
    run_cpu_fun();
    
    return 0;
}