#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cctype>

using namespace std;

// RD Parser 클래스
class Parser {
public:
    Parser(const vector<string>& tokens, map<string, long long>& env)
        : tokens(tokens), pos(0), error(false), env(env) {
    }

    // 프로그램 전체 (<program> -> { <statement> })
    // 현재 입력 줄 내의 모든 문장을 파싱하며, print 문의 결과값은 벡터에 저장됨
    vector<long long> parseProgram() {
        vector<long long> prints;
        while (pos < tokens.size() && !error) {
            parseStatement(prints);
        }
        return prints;
    }

    // isError()가 true이면 문법 에러가 있음을 의미
    bool isError() const {
        return error;
    }

    // 외부에서 남은 토큰 여부를 확인할 필요가 없도록 내부에서 모두 처리함
    bool hasMoreTokens() const {
        return pos < tokens.size();
    }

public:
    // <statement> → <var> = <expr> ; | print <var> ;
    void parseStatement(vector<long long>& prints) {
        if (currentToken() == "print") {
            getToken(); // "print" 토큰 소비
            if (!hasMoreTokens()) { setError(); return; }
            string varName = currentToken();
            if (!isValidVar(varName)) { setError(); return; }
            getToken(); // 변수 토큰 소비
            if (!matchToken(";")) { setError(); return; }
            // 동일 입력 내에서 대입되지 않은 변수는 0으로 간주
            long long val = (env.count(varName) ? env[varName] : 0);
            prints.push_back(val);
        }
        else {
            // 대입문: <var> = <expr> ;
            string varName = currentToken();
            if (!isValidVar(varName)) { setError(); return; }
            getToken(); // 변수 토큰 소비
            if (!matchToken("=")) { setError(); return; }
            long long exprVal = parseExpr();
            if (error) return;
            if (!matchToken(";")) { setError(); return; }
            env[varName] = exprVal;
        }
    }

    // <expr> → <term> { + <term> | * <term> }
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

    // <term> → <factor> { - <factor> }
    long long parseTerm() {
        long long value = parseFactor();
        while (!error && hasMoreTokens() && currentToken() == "-") {
            getToken(); // '-' 소비
            long long rhs = parseFactor();
            value = value - rhs;
        }
        return value;
    }

    // <factor> → [ - ] ( <number> | <var> | '(' <expr> ')' )
    long long parseFactor() {
        bool unaryNegative = false;
        if (hasMoreTokens() && currentToken() == "-") {
            unaryNegative = true;
            getToken(); // 유니ary '-' 소비
        }
        if (!hasMoreTokens()) { setError(); return 0; }
        long long value = 0;
        string tok = currentToken();
        if (tok == "(") {
            getToken(); // '(' 소비
            value = parseExpr();
            if (error) return 0;
            if (!matchToken(")")) { setError(); return 0; }
        }
        else if (isNumber(tok)) {
            // <number>의 길이 제한(최대 10자리)
            if (tok.size() > 10) { setError(); return 0; }
            try {
                value = stoll(tok);
            }
            catch (...) {
                setError();
                return 0;
            }
            getToken(); // 숫자 토큰 소비
        }
        else if (isValidVar(tok)) {
            // <var>의 길이 제한(최대 10문자)
            if (tok.size() > 10) { setError(); return 0; }
            value = (env.count(tok) ? env[tok] : 0);
            getToken(); // 변수 토큰 소비
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
        // 예약어 "print"는 변수명으로 사용 불가
        if (s == "print")
            return false;
        return true;
    }

    vector<string> tokens;
    size_t pos;
    bool error;
    map<string, long long>& env;
};

// 토큰화: 입력 줄을 공백 기준으로 분리함
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
        // 프롬프트 출력 (필요시)
        // cout << ">> ";
        if (!getline(cin, line))
            break;
        // 빈 줄이면 종료
        if (line.find_first_not_of(" \t") == string::npos)
            break;

        // 토큰 분리
        vector<string> tokens = tokenize(line);

        // 각 입력은 독립적 실행: 매 입력마다 새로운 환경을 사용함
        map<string, long long> localEnv;
        Parser parser(tokens, localEnv);
        vector<long long> outputs = parser.parseProgram();

        // 만약 파싱 중 에러가 발생하거나 모든 토큰을 소비하지 못한 경우
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
