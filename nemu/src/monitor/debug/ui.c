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
static int cmd_info(char *args) {
	if(args == NULL) {
	
	}else if(strcmp(args,"r") == 0) {
		printf("EAX: %8X\tECX: %8X\tEDX: %8X\tEBX: %8X\n",
			   cpu.eax,cpu.ecx,cpu.edx,cpu.ebx);
		printf("ESP: %8X\tEBP: %8X\tESI: %8X\tEDI: %8X\n",
			   cpu.esp,cpu.ebp,cpu.esi,cpu.edi);
		printf("AX:  %8X\tCX:  %8X\tDX:  %8X\tBX:  %8X\n",
			   reg_w(R_AX),reg_w(R_CX),reg_w(R_DX),reg_w(R_BX));
		printf("SP:  %8X\tDP:  %8X\tSI:  %8X\tDI:  %8X\n",
		       reg_w(R_SP),reg_w(R_BP),reg_w(R_SI),reg_w(R_DI));
		printf("AH:  %8X\tCH:  %8X\tDH:  %8X\tBH:  %8X\n",
			   reg_b(R_AH),reg_b(R_CH),reg_b(R_DH),reg_b(R_BH));
		printf("AL:  %8X\tCL:  %8X\tDL:  %8X\tBL:  %8X\n",
			   reg_b(R_AL),reg_b(R_CL),reg_b(R_DL),reg_b(R_BL));

	}
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
	{ "info","Print cpu's information", cmd_info }

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
