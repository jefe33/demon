OBJ = demon.o synchro.o

program: $(OBJ)
	gcc $(OBJ) -o program
$(OBJ): synchro.h
.PHONY: clean
clean:
	rm $(OBJ) program
