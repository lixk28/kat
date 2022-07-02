# The Kat Programming Language

## Language Syntax

The kat programming language syntax described in [EBNF](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form).

identifier
- An identifier can only be composed of letters, digits and underscores
- An identifier must begin with letters or underscore
```
identifier = (letter | underscore) , {letter | underscore | digit} ;
letter = "a" | "b" | ... | "z" | "A" | "B" | ... | "Z" ;
digit = "0" | "1" | ... | "9" ;
underscore = "_" ;
```

expression
```
expression = ["+" | "-"] term {("+" | "-") term} ;
term = factor {("*" | "/") factor} ;
factor = number | "(" expression ")" | identifier | function-call ;
number = ["+" | "-"] {digit}- ["." {digit}-] ;
function-call = identifier "(" [identifier {"," identifier }] ")" ;
```

function
- A function definition must begin with `func`
- A function may or may not has parameters
- A function may or may not has return type
- A function body is composed of block of statements surrounded by curly braces, a statement block may or may not has statements
```
function = "func" identifier "(" parameter-list ")" ["=>", type] block ;
parameter-list = [parameter {"," parameter}] ;
parameter = identifier ":" type ;
block = "{" {statement} "}" ;
type = "char" | "int" | "str" | "bool" ;
```

statement
- There are 2 kinds of statements in kat
  - Regular statements
    - Declaration statements
      - Use `let` to declare variables
      - The declared variable may or may not be initialized
    - Expression statements
      - The statement is an expression
  - Control statements
    - `if-elif-else` statment
      - `elif` statement can be omitted or repeated
      - `else` statement can be omitted or present just once
    - `while` statement
    - `break` statement
    - `continue` statement
    - `return` statement
```
statement = [regular-statement | control-statement]

regular-statement =
[ "let" identifier ":" type ["=" expression] ";"  /* declaration statement */
| [identifier "="] expression ";"                 /* expression statement */
] ;

control-statment =
[ "if" condition block      /* if-elif-else statement */
  {"elif" block}
  ["else" block]
| "while" condition block   /* while statement */
| "break" ";"               /* break statment */
| "continue" ";"            /* continue statement */
| "return" expression ";"   /* return statment */
] ;

condition = "(" expression {relational-operator expression}- ")" ;
relational-operator = "&&" | "||" | ">" | "<" | ">=" | "<=" | "==" | "!=" ;
```

program
- Currently, functions are first-class citizens in kat, a kat program is composed of functions.
```
program = {function} ;
```
