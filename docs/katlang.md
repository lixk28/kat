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
expression = primary {operator primary} ;
operator = "+" | "-" | "*" | "/" | "&&" | "||" | ">" | "<" | ">=" | "<=" | "==" | "!=" ;
primary = ["+" | "-"] ("(" expression ")" | number | identifier | function-call) ;
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
- There are 3 kinds of statements in kat
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

declaration-statement = "let" identifier ":" type ["=" expression] ";"

expression statement = [identifier "="] expression ";"

control-statment =
[ "if" "(" expression ")" block      (* if-elif-else statement *)
  {"elif" block}
  ["else" block]
| "while" "(" expression ")" block   (* while statement *)
| "break" ";"                        (* break statement *)
| "continue" ";"                     (* continue statement *)
| "return" expression ";"            (* return statement *)
] ;
```

program
- Currently, functions are first-class citizens in kat, a kat program is composed of functions.
```
program = {function} ;
```
