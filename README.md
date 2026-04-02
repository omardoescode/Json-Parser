# JSON Parser

Because I was bored.

A tiny JSON parser written in C. It works… mostly.

# How to use
```bash
make main 
./main test.json
# or you can use the pipe operator
cat test.json | ./main
```


It prints your JSON back at you. Enjoy.

Limitations:
- Only handles integers, strings, booleans, null, arrays, and objects. (no floating point numberes, or scientific notations)
- It uses ascii characters, no support for wide characters

