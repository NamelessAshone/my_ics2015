#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NUM, RT

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
	{ "\\*", '*'},							// multiplay
	/* { "-?([1-9][0-9]*|0+)", NUM},		// num
	 */
	{ "[1-9][0-9]*|0+", NUM},				// num
	{ "-", '-'},							// subtract
	{ "/", '/'},							// divide
	{ "\\(", '('},							// left bracket
	{ "\\)", ')'},							// right bracket
	{ "\\n+", RT}							// return
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
					case NOTYPE:break;
					case NUM:tokens[nr_token].type = NUM;
							 memcpy(tokens[nr_token].str, substr_start, substr_len);
							 nr_token++;
							 break;
					case EQ	:tokens[nr_token].type = EQ;
							 strcpy(tokens[nr_token].str, "==");
							 nr_token++;
							 break;
					case '+':tokens[nr_token].type = '+';
							 strcpy(tokens[nr_token].str, "+");
							 nr_token++;
							 break;
					case '-':tokens[nr_token].type = '-';
							 strcpy(tokens[nr_token].str, "-");
							 nr_token++;
							 break;
					case '*':tokens[nr_token].type = '*';
							 strcpy(tokens[nr_token].str, "*");
							 nr_token++;
							 break;
					case '/':tokens[nr_token].type = '/';
							 strcpy(tokens[nr_token].str, "/");
							 nr_token++;
							 break;
					case '(':tokens[nr_token].type = '(';
							 strcpy(tokens[nr_token].str, "(");
							 nr_token++;
							 break;
					case ')':tokens[nr_token].type = ')';
							 strcpy(tokens[nr_token].str, ")");
							 nr_token++;
							 break;
					//case RT :break; 
					default :panic("please implement me");
				}

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
	return true;
}

struct {
		char type;
		int level;	
} op_level[] = {
		{'+', 1},
		{'-', 1},
		{'*', 2},
		{'/', 2}
};

#define NR_OP (sizeof(op_level) / sizeof(op_level[0]))
static int get_op_level(char ch) {
	int i;
	for(i = 0; i < NR_OP; i++)
		if(op_level[i].type == ch)
			return op_level[i].level;
	printf("Can't find this op's <%c> level.\n", ch);
	return -1;
}

static int find_dominant_operator(int p, int q) {
	int op = -1;
	int i,u;

	for(i = p; i <= q; i++) {
		switch(tokens[i].type) {
			case '+': 
			case '-':
		   	case '*': 
			case '/': if(op == -1)
						 op = i;
					  else
						if(get_op_level(tokens[i].type) < get_op_level(tokens[op].type))
							op = i;
					  break; 
			case '(': u = i + 1;
					  while(!check_parentheses(i, u) && u <= q) 
						  u++;
					  if(u == q + 1)
						  printf("\33[30;46mParentheses match failed\n\33[0m");
					  i = u;
					  break;
			default : continue;		 
		}
	}

	printf("%d<%c>", op, tokens[op].type);
	return op;
}

static uint32_t eval(int p, int q) {
	printf("\33[30;41mp = %d , q = %d\33[0m\n", p, q);
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
		int val = 0;
		switch(tokens[p].type) {
			case NUM: sscanf(tokens[p].str, "%d", &val);
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
		int	val1 = eval(p, op - 1);
		int val2 = eval(op + 1, q);
		printf("\33[30;102mval1: %d val2 :%d dominant op : %c\33[0m\n", val1, val2, tokens[op].type);	
		switch(tokens[op].type) {
			case '+': return  val1 + val2;
			case '-': return  val1 - val2;
			case '*': return  val1 * val2;
			case '/': return  val1 / val2;
			default : panic("Wrong operator type.\n"); 
		} 
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

#define TEST_PRINT_TOKENS
#ifdef TEST_PRINT_TOKENS
	int i = 0;
	for(; i < nr_token; i++) {
		printf("'%s'\t",tokens[i].str);
		if(i % 10 == 9) {
			printf("\n");
			int j = i - 9;
			while(j % 10 != 0 && j > 0) 
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
	printf("\33[30;102m%d\n\33[0m", eval(0,nr_token-1));

	//panic("please implement me");
	return 0;
}

