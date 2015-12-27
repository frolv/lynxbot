#include "..\stdafx.h"
#include "OpMap.h"

ExpressionParser::ExpressionParser(const std::string &expr) : m_expr(expr) {

	// expression can only contain numbers and basic mathematical operators
	std::regex exprRegex("^[\\d\\.()+\\-*/^]+$");
	std::smatch match;
	if (!std::regex_search(m_expr.begin(), m_expr.end(), match, exprRegex)) {
		throw std::runtime_error("Expression can only contain numbers, parentheses and operators.");
	}

}

ExpressionParser::~ExpressionParser() {}

/* Split the expression into its various tokens. */
void ExpressionParser::tokenizeExpr() {

	std::stringstream parser(m_expr);
	// last read token
	token last;

	while (parser) {

		token t;
		if (isalnum(parser.peek())) {
			parser >> t.d;
		}
		else {
			char c;
			parser >> c;
			// unary plus and minus
			if ((c == '+' || c == '-') && (last.isOp || last.c == '(')) {
				t.c = c == '+' ? 'p' : 'm';
			}
			else {
				t.c = c;
			}
		}
		t.isNum = t.c == 0;
		t.isOp = t.c != 0 && t.c != '(' && t.c != ')';
		tokens.push_back(t);
		last = t;
	
	}

	tokens.pop_back();

}

double ExpressionParser::eval() {
	shuntingYard();
	evalRevPol();
	return m_output.top();
}

void ExpressionParser::shuntingYard() {

	for (auto &t : tokens) {

		if (t.isOp) {

			if (!m_opstack.empty()) {

				token top = m_opstack.top();
				// compare precendece of the two operators
				while (top.isOp && ((isAssociative(t.c, LEFT_ASSOC) && comparePrecedence(t.c, top.c) <= 0)
				                 || (isAssociative(t.c, RIGHT_ASSOC) && comparePrecedence(t.c, top.c) < 0))) {
					
					// pop from stack into queue
					m_opstack.pop();
					m_revpol.push(top);

					if (!m_opstack.empty()) {
						// move on to the next operator in the stack
						top = m_opstack.top();
					}
					else {
						break;
					}

				}

			}
			
			m_opstack.push(t);
		
		}
		else if (t.c == '(') {
			m_opstack.push(t);
		}
		else if (t.c == ')') {

			if (m_opstack.empty()) {
				throw std::runtime_error("Mismatched parentheses.");
			}
			
			token top = m_opstack.top();
			// pop all tokens until a left bracket from the stack and add to the queue
			while (top.c != '(') {

				m_opstack.pop();
				m_revpol.push(top);
				if (m_opstack.empty()) {
					throw std::runtime_error("Mismatched parentheses.");
				}
				top = m_opstack.top();

			}

			if (top.c != '(') {
				throw std::runtime_error("Mismatched parentheses.");
			}
			// pop the left bracket from the stack
			m_opstack.pop();

		}
		else {
			// t is a number
			m_revpol.push(t);
		}
	}

	// after all tokens have been read, pop all remaining operators from the stack to the queue
	while (!m_opstack.empty()) {
		token top = m_opstack.top();
		if (top.c == '(' || top.c == ')') {
			throw std::runtime_error("Mismatched parentheses.");
		}
		m_opstack.pop();
		m_revpol.push(top);
	}

}

void ExpressionParser::evalRevPol() {

	while (!m_revpol.empty()) {

		token next = m_revpol.front();
		if (next.isNum) {
			m_output.push(next.d);
		}
		else {
			// token is operator
			if (next.c == 'm' || next.c == 'p') {
				if (m_output.empty()) {
					throw std::runtime_error("Invalid mathematical expression.");
				}

				double d = m_output.top();
				m_output.pop();
				m_output.push(next.c == 'm' ? -d : d);

			}
			else {
				// all other operators are binary
				if (m_output.size() < 2) {
					throw std::runtime_error("Invalid mathematical expression.");
				}

				double d2 = m_output.top();
				m_output.pop();
				double d1 = m_output.top();
				m_output.pop();
				double result;

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

	if (m_output.size() != 1) {
		throw std::runtime_error("Invalid mathematical expression.");
	}

}