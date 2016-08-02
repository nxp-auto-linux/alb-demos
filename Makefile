OBJ_MULTICORE := multicore.o functions.o
subsystem:
	cd multicore && $(MAKE)

clean:
	cd multicore && rm -f $(OBJ_MULTICORE) multicore 
