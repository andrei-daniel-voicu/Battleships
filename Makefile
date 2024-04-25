build:
	gcc battleship.c -lncurses -o battleship

run: build
	./battleship config1 config2

clean:
	rm battleship

