gcc -c -g at-funcs.c
ar cru at-funcs.a at-funcs.o
emxomf -o at-funcs.lib at-funcs.a
copy at-funcs.a \extras\lib
copy at-funcs.lib \extras\lib