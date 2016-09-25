#pragma once

#include <stack>
#include <queue>
#include <vector>

class ExpressionParser {

	public:
		ExpressionParser(const std::string &expr);
		~ExpressionParser();
		double eval();

	private:
		struct token {
			char c;
			double d;
			bool isNum;
			bool isOp;
			token() : c(0), isNum(false), isOp(true) {};
		};
		const std::string m_expr;
		std::vector<token> tokens;
		std::stack<token> m_opstack;
		std::queue<token> m_revpol;
		std::stack<double> m_output;
		void tokenizeExpr();
		void shuntingYard();
		void evalRevPol();

};
