CC = gcc

CFLAGS = -Wall -Wextra -g -std=c99

LDFLAGS = 

RM = rm -rf

BIN_DIR = bin
OBJ_DIR = obj
RES_DIR = result

TARGET_SCANNER = $(BIN_DIR)/scanner
TARGET_PARSER = $(BIN_DIR)/parser

OBJS_SCANNER_SRC = mainScanner.o Scanner.o
OBJS_PARSER_SRC = mainParser.o Parser.o Scanner.o

OBJS_SCANNER = $(addprefix $(OBJ_DIR)/, $(OBJS_SCANNER_SRC))
OBJS_PARSER = $(addprefix $(OBJ_DIR)/, $(OBJS_PARSER_SRC))

all: $(TARGET_SCANNER) $(TARGET_PARSER)

$(TARGET_PARSER): $(OBJS_PARSER) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "--- Ejecutable '$(TARGET_PARSER)' creado. ---"

$(TARGET_SCANNER): $(OBJS_SCANNER) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "--- Ejecutable '$(TARGET_SCANNER)' creado. ---"

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

$(OBJ_DIR)/mainParser.o: mainParser.c parser.h Scanner.h
$(OBJ_DIR)/Parser.o: Parser.c parser.h Scanner.h
$(OBJ_DIR)/mainScanner.o: mainScanner.c Scanner.h
$(OBJ_DIR)/Scanner.o: Scanner.c Scanner.h

parser: 
	./bin/parser

scanner: 
	./bin/scanner

.PHONY: clean
clean:
	$(RM) $(BIN_DIR) $(OBJ_DIR) $(RES_DIR)
	@echo "--- Directorios 'bin' y 'obj' eliminados. ---"