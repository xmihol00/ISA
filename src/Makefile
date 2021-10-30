#====================================================================================================================
# Soubor:      Makefile
# Projekt:     VUT, FIT, ISA, TFTPv2 klient
# Datum:       30. 10. 2021
# Autor:       David Mihola
# Kontakt:     xmihol00@stud.fit.vutbr.cz
# Popis:       Soubor obsahující instrukce pro překlad souborů v adresáři src/.
#====================================================================================================================

CC = g++
CFLAGS = -std=c++17 -Wall -Wextra -MMD
BUILD_DIR = ../build/
EXE = ../mytftpclient
SRC = $(wildcard *.cpp)
OBJ = ${SRC:.cpp=.o}
DEPS = ${OBJ:.o=.d}
DIR_OBJ = $(addprefix $(BUILD_DIR), $(OBJ))
DIR_DEPS = $(addprefix $(BUILD_DIR), $(DEPS))

.PHONY: all clean

all: $(EXE)

-include $(DIR_DEPS)

$(EXE): $(DIR_OBJ)
	$(CC) $(DIR_OBJ) $(CFLAGS) -o $(EXE)

$(BUILD_DIR)%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CC) $< $(CFLAGS) -c -o $@
