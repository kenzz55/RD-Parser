#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
using namespace std;

static ostringstream out;
static bool evalEnabled = true;
enum class TokenType {
    END, IDENT, NUMBER,
    KW_INTEGER, KW_PRINT, KW_WHILE, KW_DO, KW_IF, KW_ELSE,
    OP_PLUS, OP_MINUS, OP_MUL,
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_ASSIGN,
    SEMICOLON, LPAREN, RPAREN, LBRACE, RBRACE,
    UNKNOWN
};

struct Token {
    TokenType type;
    string    lexeme;
};

vector<Token> tokens;
int pos;
bool errorFlag;
bool printed;
unordered_map<string, long long> sym;

// 유틸리티
void error() { errorFlag = true; }
Token& cur() { return tokens[pos]; }
bool match(TokenType t) {
    if (cur().type == t) { pos++; return true; }
    return false;
}
bool isLetter(char c) { return c >= 'a' && c <= 'z'; }
bool isDigit(char c) { return c >= '0' && c <= '9'; }

// 토크나이저 (공백 구분)
vector<Token> tokenize(const string& line) {
    vector<Token> out;
    istringstream ss(line);
    string w;
    while (ss >> w) {
        Token tk{ TokenType::UNKNOWN, w };
        if (w == "integer") tk.type = TokenType::KW_INTEGER;
        else if (w == "print")   tk.type = TokenType::KW_PRINT;
        else if (w == "while")   tk.type = TokenType::KW_WHILE;
        else if (w == "do")      tk.type = TokenType::KW_DO;
        else if (w == "if")      tk.type = TokenType::KW_IF;
        else if (w == "else")    tk.type = TokenType::KW_ELSE;
        else if (w == "+")       tk.type = TokenType::OP_PLUS;
        else if (w == "-")       tk.type = TokenType::OP_MINUS;
        else if (w == "*")       tk.type = TokenType::OP_MUL;
        else if (w == "==")      tk.type = TokenType::OP_EQ;
        else if (w == "!=")      tk.type = TokenType::OP_NEQ;
        else if (w == "<")       tk.type = TokenType::OP_LT;
        else if (w == ">")       tk.type = TokenType::OP_GT;
        else if (w == "=")       tk.type = TokenType::OP_ASSIGN;
        else if (w == ";")       tk.type = TokenType::SEMICOLON;
        else if (w == "(")       tk.type = TokenType::LPAREN;
        else if (w == ")")       tk.type = TokenType::RPAREN;
        else if (w == "{")       tk.type = TokenType::LBRACE;
        else if (w == "}")       tk.type = TokenType::RBRACE;
        else if (isDigit(w[0])) {
            if (w.size() > 10) error();
            for (char c : w) if (!isDigit(c)) error();
            tk.type = TokenType::NUMBER;
        }
        else if (isLetter(w[0])) {
            if (w.size() > 10) error();
            for (char c : w) if (!isLetter(c)) error();
            tk.type = TokenType::IDENT;
        }
        else error();
        out.push_back(tk);
    }
    out.push_back({ TokenType::END,"" });
    return out;
}

// 전방 선언
void parseProgram();
void parseDeclaration();
void parseStatement();
int  parseBExpr();
long long parseAExpr();
long long parseTerm();
long long parseFactor();

// <program> → {<declaration>} {<statement>}
void parseProgram() {
    while (!errorFlag && cur().type == TokenType::KW_INTEGER)
        parseDeclaration();
    while (!errorFlag && (
        cur().type == TokenType::IDENT ||
        cur().type == TokenType::KW_PRINT ||
        cur().type == TokenType::KW_WHILE ||
        cur().type == TokenType::KW_IF))
        parseStatement();
}

// <declaration> → integer <var> ;
void parseDeclaration() {
    match(TokenType::KW_INTEGER);
    if (cur().type != TokenType::IDENT) { error(); return; }
    string var = cur().lexeme; pos++;
    if (!match(TokenType::SEMICOLON)) { error(); return; }
    sym[var] = 0;
}

// <statement> → 대입 | print | while | if-else
void parseStatement() {
    // 대입
    if (cur().type == TokenType::IDENT) {
        string var = cur().lexeme; pos++;
        if (!sym.count(var)) { error(); return; }
        if (!match(TokenType::OP_ASSIGN)) { error(); return; }
        long long val = parseAExpr();
        if (!match(TokenType::SEMICOLON)) { error(); return; }
        if (!errorFlag && evalEnabled) sym[var] = val;
    }
    // print
    else if (match(TokenType::KW_PRINT)) {
        long long val = parseAExpr();
        if (!match(TokenType::SEMICOLON)) { error(); return; }
        if (!errorFlag && evalEnabled) {
            out << val << " ";
            printed = true;
        }
    }
    // while
    else if (match(TokenType::KW_WHILE)) {
        // 1) 조건 파싱
        if (!match(TokenType::LPAREN)) { error(); return; }
        int condStartPos = pos;
        parseBExpr();
        if (!match(TokenType::RPAREN)) { error(); return; }
        if (!match(TokenType::KW_DO)) { error(); return; }

        // 2) 블록 토큰 슬라이스
        if (!match(TokenType::LBRACE)) { error(); return; }
        int blockStart = pos, depth = 1, scan = pos;
        while (depth > 0 && tokens[scan].type != TokenType::END) {
            if (tokens[scan].type == TokenType::LBRACE)  depth++;
            else if (tokens[scan].type == TokenType::RBRACE) depth--;
            scan++;
        }
        int blockEnd = scan - 1;
        vector<Token> blockTokens(
            tokens.begin() + blockStart,
            tokens.begin() + blockEnd
        );
        blockTokens.push_back({ TokenType::END,"" });

        // ── grammar-only 검사 ──
        if (!evalEnabled) {
            auto  savedTokens = tokens;
            int   savedPos = pos;
            bool  savedErr = errorFlag;
            tokens = blockTokens;
            pos = 0;
            errorFlag = false;
            while (!errorFlag && cur().type != TokenType::END)
                parseStatement();
            errorFlag = savedErr || errorFlag;
            tokens = savedTokens;
            pos = blockEnd;
            match(TokenType::RBRACE);
            match(TokenType::SEMICOLON);
            return;
        }
        // ─────────────────────

        // 3) 정상 매치
        pos = blockEnd;
        if (!match(TokenType::RBRACE)) { error(); return; }
        if (!match(TokenType::SEMICOLON)) { error(); return; }

        // 4) 실제 실행 루프
        bool prevEval = evalEnabled;
        auto savedTokensAll = tokens;
        int  savedPosAll = pos;
        bool savedErrAll = errorFlag;

        while (true) {
            // (a) 조건 재평가
            pos = condStartPos;
            tokens = savedTokensAll;
            errorFlag = false;
            int cv = parseBExpr();

            // (b) 거짓이거나 에러면 종료
            tokens = savedTokensAll;
            errorFlag = savedErrAll;
            if (cv == 0 || errorFlag) {
                pos = savedPosAll;
                break;
            }

            // (c) 블록 실행
            auto backupTokens = tokens;
            bool backupErr = errorFlag;
            tokens = blockTokens;
            pos = 0;
            errorFlag = false;
            while (!errorFlag && cur().type != TokenType::END)
                parseStatement();
            tokens = backupTokens;
            errorFlag = backupErr;
            pos = savedPosAll;
        }

        // 5) evalEnabled 복구
        evalEnabled = prevEval;
    }
    // if-else
    else if (match(TokenType::KW_IF)) {
        if (!match(TokenType::LPAREN)) { error(); return; }
        int cond = parseBExpr();
        if (!match(TokenType::RPAREN)) { error(); return; }

        bool prevEval = evalEnabled;
        if (!match(TokenType::LBRACE)) { error(); return; }
        evalEnabled = prevEval && cond;
        while (!errorFlag && cur().type != TokenType::RBRACE)
            parseStatement();
        match(TokenType::RBRACE);

        if (!match(TokenType::KW_ELSE)) { error(); return; }
        if (!match(TokenType::LBRACE)) { error(); return; }
        evalEnabled = prevEval && !cond;
        while (!errorFlag && cur().type != TokenType::RBRACE)
            parseStatement();
        match(TokenType::RBRACE);

        evalEnabled = prevEval;
        if (!match(TokenType::SEMICOLON)) { error(); return; }
    }
    else {
        error();
    }
}

// <bexpr> → <var> <relop> <var>
int parseBExpr() {
    if (cur().type != TokenType::IDENT) { error(); return 0; }
    string v1 = cur().lexeme; pos++;
    if (!sym.count(v1)) { error(); return 0; }
    TokenType op = cur().type; pos++;
    if (!(op == TokenType::OP_EQ || op == TokenType::OP_NEQ ||
        op == TokenType::OP_LT || op == TokenType::OP_GT)) {
        error(); return 0;
    }
    if (cur().type != TokenType::IDENT) { error(); return 0; }
    string v2 = cur().lexeme; pos++;
    if (!sym.count(v2)) { error(); return 0; }
    long long a = sym[v1], b = sym[v2];
    switch (op) {
    case TokenType::OP_EQ:  return a == b;
    case TokenType::OP_NEQ: return a != b;
    case TokenType::OP_LT:  return a < b;
    case TokenType::OP_GT:  return a > b;
    default: return 0;
    }
}

// <aexpr>, <term>, <factor> 구현
long long parseAExpr() {
    long long val = parseTerm();
    while (!errorFlag &&
        (cur().type == TokenType::OP_PLUS ||
            cur().type == TokenType::OP_MINUS)) {
        TokenType op = cur().type; pos++;
        long long rhs = parseTerm();
        val = (op == TokenType::OP_PLUS ? val + rhs : val - rhs);
    }
    return val;
}
long long parseTerm() {
    long long val = parseFactor();
    while (!errorFlag && cur().type == TokenType::OP_MUL) {
        pos++;
        val *= parseFactor();
    }
    return val;
}
long long parseFactor() {
    bool neg = false;
    if (match(TokenType::OP_MINUS)) neg = true;
    long long val = 0;
    if (cur().type == TokenType::NUMBER) {
        val = stoll(cur().lexeme); pos++;
    }
    else if (cur().type == TokenType::IDENT) {
        string v = cur().lexeme; pos++;
        if (!sym.count(v)) { error(); return 0; }
        val = sym[v];
    }
    else if (match(TokenType::LPAREN)) {
        val = parseAExpr();
        if (!match(TokenType::RPAREN)) { error(); return 0; }
    }
    else {
        error();
        return 0;
    }
    return neg ? -val : val;
}

// --- 메인: 두 단계(pass)로 문법 검사 → 실행 ---
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    while (getline(cin, line)) {
        if (line.empty()) break;

        tokens = tokenize(line);

        // --- Pass 1: 문법 전용 ---
        out.str(""); out.clear();
        sym.clear();
        printed = false;
        evalEnabled = false;
        pos = 0;
        errorFlag = false;
        parseProgram();
        if (errorFlag || cur().type != TokenType::END) {
            cout << "Syntax Error!\n";
            continue;
        }

        // --- Pass 2: 실제 실행 ---
        out.str(""); out.clear();
        sym.clear();
        printed = false;
        evalEnabled = true;
        pos = 0;
        errorFlag = false;
        parseProgram();
        if (errorFlag || cur().type != TokenType::END) {
            cout << "Syntax Error!\n";
        }
        else if (printed) {
            cout << out.str() << "\n";
        }
    }

    return 0;
}
