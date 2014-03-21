#!/bin/bash
lex shell.l
yacc -d shell.y
gcc y.tab.c shell.c lex.yy.c -o shell -l readline
