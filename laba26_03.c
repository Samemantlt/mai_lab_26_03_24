#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef struct sConstantExpression ConstantExpression;
typedef struct sTwoOperandExpression TwoOperandExpression;
typedef struct sVariableExpression VariableExpression;
typedef struct sExpressionNodeRef ExpressionNodeRef;

typedef enum
{
	Constant,
	TwoOperand,
	Variable
} ExpressionNodeType;

typedef enum
{
	Add,
	Sub,
	Mul,
	Div
} Operand;

struct sExpressionNodeRef
{
	ExpressionNodeType type;
	union
	{
		ConstantExpression *constantExpression;
		TwoOperandExpression *twoOperandExpression;
		VariableExpression *variableExpression;
	};
};

struct sConstantExpression
{
	double value;
};

struct sTwoOperandExpression
{
	Operand operand;
	ExpressionNodeRef left;
	ExpressionNodeRef right;
};

struct sVariableExpression
{
	char name;
};

ExpressionNodeRef readConstant(char *str, int *pos)
{
	double number;
	int read;
	sscanf(&str[*pos], "%lf%n", &number, &read);

	(*pos) += read;
	ExpressionNodeRef result;
	result.type = Constant;
	result.constantExpression = malloc(sizeof(ConstantExpression));
	result.constantExpression->value = number;
	return result;
}

ExpressionNodeRef readVariable(char *str, int *pos)
{
	char alpha = str[*pos];

	(*pos)++;
	ExpressionNodeRef result;
	result.type = Variable;
	result.variableExpression = malloc(sizeof(VariableExpression));
	result.variableExpression->name = alpha;
	return result;
}

bool isNull(ExpressionNodeRef ref)
{
	return ref.constantExpression == NULL;
}

const char OPERAND_SYMBOLS[4] = "*/-+";
bool isOperand(char symbol)
{
	for (int i = 0; i < sizeof(OPERAND_SYMBOLS) / sizeof(char); i++)
	{
		if (symbol == OPERAND_SYMBOLS[i])
			return true;
	}
	return false;
}

Operand parseOperand(char symbol)
{
	switch (symbol)
	{
	case '+':
		return Add;
	case '*':
		return Mul;
	case '/':
		return Div;
	case '-':
		return Sub;
	default:
		return -1;
	}
}

int getOperandPriority(Operand operand)
{
	switch (operand)
	{
	case Add:
	case Sub:
		return 1;
	case Mul:
	case Div:
		return 2;
	}
}

ExpressionNodeRef parse(char *str, int *i, int bracketPriority, int prevOperandPriority)
{
	char previous;
	char current;

	ExpressionNodeRef left = {0, 0};
	int length = strlen(str);
	for (; *i < length; (*i)++)
	{
		if (isspace(str[*i]))
			continue;

		if (isdigit(str[*i]) || (str[*i] == '-' && isNull(left)))
		{
			if (!isNull(left))
			{
				printf("Error isdigit '!isNull(left)': %d", left.type);
			}
			left = readConstant(str, i);
			(*i)--;
			continue;
		}

		if (isalpha(str[*i])) {
			if (!isNull(left))
			{
				printf("Error isalpha '!isNull(left)': %d", left.type);
			}
			left = readVariable(str, i);
			(*i)--;
			continue;
		}

		if (isOperand(str[*i]))
		{
			Operand operand = parseOperand(str[*i]);
			if (isNull(left))
			{
				printf("Error isOperand 'isNull(left)': %d", left.type);
			}

			int operandPriority = getOperandPriority(operand);
			if (operandPriority < prevOperandPriority)
			{
				(*i)--;
				return left;
			}

			(*i)++;

			ExpressionNodeRef right = parse(str, i, bracketPriority, operandPriority);

			TwoOperandExpression *opExpression = malloc(sizeof(TwoOperandExpression));
			opExpression->operand = operand;
			opExpression->left = left;
			opExpression->right = right;

			ExpressionNodeRef ref;
			ref.type = TwoOperand;
			ref.twoOperandExpression = opExpression;

			left = ref;
			continue;
		}

		if (str[*i] == '(')
		{
			(*i)++;
			return parse(str, i, bracketPriority + 1, -1);
		}

		if (str[*i] == ')')
		{
			(*i)++;
			return left;
		}
	}

	return left;
}

ExpressionNodeRef parseDefault(char *str)
{
	int i = 0;
	return parse(str, &i, 0, -1);
}

double evaluate(ExpressionNodeRef ref, double variableValue)
{
	switch (ref.type)
	{
	case Constant:
		return ref.constantExpression->value;
	case TwoOperand:
		double leftValue = evaluate(ref.twoOperandExpression->left, variableValue);
		double rightValue = evaluate(ref.twoOperandExpression->right, variableValue);
		switch (ref.twoOperandExpression->operand)
		{
		case Add:
			return leftValue + rightValue;
		case Mul:
			return leftValue * rightValue;
		case Div:
			return leftValue / rightValue;
		case Sub:
			return leftValue - rightValue;
		}
	case Variable:
		return variableValue;
	}
	printf("Nothing matched evaluating");
	return -1;
}

double evaluateText(char *exprText, double variableValue)
{
	ExpressionNodeRef expression = parseDefault(exprText);
	return evaluate(expression, variableValue);
}

void testExpr(char *expr, double expected)
{
	double actual = evaluateText(expr, 0);
	if (actual != expected)
	{
		fprintf(stderr, "Assert error: %s // %lf != %lf\n", expr, actual, expected);
	}
}

void testVarExpr(char *expr, double variableValue, double expected)
{
	double actual = evaluateText(expr, variableValue);
	if (actual != expected)
	{
		fprintf(stderr, "Assert error: %s // var %lf // %lf != %lf\n", expr, variableValue, actual, expected);
	}
}

void tests() {
	testExpr("15  +67", 15 + 67);
	testExpr("21*3", 21 * 3);
	testExpr("21*-3", 21 * -3);
	testExpr("21*(-3)", 21 * -3);
	testExpr("25 + 4 * 6", 25 + 4 * 6);
	testExpr("25 + 4 * -6", 25 + 4 * -6);
	testExpr("(25 + 4) * -6", (25 + 4) * -6);
	testExpr("25 + (4 * -6)", 25 + (4 * -6));
	testExpr("(55 + 33) / (11 * 8)", (55 + 33) / (11 * 8));
	testExpr("60 / (4 + 3 * 2)", 60 / (4 + 3 * 2));
	testExpr("60 / (3 + 3 * 2 + 1)", 60 / (3 + 3 * 2 + 1));
	testExpr("20 * 3 / (3 + 3 * 2 + 1)", 20 * 3 / (3 + 3 * 2 + 1));
	testExpr("2 * 10 * 3 / (3 + 3 * 2 + 1)", 2 * 10 * 3 / (3 + 3 * 2 + 1));

	testVarExpr("2 * a", 3, 2 * 3);
	testVarExpr("5*a", 3, 5 * 3);
}

void replaceVariable(ExpressionNodeRef* this, char variableName, ExpressionNodeRef to) {
	switch (this->type)
	{
	case Constant:
		break;

	case TwoOperand:
		TwoOperandExpression* expr = this->twoOperandExpression;
		replaceVariable(&expr->left, variableName, to);
		replaceVariable(&expr->right, variableName, to);
		break;

	case Variable:
		*this = to;
		break;
	
	default:
		break;
	}
}

char getOperand(Operand operand) {
	switch (operand)
	{
	case Add:
		return '+';
	case Sub:
		return '-';
	case Mul:
		return '*';
	case Div:
		return '/';
	}

	return (char)1;
}

void printNode(ExpressionNodeRef ref, int prevOperandPriority) {
	switch (ref.type)
	{
	case Constant:
		printf("%lg", ref.constantExpression->value);
		break;
	case TwoOperand:
		int curOperandPriority = getOperandPriority(ref.twoOperandExpression->operand);
		if (curOperandPriority < prevOperandPriority)
			printf("(");
		printNode(ref.twoOperandExpression->left, curOperandPriority);
		printf(" %c ", getOperand(ref.twoOperandExpression->operand));
		printNode(ref.twoOperandExpression->right, curOperandPriority);
		if (curOperandPriority < prevOperandPriority)
			printf(")");
		break;
	case Variable:
		printf("%c", ref.variableExpression->name);
		break;
	
	default:
		break;
	}
}

int main()
{
	tests();

	ExpressionNodeRef expr = parseDefault("(8 - 3) * 3 * a + 4");
	printNode(expr, -1);
	printf("\n\n");

	ExpressionNodeRef toRepl = parseDefault("4 * 8 - 2");
	replaceVariable(&expr, 'a', toRepl);

	double result = evaluate(expr, -100000);
	printNode(expr, -1);
	printf("\n\n\n%f\n", 5.0);

	return 0;
}