import sys

# 토큰 종류
def enum_token_types():
    class TokenType:
        END = 'END';    IDENT = 'IDENT';  NUMBER = 'NUMBER'
        KW_INTEGER = 'KW_INTEGER'; KW_PRINT = 'KW_PRINT'
        KW_WHILE = 'KW_WHILE';     KW_DO = 'KW_DO'
        KW_IF = 'KW_IF';           KW_ELSE = 'KW_ELSE'
        OP_PLUS = 'OP_PLUS';       OP_MINUS = 'OP_MINUS';   OP_MUL = 'OP_MUL'
        OP_EQ = 'OP_EQ';           OP_NEQ = 'OP_NEQ'
        OP_LT = 'OP_LT';           OP_GT = 'OP_GT'
        OP_ASSIGN = 'OP_ASSIGN'
        SEMICOLON = 'SEMICOLON'
        LPAREN = 'LPAREN';         RPAREN = 'RPAREN'
        LBRACE = 'LBRACE';         RBRACE = 'RBRACE'
        UNKNOWN = 'UNKNOWN'
    return TokenType

TokenType = enum_token_types()

# 토큰 구조
class Token:
    def __init__(self, ttype, lexeme):
        self.type = ttype
        self.lexeme = lexeme

# 전역 상태
tokens = []
pos = 0
errorFlag = False
printed = False
sym = {}  # symbol table: var -> int
outputs = []

# 에러 표시
def error():
    global errorFlag
    errorFlag = True

# 현재 토큰 반환
def cur():
    global pos, tokens
    return tokens[pos]

# 매칭 시 포지션 이동
def match(ttype):
    global pos
    if cur().type == ttype:
        pos += 1
        return True
    return False

# 문자 검사
def is_letter(c): return 'a' <= c <= 'z'
def is_digit(c):  return '0' <= c <= '9'

# 토크나이저
def tokenize(line):
    out = []
    for w in line.split():
        tk = Token(TokenType.UNKNOWN, w)
        if   w == 'integer':   tk.type = TokenType.KW_INTEGER
        elif w == 'print':     tk.type = TokenType.KW_PRINT
        elif w == 'while':     tk.type = TokenType.KW_WHILE
        elif w == 'do':        tk.type = TokenType.KW_DO
        elif w == 'if':        tk.type = TokenType.KW_IF
        elif w == 'else':      tk.type = TokenType.KW_ELSE
        elif w == '+':         tk.type = TokenType.OP_PLUS
        elif w == '-':         tk.type = TokenType.OP_MINUS
        elif w == '*':         tk.type = TokenType.OP_MUL
        elif w == '==':        tk.type = TokenType.OP_EQ
        elif w == '!=':        tk.type = TokenType.OP_NEQ
        elif w == '<':         tk.type = TokenType.OP_LT
        elif w == '>':         tk.type = TokenType.OP_GT
        elif w == '=':         tk.type = TokenType.OP_ASSIGN
        elif w == ';':         tk.type = TokenType.SEMICOLON
        elif w == '(':         tk.type = TokenType.LPAREN
        elif w == ')':         tk.type = TokenType.RPAREN
        elif w == '{':         tk.type = TokenType.LBRACE
        elif w == '}':         tk.type = TokenType.RBRACE
        elif w[0].isdigit():
            if len(w) > 10 or not all(is_digit(c) for c in w):
                error()
            else:
                tk.type = TokenType.NUMBER
        elif w[0].isalpha():
            if len(w) > 10 or not all(is_letter(c) for c in w):
                error()
            else:
                tk.type = TokenType.IDENT
        else:
            error()
        out.append(tk)
    out.append(Token(TokenType.END, ""))
    return out

# 파서 전방 선언
def parseProgram(execFlag=True):
    global pos, tokens, errorFlag
    # 선언문
    while not errorFlag and cur().type == TokenType.KW_INTEGER:
        parseDeclaration()
    # 문장
    while not errorFlag and cur().type in (
        TokenType.IDENT, TokenType.KW_PRINT,
        TokenType.KW_WHILE, TokenType.KW_IF):
        parseStatement(execFlag)

# 선언문
def parseDeclaration():
    global pos, sym
    match(TokenType.KW_INTEGER)
    if cur().type != TokenType.IDENT:
        error(); return
    var = cur().lexeme; pos += 1
    if not match(TokenType.SEMICOLON):
        error(); return
    sym[var] = 0

# 문장
def parseStatement(execFlag=True):
    global pos, tokens, errorFlag, outputs, sym
    # 대입
    if cur().type == TokenType.IDENT:
        var = cur().lexeme; pos += 1
        if var not in sym or not match(TokenType.OP_ASSIGN): error(); return
        val = parseAExpr()
        if not match(TokenType.SEMICOLON): error(); return
        if not errorFlag and execFlag: sym[var] = val
    # print
    elif match(TokenType.KW_PRINT):
        val = parseAExpr()
        if not match(TokenType.SEMICOLON): error(); return
        if not errorFlag and execFlag:
            outputs.append(str(val))
    # while-do
    elif match(TokenType.KW_WHILE):
    # 1) 구조만 파싱 (조건식과 중괄호 매칭)
        if not match(TokenType.LPAREN): error(); return
        condStart = pos
        parseBExpr()
        if not match(TokenType.RPAREN) or not match(TokenType.KW_DO): error(); return
        if not match(TokenType.LBRACE): error(); return

    # 중괄호 짝 찾기
        depth = 1
        scan = pos
        while depth > 0 and tokens[scan].type != TokenType.END:
            if tokens[scan].type == TokenType.LBRACE:
                depth += 1
            elif tokens[scan].type == TokenType.RBRACE:
                depth -= 1
            scan += 1
        if depth != 0:
            error(); return

    # 본문 토큰 슬라이스
        blockTokens = tokens[pos:scan-1] + [Token(TokenType.END, "")]

        if not execFlag:
            savedTokens2, savedPos2 = tokens, pos       # errorFlag는 건드리지 않음
            tokens, pos = blockTokens[:], 0
            while not errorFlag and cur().type != TokenType.END:
                parseStatement(False)
            tokens, pos = savedTokens2, savedPos2

    # } 이동한 뒤 세미콜론 소비
        pos = scan
        if not match(TokenType.SEMICOLON):
            error(); return

    # 2 execFlag=True 일 때만 실제 반복 실행
        if execFlag:
            savedSym   = sym.copy()
            savedTokens, savedPos, savedErr = tokens[:], pos, errorFlag

            while True:
            # (a) 조건 재평가
                tokens, pos, errorFlag = savedTokens[:], condStart, savedErr
                condOK = parseBExpr()
                if errorFlag:
                    return
                if not condOK:
                # 조건 거짓: 상태 복원 후 반복 종료
                    tokens, pos, errorFlag, sym = savedTokens[:], savedPos, savedErr, savedSym
                    break

            # (b) 본문 실행
                sym = savedSym.copy()
                tokens, pos = blockTokens, 0
                while not errorFlag and cur().type != TokenType.END:
                    parseStatement(execFlag)
                    if errorFlag:
                        return

            # 다음 반복을 위해 상태 저장 및 복원
                savedSym = sym.copy()
                tokens, pos, errorFlag = savedTokens[:], savedPos, savedErr
    # if-else
    elif match(TokenType.KW_IF):
        if not match(TokenType.LPAREN): error(); return
        cond = parseBExpr()
        if not match(TokenType.RPAREN): error(); return
        # then
        if not match(TokenType.LBRACE): error(); return
        while not errorFlag and cur().type != TokenType.RBRACE:
            parseStatement(execFlag and cond)
        match(TokenType.RBRACE)
        if not match(TokenType.KW_ELSE) or not match(TokenType.LBRACE): error(); return
        while not errorFlag and cur().type != TokenType.RBRACE:
            parseStatement(execFlag and not cond)
        match(TokenType.RBRACE)
        if not match(TokenType.SEMICOLON): error(); return
    else:
        error()

# 비교식
def parseBExpr():
    global pos, sym, errorFlag
    if cur().type != TokenType.IDENT: error(); return 0
    v1 = cur().lexeme; pos += 1
    if v1 not in sym: error(); return 0
    op = cur().type; pos += 1
    if op not in (TokenType.OP_EQ, TokenType.OP_NEQ, TokenType.OP_LT, TokenType.OP_GT): error(); return 0
    if cur().type != TokenType.IDENT: error(); return 0
    v2 = cur().lexeme; pos += 1
    if v2 not in sym: error(); return 0
    a, b = sym[v1], sym[v2]
    if op == TokenType.OP_EQ: return a == b
    if op == TokenType.OP_NEQ: return a != b
    if op == TokenType.OP_LT: return a < b
    if op == TokenType.OP_GT: return a > b
    return 0

# 산술식
def parseAExpr():
    global pos
    val = parseTerm()
    while not errorFlag and cur().type in (TokenType.OP_PLUS, TokenType.OP_MINUS):
        op = cur().type; pos += 1
        rhs = parseTerm()
        val = val + rhs if op == TokenType.OP_PLUS else val - rhs
    return val

def parseTerm():
    global pos
    val = parseFactor()
    while not errorFlag and cur().type == TokenType.OP_MUL:
        pos += 1
        val *= parseFactor()
    return val


def parseFactor():
    global pos
    neg = False
    if match(TokenType.OP_MINUS): neg = True
    if cur().type == TokenType.NUMBER:
        val = int(cur().lexeme); pos += 1
    elif cur().type == TokenType.IDENT:
        v = cur().lexeme; pos += 1
        if v not in sym: error(); return 0
        val = sym[v]
    elif match(TokenType.LPAREN):
        val = parseAExpr()
        if not match(TokenType.RPAREN): error(); return 0
    else:
        error(); return 0
    return -val if neg else val

# 메인: 두 단계(pass) for 문법+실행
if __name__ == '__main__':
    for line in sys.stdin:
        line = line.strip()
        if not line:
            break

    # 1) tokenize 한 번만 하고, 각 pass 에서 복사해서 쓴다
        full_tokens = tokenize(line)

    # --- 문법 검사용 pass (execFlag=False) ---
        tokens    = full_tokens[:]
        pos       = 0
        errorFlag = False
        sym.clear()
        parseProgram(execFlag=False)
        if not errorFlag and cur().type != TokenType.END:
            errorFlag = True

        if errorFlag:
            print("Syntax Error!")
            continue

    # --- 실행용 pass (execFlag=True) ---
        tokens    = full_tokens[:]
        pos       = 0
        errorFlag = False
        sym.clear()
        outputs   = []
        parseProgram(execFlag=True)

        if errorFlag or cur().type != TokenType.END:
            print("Syntax Error!")
        else:
            if outputs:
                print(" ".join(outputs), end=" ")
                print()

            
    