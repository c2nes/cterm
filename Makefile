
export BIN_NAME=cterm

$(BIN_NAME):
	$(MAKE) -C src

clean:
	rm src/*.o $(BIN_NAME)

.PHONY: clean $(BIN_NAME)
