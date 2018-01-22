#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the ``readline'' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
 	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_si(char *args) {
	cpu_exec(1);
	return 0;
}

static int cmd_x(char *args) {
	//ssx : scan the memory for print
	char *addr_str = NULL;
	char *len_str = NULL;
	int len = 1;
	int i = 0;
	swaddr_t read_addr = 0;
	len_str = strtok(args, " ");
	addr_str = strtok(NULL, " ");
	sscanf(len_str, "%d", &len);
	sscanf(addr_str, "%x", &read_addr);
	//TO DO: exception solve
	
	while(i < len) {
		int value = swaddr_read(read_addr,1);
		if(i % 8 == 0 ) {
			if(i != 0)
				printf("\n");
			printf("0x%08x: ", read_addr);
		}
		printf("%02X ", value);
		if(isgraph(value))
			printf("%c", value);
		else
			printf(" ");
		printf(" - ");
		i++;
		read_addr++;
	}
	printf("\n");
	return 0;
}

static int cmd_info(char *args) {
	if(args == NULL) {
	
	}else if(strcmp(args,"r") == 0) {
		printf("EAX: 0x%08X\tECX: 0x%08X\tEDX: 0x%08X\tEBX: 0x%08X\n",
			   cpu.eax,cpu.ecx,cpu.edx,cpu.ebx);
		printf("ESP: 0x%08X\tEBP: 0x%08X\tESI: 0x%08X\tEDI: 0x%08X\n",
			   cpu.esp,cpu.ebp,cpu.esi,cpu.edi);
		printf("AX:      0x%04X\tCX:      0x%04X\tDX:      0x%04X\tBX:      0x%04X\n",
			   reg_w(R_AX),reg_w(R_CX),reg_w(R_DX),reg_w(R_BX));
		printf("SP:      0x%04X\tDP:      0x%04X\tSI:      0x%04X\tDI:      0x%04X\n",
		       reg_w(R_SP),reg_w(R_BP),reg_w(R_SI),reg_w(R_DI));
		printf("AH:        0x%02X\tCH:        0x%02X\tDH:        0x%02X\tBH:        0x%02X\n",
			   reg_b(R_AH),reg_b(R_CH),reg_b(R_DH),reg_b(R_BH));
		printf("AL:        0x%02X\tCL:        0x%02X\tDL:        0x%02X\tBL:        0x%02X\n",
			   reg_b(R_AL),reg_b(R_CL),reg_b(R_DL),reg_b(R_BL));

	}
	return 0;
}

static int cmd_p(char *args) {
	bool success = false;
	if(!args) {
		goto EXCEPTION_NULL_ARGS;
	}

	expr(args, &success);
	
	return 1;
	
EXCEPTION_NULL_ARGS:
	printf("No expr to evaluate,please add expr as argument after command 'p'\n");
	return 0;
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Sigle-step run", cmd_si },
	{ "info","Print cpu's information", cmd_info },
	{ "x","Scan memory and print them", cmd_x },
	{ "p","Evaluate the following expression",cmd_p}
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

 	if(arg == NULL) {
		/* no argument given */
 		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
