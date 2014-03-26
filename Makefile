shell:
	yacc -d shell.y
	lex shell.l
	gcc y.tab.c shell.c lex.yy.c -o shell -l readline
clean:
	rm shell y.tab.c lex.yy.c 

