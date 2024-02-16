Cfile=httpServer.c  function.c

httpServer:$(Cfile)
	gcc *.c -o httpServer -l pthread