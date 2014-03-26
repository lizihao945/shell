#!/bin/bash
yacc -d shell.y
lex shell.l
gcc y.tab.c shell.c lex.yy.c -o shell -l readline
