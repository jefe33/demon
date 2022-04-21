OBJ = demon.o synchro.o

program: $(OBJ)
	gcc $(OBJ) -o program
$(OBJ): synchro.h

clean:
	rm $(OBJ) program
