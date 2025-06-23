## Recursive-Descent Parser Interpreter

### 개요  
주어진 EBNF 문법에 맞는 언어를 위한 해석기를 C++과 Python으로 각각 구현함.  
입력된 소스 코드를 토크나이즈하고, Recursive Descent 방식으로 문법을 분석한 후 실행.

---

### 구현1: 단순 대입 및 산술 표현식 해석기

**지원 문법** (EBNF)
```
<program>     → {<statement>}
<statement>   → <var> = <expr> ; | print <var> ;
<expr>        → <term> {+ <term> | * <term>}
<term>        → <factor> {- <factor>}
<factor>      → [ - ] ( <number> | <var> | ‘(’<expr>‘)’ )
<number>      → <digit> {<digit>}
<digit>       → 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
<var>         → <alphabet>{<alphabet>}
<alphabet>    → a | b | c | d | e | f | g | h | i | j | k | l | m | n | o | p | q | r | 
s | t | u | v | w | x | y | z
```

**동작 방식**
- 공백 단위로 입력 분리 후, 구문 분석을 통해 실행 또는 오류 처리
- 대입되지 않은 변수는 0으로 초기화됨
- 산술 연산은 **왼쪽 결합성** (Left Associativity)
- 변수 및 숫자는 최대 10자 제한
- 문법 오류 시: `"Syntax Error!"` 출력

**예시**
```plaintext
x = - 12 + 3 ; print x ;
 >> -9
a = 5 ; b = 3 ; print a ;
 >> 5
x = ( 12 + 3 ;      // 오류
 >> Syntax Error!
```

---

### 구현 2: 선언문, 조건문, 반복문 포함 확장 해석기

**지원 문법 (EBNF 확장)**
```
<program>     → {<declaration>} {<statement>}
<declaration> → integer <var> ;
<statement>   → <var> = <aexpr> ; | print <aexpr> ; |
                while ( <bexpr> ) do { {<statement>} } ;
                if ( <bexpr> ) { {<statement>} } else { {<statement>} } ;
<bexpr>       → <var> <relop> <var>    // 관계 연산자: == != < >
<relop>       → == | != | < | >
<aexpr>       → <term> {( + | - ) <term>}
<term>        → <factor> { * <factor>}
<factor>      → [ - ] ( <number> | <var> | ‘(’<aexpr>‘)’ )
<type>        → integer
<number>      → <digit> {<digit>}
<digit>       → 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
<var>         → <alphabet>{<alphabet>}
<alphabet>    → a | b | c | d |
```

**특징**
- 변수 선언 필수 (`integer foo;`) — 선언 안된 변수 사용 시 오류
- 관계 연산을 포함한 조건문, 반복문, 중첩 if/while 지원
- `evalFlag` 플래그를 통해 문법 검사와 실제 실행 단계 구분
- 모든 선언된 변수는 0으로 초기화

**예시**
```plaintext
integer x ; x = 10 + 5 ; print x ;
 >> 15

integer k ; integer j ; k = 30 ; j = 25 ; while ( k > j ) do { print ( k - j ) * 10 ; k = k – 1 ; } ; print k ;
 >> 50 40 30 20 10 25 

if ( x > 0 ) { print x ; } else { print 0 ; } ;
 >> 15
```

---

### 공통 구현 방식 (C++ / Python)

- **토크나이저**: 공백 단위로 토큰 분리, 잘못된 토큰 검사
- **파서**: Recursive Descent 방식  
  - `parseExpr`, `parseTerm`, `parseFactor` 로 우선순위 제어  
  - `parseStatement`, `parseProgram` 로 전체 실행 흐름 관리
- **에러 처리**: 문법 위반 또는 남은 토큰이 존재할 경우 `"Syntax Error!"` 출력
- **실행 흐름**:  
  1. 문법 검사(pass1): 실행 없이 구문 분석  
  2. 실행 단계(pass2): 변수 평가 및 출력

---
