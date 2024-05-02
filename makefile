OBJ = server.c server.h myChat.c login.c login.h
main: $(OBJ)
	gcc $(OBJ) -o myChat -Wall -lncurses;

server:
	@./myChat --start

login1:
	@./myChat --login User1

login2:
	@./myChat --login User2

.PHONY: clean

clean:
	@rm -r myChat
	@rm -r pipe*