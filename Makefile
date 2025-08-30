CFLAGS=-Wall -Wextra

standby: standby.c
	cc $(CFLAGS) -o standby standby.c -I../raylib-5.5/include -L../raylib-5.5/lib -lm -l:libraylib.a

