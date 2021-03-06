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
	//ssx : scan the memory to print
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

static int cmd_d(char *args) {
	int del_no = -1;
	WP *ptr = NULL;

   	sscanf(args, "%d", &del_no);
	if(del_no == EOF || del_no >= get_NR_WP() || del_no < 0) {
		printf("The wp no must in <0, %d>\n", get_NR_WP());
		return 0;
	}
	ptr = get_wp_ptr(del_no);
	if(ptr != NULL) {
		free_wp(ptr);
	}
	return 0;
}

static int cmd_info(char *args) {
	if(args == NULL) {
	
	} else if(strcmp(args, "r") == 0) {
		printf("EAX: %08X\tECX: %08X\tEDX: %08X\tEBX: %08X\n",
			   cpu.eax, cpu.ecx, cpu.edx, cpu.ebx);
		printf(" AX: %8.4X\t CX: %8.4X\t DX: %8.4X\t BX: %8.4X\n",
			   reg_w(R_AX), reg_w(R_CX), reg_w(R_DX), reg_w(R_BX));
		printf(" AH: %8.2X\t CH: %8.2X\t DH: %8.2X\t BH: %8.2X\n",
			   reg_b(R_AH), reg_b(R_CH), reg_b(R_DH), reg_b(R_BH));
		printf(" AL: %8.2X\t CL: %8.2X\t DL: %8.2X\t BL: %8.2X\n",
			   reg_b(R_AL), reg_b(R_CL), reg_b(R_DL), reg_b(R_BL));
		printf("ESP: %08X\tEBP: %08X\tESI: %08X\tEDI: %08X\n",
			   cpu.esp, cpu.ebp, cpu.esi, cpu.edi);
		printf(" SP: %8.4X\t DP: %8.4X\t SI: %8.4X\t DI: %8.4X\n",
		       reg_w(R_SP), reg_w(R_BP), reg_w(R_SI), reg_w(R_DI));
		printf("EIP: %08X\n", cpu.eip);
	} else if(strcmp(args, "w") == 0) {
		print_wp();
	}
	return 0;
}

static int cmd_p(char *args) {
	bool success = false;
	if(!args) 
		goto EXCEPTION_NULL_ARGS;
	int r = expr(args, &success);
	if(!success)
		goto EXCEPTION_EVAL_FAILED;
	printf("val = %d (0x%08x)\n", r, r);
	return 1;
	
EXCEPTION_NULL_ARGS:
	printf("No expr to evaluate\n");
	return 0;
EXCEPTION_EVAL_FAILED:
	printf("Evaluate the expr \"%s\" failed\n", args);
	return 0;
}

static int cmd_w(char *args) {
	uint32_t new_val;
	bool success = true;
	new_val = expr(args, &success);
	if(success == false) {
		printf("Wrong expr\n");
		return 0;
	}
	WP *n_wp = new_wp();	
	n_wp->val = new_val;
	n_wp->enb = true;
	strcpy(n_wp->what, args);
    printf("expr \"%s\" = %d (0x%08x)\n", n_wp->what, new_val, new_val);
	return 0;
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help},
	{ "c"	, "Continue the execution of the program", cmd_c},
	{ "q"	, "Exit NEMU", cmd_q},
	{ "si"	, "Sigle-step run", cmd_si},
	{ "info", "Print cpu's information", cmd_info},
	{ "x"	, "Scan memory and print them", cmd_x},
	{ "p"	, "Evaluate the following expression", cmd_p},
	{ "d"	, "Delete a watchpoint", cmd_d},
	{ "w"	, "Set a watchpoint", cmd_w}
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
