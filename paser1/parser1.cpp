#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cctype>

using namespace std;

// RD Parser Ŭ����
class Parser {
public:
    Parser(const vector<string>& tokens, map<string, long long>& env)
        : tokens(tokens), pos(0), error(false), env(env) {
    }

    // ���α׷� ��ü (<program> -> { <statement> })
    // ���� �Է� �� ���� ��� ������ �Ľ��ϸ�, print ���� ������� ���Ϳ� �����
    vector<long long> parseProgram() {
        vector<long long> prints;
        while (pos < tokens.size() && !error) {
            parseStatement(prints);
        }
        return prints;
    }

    // isError()�� true�̸� ���� ������ ������ �ǹ�
    bool isError() const {
        return error;
    }

    // �ܺο��� ���� ��ū ���θ� Ȯ���� �ʿ䰡 ������ ���ο��� ��� ó����
    bool hasMoreTokens() const {
        return pos < tokens.size();
    }

public:
    // <statement> �� <var> = <expr> ; | print <var> ;
    void parseStatement(vector<long long>& prints) {
        if (currentToken() == "print") {
            getToken(); // "print" ��ū �Һ�
            if (!hasMoreTokens()) { setError(); return; }
            string varName = currentToken();
            if (!isValidVar(varName)) { setError(); return; }
            getToken(); // ���� ��ū �Һ�
            if (!matchToken(";")) { setError(); return; }
            // ���� �Է� ������ ���Ե��� ���� ������ 0���� ����
            long long val = (env.count(varName) ? env[varName] : 0);
            prints.push_back(val);
        }
        else {
            // ���Թ�: <var> = <expr> ;
            string varName = currentToken();
            if (!isValidVar(varName)) { setError(); return; }
            getToken(); // ���� ��ū �Һ�
            if (!matchToken("=")) { setError(); return; }
            long long exprVal = parseExpr();
            if (error) return;
            if (!matchToken(";")) { setError(); return; }
            env[varName] = exprVal;
        }
    }

    // <expr> �� <term> { + <term> | * <term> }
    long long parseExpr() {
        long long value = parseTerm();
        while (!error && hasMoreTokens() &&
            (currentToken() == "+" || currentToken() == "*")) {
            string op = getToken();
            long long rhs = parseTerm();
            if (error) return 0;
            if (op == "+")
                value = value + rhs;
            else if (op == "*")
                value = value * rhs;
        }
        return value;
    }

    // <term> �� <factor> { - <factor> }
    long long parseTerm() {
        long long value = parseFactor();
        while (!error && hasMoreTokens() && currentToken() == "-") {
            getToken(); // '-' �Һ�
            long long rhs = parseFactor();
            value = value - rhs;
        }
        return value;
    }

    // <factor> �� [ - ] ( <number> | <var> | '(' <expr> ')' )
    long long parseFactor() {
        bool unaryNegative = false;
        if (hasMoreTokens() && currentToken() == "-") {
            unaryNegative = true;
            getToken(); // ����ary '-' �Һ�
        }
        if (!hasMoreTokens()) { setError(); return 0; }
        long long value = 0;
        string tok = currentToken();
        if (tok == "(") {
            getToken(); // '(' �Һ�
            value = parseExpr();
            if (error) return 0;
            if (!matchToken(")")) { setError(); return 0; }
        }
        else if (isNumber(tok)) {
            // <number>�� ���� ����(�ִ� 10�ڸ�)
            if (tok.size() > 10) { setError(); return 0; }
            try {
                value = stoll(tok);
            }
            catch (...) {
                setError();
                return 0;
            }
            getToken(); // ���� ��ū �Һ�
        }
        else if (isValidVar(tok)) {
            // <var>�� ���� ����(�ִ� 10����)
            if (tok.size() > 10) { setError(); return 0; }
            value = (env.count(tok) ? env[tok] : 0);
            getToken(); // ���� ��ū �Һ�
        }
        else {
            setError();
            return 0;
        }
        if (unaryNegative) value = -value;
        return value;
    }

    string currentToken() const {
        if (pos < tokens.size())
            return tokens[pos];
        return "";
    }

    string getToken() {
        if (pos < tokens.size())
            return tokens[pos++];
        setError();
        return "";
    }

    bool matchToken(const string& expected) {
        if (hasMoreTokens() && currentToken() == expected) {
            getToken();
            return true;
        }
        return false;
    }

    void setError() { error = true; }

    bool isNumber(const string& s) const {
        if (s.empty()) return false;
        for (char c : s) {
            if (!isdigit(c))
                return false;
        }
        return true;
    }

    bool isValidVar(const string& s) const {
        if (s.empty() || s.size() > 10)
            return false;
        for (char c : s) {
            if (c < 'a' || c > 'z')
                return false;
        }
        // ����� "print"�� ���������� ��� �Ұ�
        if (s == "print")
            return false;
        return true;
    }

    vector<string> tokens;
    size_t pos;
    bool error;
    map<string, long long>& env;
};

// ��ūȭ: �Է� ���� ���� �������� �и���
vector<string> tokenize(const string& line) {
    vector<string> tokens;
    istringstream iss(line);
    string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    while (true) {
        // ������Ʈ ��� (�ʿ��)
        // cout << ">> ";
        if (!getline(cin, line))
            break;
        // �� ���̸� ����
        if (line.find_first_not_of(" \t") == string::npos)
            break;

        // ��ū �и�
        vector<string> tokens = tokenize(line);

        // �� �Է��� ������ ����: �� �Է¸��� ���ο� ȯ���� �����
        map<string, long long> localEnv;
        Parser parser(tokens, localEnv);
        vector<long long> outputs = parser.parseProgram();

        // ���� �Ľ� �� ������ �߻��ϰų� ��� ��ū�� �Һ����� ���� ���
        if (parser.isError() || parser.hasMoreTokens()) {
            cout << "Syntax Error!" << "\n";
        }
        else {
            if (!outputs.empty()) {
                for (size_t i = 0; i < outputs.size(); i++) {
                    cout << outputs[i];
                    if (i < outputs.size() - 1)
                        cout << " ";
                }
                cout << "\n";
            }
        }
    }
    return 0;
}
