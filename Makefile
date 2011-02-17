all:
	gcc -Wall -pedantic v4l_lua.c core.c -shared -o v4l.so -I /usr/include/lua5.1/ -llua5.1 -lv4lconvert 

clean:
	rm *.so
