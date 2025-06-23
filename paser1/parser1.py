#!/usr/bin/env python3
import sys

class Parser:
    def __init__(self, tokens, env):
        self.tokens = tokens
        self.pos = 0
        self.error = False
        self.env = env

    def parse_program(self):
        # <program> -> { <statement> }
        prints = []
        while self.pos < len(self.tokens) and not self.error:
            self.parse_statement(prints)
        return prints

    def is_error(self):
        return self.error

    def has_more_tokens(self):
        return self.pos < len(self.tokens)

    def parse_statement(self, prints):
        # <statement> -> print <var> ; | <var> = <expr> ;
        if self.current_token() == "print":
            self.get_token()  # consume 'print'
            if not self.has_more_tokens():
                self.set_error(); return
            var_name = self.current_token()
            if not self.is_valid_var(var_name):
                self.set_error(); return
            self.get_token()  # consume variable
            if not self.match_token(";"):
                self.set_error(); return
            val = self.env.get(var_name, 0)
            prints.append(val)
        else:
            var_name = self.current_token()
            if not self.is_valid_var(var_name):
                self.set_error(); return
            self.get_token()  # consume variable
            if not self.match_token("="):
                self.set_error(); return
            expr_val = self.parse_expr()
            if self.error: return
            if not self.match_token(";"):
                self.set_error(); return
            self.env[var_name] = expr_val

    def parse_expr(self):
        # <expr> -> <term> { + <term> | * <term> }
        value = self.parse_term()
        while not self.error and self.has_more_tokens() and self.current_token() in ("+", "*"):
            op = self.get_token()
            rhs = self.parse_term()
            if self.error: return 0
            value = value + rhs if op == "+" else value * rhs
        return value

    def parse_term(self):
        # <term> -> <factor> { - <factor> }
        value = self.parse_factor()
        while not self.error and self.has_more_tokens() and self.current_token() == "-":
            self.get_token()  # consume '-'
            rhs = self.parse_factor()
            value = value - rhs
        return value

    def parse_factor(self):
        # <factor> -> [ - ] ( <number> | <var> | '(' <expr> ')' )
        unary_negative = False
        if self.has_more_tokens() and self.current_token() == "-":
            unary_negative = True
            self.get_token()
        if not self.has_more_tokens():
            self.set_error(); return 0
        tok = self.current_token()
        if tok == "(":
            self.get_token()
            value = self.parse_expr()
            if self.error: return 0
            if not self.match_token(")"):
                self.set_error(); return 0
        elif self.is_number(tok):
            if len(tok) > 10:
                self.set_error(); return 0
            try:
                value = int(tok)
            except ValueError:
                self.set_error(); return 0
            self.get_token()
        elif self.is_valid_var(tok):
            if len(tok) > 10:
                self.set_error(); return 0
            value = self.env.get(tok, 0)
            self.get_token()
        else:
            self.set_error(); return 0
        if unary_negative:
            value = -value
        return value

    def current_token(self):
        if self.pos < len(self.tokens):
            return self.tokens[self.pos]
        return ""

    def get_token(self):
        if self.pos < len(self.tokens):
            tok = self.tokens[self.pos]
            self.pos += 1
            return tok
        self.set_error()
        return ""

    def match_token(self, expected):
        if self.has_more_tokens() and self.current_token() == expected:
            self.get_token()
            return True
        return False

    def set_error(self):
        self.error = True

    def is_number(self, s):
        return s.isdigit()

    def is_valid_var(self, s):
        if not s or len(s) > 10 or s == "print":
            return False
        return all('a' <= c <= 'z' for c in s)


def tokenize(line):
    return line.strip().split()


def main():
    for line in sys.stdin:
        line = line.rstrip('\n')
        if not line.strip():
            break
        tokens = tokenize(line)
        env = {}
        parser = Parser(tokens, env)
        outputs = parser.parse_program()
        if parser.is_error() or parser.has_more_tokens():
            print("Syntax Error!")
        elif outputs:
            print(" ".join(str(x) for x in outputs))

if __name__ == "__main__":
    main()
