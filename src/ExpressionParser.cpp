#include <cmath>
#include <ctype.h>
#include <sstream>
#include "ExpressionParser.h"
#include "opmap.h"

static bool isop(char c);
static bool isparen(char c);

ExpressionParser::ExpressionParser(const std::string &expr) : m_expr(expr)
{
	/* expression only contains numbers and basic mathematical operators */
	for (char c : expr) {
		if (!isdigit(c) && !isop(c) && !isparen(c) && c != '.')
			throw std::runtime_error("expression can only contain "
					"numbers, parentheses and operators");
	}
}

ExpressionParser::~ExpressionParser() {}

/* split the expression into its various tokens */
void ExpressionParser::tokenizeExpr()
{
	std::stringstream parser(m_expr);
	token last;

	while (parser) {
		token t;
		if (isalnum(parser.peek())) {
			parser >> t.d;
		} else {
			char c;
			parser >> c;
			/* unary plus and minus */
			if ((c == '+' || c == '-') && (last.isOp || last.c == '('))
				t.c = c == '+' ? 'p' : 'm';
			else
				t.c = c;
		}
		t.isNum = t.c == 0;
		t.isOp = t.c != 0 && t.c != '(' && t.c != ')';
		tokens.push_back(t);
		last = t;
	}

	tokens.pop_back();
}

double ExpressionParser::eval()
{
	shuntingYard();
	evalRevPol();
	return m_output.top();
}

void ExpressionParser::shuntingYard()
{
	for (auto &t : tokens) {
		if (t.isOp) {
			if (!m_opstack.empty()) {
				token top = m_opstack.top();
				/* compare precendece of the two operators */
				while (top.isOp && ((is_associative(t.c, LEFT_ASSOC)
						&& compare_precedence(t.c, top.c) <= 0)
						|| (is_associative(t.c, RIGHT_ASSOC)
						&& compare_precedence(t.c, top.c) < 0))) {

					/* pop from stack into queue */
					m_opstack.pop();
					m_revpol.push(top);

					if (!m_opstack.empty())
						top = m_opstack.top();
					else
						break;
				}
			}
			m_opstack.push(t);
		} else if (t.c == '(') {
			m_opstack.push(t);
		} else if (t.c == ')') {
			if (m_opstack.empty())
				throw std::runtime_error("mismatched parentheses");

			token top = m_opstack.top();
			/* pop until matching paren */
			while (top.c != '(') {
				m_opstack.pop();
				m_revpol.push(top);
				if (m_opstack.empty())
					throw std::runtime_error("mismatched parentheses");
				top = m_opstack.top();
			}

			if (top.c != '(')
				throw std::runtime_error("mismatched parentheses");

			/* pop the left paren from the stack */
			m_opstack.pop();

		} else {
			/* t is a number */
			m_revpol.push(t);
		}
	}

	/* pop all remaining operators from the stack into the queue */
	while (!m_opstack.empty()) {
		token top = m_opstack.top();
		if (top.c == '(' || top.c == ')')
			throw std::runtime_error("mismatched parentheses");
		m_opstack.pop();
		m_revpol.push(top);
	}
}

void ExpressionParser::evalRevPol()
{
	while (!m_revpol.empty()) {
		token next = m_revpol.front();
		if (next.isNum) {
			m_output.push(next.d);
		} else {
			/* token is operator */
			if (next.c == 'm' || next.c == 'p') {
				if (m_output.empty())
					throw std::runtime_error("invalid "
						"mathematical expression");

				double d = m_output.top();
				m_output.pop();
				m_output.push(next.c == 'm' ? -d : d);
			} else {
				/* all other operators are binary */
				if (m_output.size() < 2)
					throw std::runtime_error("invalid "
						"mathematical expression");

				double d1, d2, result;
				d2 = m_output.top();
				m_output.pop();
				d1 = m_output.top();
				m_output.pop();

				result = next.c == '+' ? d1 + d2 :
					next.c == '-' ? d1 - d2 :
					next.c == '*' ? d1 * d2 :
					next.c == '/' ? d1 / d2 :
					pow(d1, d2);

				m_output.push(result);
			}
		}
		m_revpol.pop();
	}

	if (m_output.size() != 1)
		throw std::runtime_error("Invalid mathematical expression.");
}

static bool isop(char c)
{
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
}

static bool isparen(char c)
{
	return c == '(' || c == ')';
}
