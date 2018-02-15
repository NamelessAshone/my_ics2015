#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, LEQ, LSS, GEQ, GTR, NEQ , AND, OR, NUM, HEX, DEREF, MINUS, REG

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{ " +",	NOTYPE},						// spaces
	{ "\\+", '+'},							// plus
	{ "==", EQ},							// equal
	{ "<=", LEQ},							// less or equal
	{ "<", LSS},							// less
	{ ">=", GEQ},							// greater or equal
	{ ">", GTR},							// greater
	{ "!=", NEQ},							// not equal
	{ "&&", AND},							// and
	{ "\\|\\|", OR},						// or
	{ "\\*", '*'},							// multiplay
	{ "^0x[0-9a-fA-F]+", HEX},				// hex
	{ "[0-9]+", NUM},						// num
	{ "-", '-'},							// subtract
	{ "/", '/'},							// divide
	{ "\\(", '('},							// left bracket
	{ "\\)", ')'},							// right bracket
	{ "^\\$((e)?([abcd]x|[bsi]p|[ds]i)|[abcd]l|[abcd]h)", REG},
											// register

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

//SSX : NR_REGEX is used to compute the length of a array.

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */
				
				switch(rules[i].token_type) {
					case NOTYPE:continue;
					case NUM:tokens[nr_token].type = NUM;
							 break;
					case EQ	:tokens[nr_token].type = EQ;
							 break;
					case NEQ:tokens[nr_token].type = NEQ;
							 break;
					case LSS:tokens[nr_token].type = LSS;
							 break;
					case LEQ:tokens[nr_token].type = LEQ;
							 break;
					case GTR:tokens[nr_token].type = GTR;
							 break;
					case GEQ:tokens[nr_token].type = GEQ;
							 break;
					case '+':tokens[nr_token].type = '+';
							 break;
					case '-':tokens[nr_token].type = '-';
							 break;
					case '*':tokens[nr_token].type = '*';
							 break;
					case '/':tokens[nr_token].type = '/';
							 break;
					case '(':tokens[nr_token].type = '(';
							 break;
					case ')':tokens[nr_token].type = ')';
							 break;
					case HEX:tokens[nr_token].type = HEX;
							 break;
					case REG:tokens[nr_token].type = REG;
							 break;
					case AND:tokens[nr_token].type = AND;
							 break;
					case OR :tokens[nr_token].type = OR;
							 break;
					default :panic("please implement me");
				}
				memcpy(tokens[nr_token].str, substr_start, substr_len);
				nr_token++;
				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}
	return true; 
}

static bool check_parentheses(int p, int q) {
	char buf[32];
	int buf_top = 0;
	int i;

	memset(buf, 0, 32 * sizeof(char));
	if(strcmp(tokens[p].str, "(") != 0 || strcmp(tokens[q].str, ")") != 0)
		return false;
	for(i = p + 1; i <= q - 1; i++) {
		if(strcmp(tokens[i].str, "(") == 0) {
			buf[buf_top++] = '(';
		} else if(strcmp(tokens[i].str, ")") == 0) {
			if(buf_top > 0 )
			   buf[--buf_top] = 0;
			else
				return false;	
		}
	}
	if(buf_top > 0)
		return false;
	return true;
}

static int get_op_level(char ch) {
	struct {
		int type;
		int level;	
	} op_level[] = {
		{ AND, 0},
		{ OR , 1},
		{ GTR, 2},
		{ GEQ, 2},
		{ LSS, 2},
		{ LEQ, 2},
		{ EQ , 2},
		{ NEQ, 2},
		{ '+', 3},
		{ '-', 3},
		{ '*', 4},
		{ '/', 4}
	};
	int NR_OP = sizeof(op_level) / sizeof(op_level[0]);
	int i;

	for(i = 0; i < NR_OP; i++)
		if(op_level[i].type == ch)
			return op_level[i].level;
	printf("Can't find this op's <%c> level.\n", ch);
	return -1;
}

static int find_dominant_operator(int p, int q) {
	int op = -1;
	int i, u;

	for(i = q; i >= p; i--) {
		switch(tokens[i].type) {
			case AND:
			case OR :
			case LSS:
			case LEQ:
			case GTR:
			case GEQ:
			case EQ :
			case NEQ:
			case '+': 
			case '-':
		   	case '*': 
			case '/': if(op == -1)
						 op = i;
					  else
						if(get_op_level(tokens[i].type) < get_op_level(tokens[op].type))
							op = i;
					  break; 
			case ')': u = i - 1;
					  while(!check_parentheses(u, i) && u >= p) 
						  u--;
					  printf("In func 'find_dominant_operator'-->> u: %d, <%d, %d>\n", u, p, q);
					  if(u == p - 1)
						  printf("\33[30;46mParentheses match failed\n\33[0m");
					  i = u;
					  break;
			default : continue;		 
		}
	}

	printf("op : %d<%c>\n", op, tokens[op].type);
	return op;
}

static uint32_t eval(int p, int q) {
	printf("\33[30;41mIn func eval -->> p = %d , q = %d\33[0m\n", p, q);
	if(p > q) {
		printf("fork 1\n");
		/* Bad expression. */
		panic("Bad expression fault.\n");
		return 0;
	} else if(p == q) {
		printf("fork 2\n");
		/* Single token.
		 * For now this token should be number.
		 * Return the value of the number.
		 * */
		enum { R_EIP = 256 };
	    struct {
			char *str;
			int size;
		    int type;	
		} reg[] = {
			{ "$eax", 32, R_EAX},
			{ "$ecx", 32, R_ECX},
			{ "$edx", 32, R_EDX},
			{ "$ebx", 32, R_EBX},
			{ "$esp", 32, R_ESP},
			{ "$ebp", 32, R_EBP},
			{ "$esi", 32, R_ESI},
			{ "$edi", 32, R_EDI},
			{ "$ax", 16, R_AX},
			{ "$cx", 16, R_CX},
			{ "$dx", 16, R_DX},
			{ "$bx", 16, R_BX},
			{ "$sp", 16, R_SP},
			{ "$bp", 16, R_BP},
			{ "$si", 16, R_SI},
			{ "$di", 16, R_DI},
			{ "$al", 8, R_AL},
			{ "$cl", 8, R_CL},
			{ "$dl", 8, R_DL},
			{ "$bl", 8, R_BL},
			{ "$ah", 8, R_AH},
			{ "$ch", 8, R_CH},
			{ "$dh", 8, R_DH},
			{ "$bh", 8, R_BH},
			{ "$eip", 32, R_EIP}
		};
		uint32_t val = 0;
  		int nr_reg = sizeof(reg) / sizeof(reg[0]);	
		int i;


		switch(tokens[p].type) {
			case NUM: sscanf(tokens[p].str, "%d", &val);
					  break;
			case REG: for(i = 0; i < nr_reg; i++)
						  if(strcmp(tokens[p].str, reg[i].str) == 0) {
							  if(reg[i].type == R_EIP) {
								  val = cpu.eip;
								  break;
							  }
						      switch(reg[i].size) {
								  case 8 : val = cpu.gpr[reg[i].type%4]._8[reg[i].type >= 4];
										   break;
								  case 16: val = cpu.gpr[reg[i].type]._16;
										   break;
								  case 32: val = cpu.gpr[reg[i].type]._32;
										   break;
							  }
							  break;
						  }
					  break;
			case HEX: sscanf(tokens[p].str, "%x", &val);
					  break;
			default : printf("This token can't be evaluated\n");			  
		}
		return val;
	} else if(check_parentheses(p, q) == true) {
		printf("fork 3\n");
		/* The expression is surrounded by a matched pair of parentheses.
		 * If that is the case, just throw away the parentheses. 
		 * */
		return eval(p + 1, q - 1);
	} else {
		printf("fork 4\n");
		/* We should do more things here. */
		int op = find_dominant_operator(p, q);
		int	val1 = 0;
		int val2 = 0;
		if(tokens[op].type == DEREF) {
			val1 = eval(op + 1, q);
			printf("\33[30;102maddress: 0x%x  val: 0x%x  dominant op : DEREF\33[0m\n", val1, swaddr_read(val1, 1));	
			return swaddr_read(val1, 1);
		} else if (tokens[op].type == MINUS) {
			val1 = eval(op + 1, q);
			printf("\33[30;102mbefore: %x after: %x dominant op : MINUS'\33[0m\n", val1, -val1);			return -val1;	
		} else {
		    val1 = eval(p, op - 1);
			val2 = eval(op + 1, q);
			printf("\33[30;102mval1: %d val2 :%d dominant op : %c\33[0m\n", val1, val2, tokens[op].type);	
			switch(tokens[op].type) {
				case '+'  : return val1 +  val2;
				case '-'  : return val1 -  val2;
				case '*'  : return val1 *  val2;
				case '/'  : return val1 /  val2;
				case LSS  : return val1 <  val2;
				case LEQ  : return val1 <= val2;
				case GTR  : return val1 >  val2;
				case GEQ  : return val1 >= val2;
				case EQ   : return val1 == val2;
				case NEQ  : return val1 != val2;
				case AND  : return val1 && val2;
				case OR   : return val2 || val2;
				default : panic("Wrong operator type.\n"); 		
			} 		
		}
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

// #define TEST_PRINT_TOKENS
#ifdef TEST_PRINT_TOKENS
	int i = 0, j = 0;
	for(; i < nr_token; i++) {
		printf("'%s'\t",tokens[i].str);
		if(i % 10 == 9 || i == nr_token - 1) {
			printf("\n");
			while(j != i + 1) 
				printf("%d\t", j++);
			printf("\n");
		}
	}
	printf("\n");
#endif

#ifdef TEST_CHECK_PARETHESE
	printf("[[%d]]\n",nr_token);
	if(check_parentheses(0, nr_token))
		printf("\33[30;43mmatched\33[0m\n");
	else
		printf("\33[30;43mno matched\33[0m\n"); 
#endif

#ifdef 	TEST_DOMINANT_OPERATOR
	printf("\33[30;102mdominant op : \33[0m");
	printf("\33[30;102m%c\33[0m\n",tokens[find_dominant_operator(0, nr_token - 1)].type);
#endif
	/* TODO: Insert codes to evaluate the expression. */
	int i = 0;
	for(i = 0; i < nr_token; i++) {
		if(tokens[i].type == '*' && (i == 0 || tokens[i - 1].type == NUM || tokens[i - 1].type == HEX || tokens[i - 1].type == REG)) {
			tokens[i].type = DEREF;
		}
	}
    int r = eval(0,nr_token-1);
	printf("\33[30;102m%d\n0x%x\n\33[0m", r, r);

	//panic("please implement me");
	return 0;
}

