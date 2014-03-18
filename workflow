#!/bin/bash
lex shell.l
yacc -d shell.y
gcc y.tab.c lex.yy.c shell.c -o shell -l readline
