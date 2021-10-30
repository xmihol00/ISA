
#====================================================================================================================
# Soubor:      Makefile
# Projekt:     VUT, FIT, ISA, TFTPv2 klient
# Datum:       30. 10. 2021
# Autor:       David Mihola
# Kontakt:     xmihol00@stud.fit.vutbr.cz
# Popis:       Soubor obsahující instrukce pro překlad souborů v adresáři src/.
#====================================================================================================================

.PHONY: all clean run pack

all:
	@cd src && make -j 8

run:
	@cd src && make -j 8 && cd - && ./mytftpclient

clean:
	@rm -rf mytftpclient build xmihol00.tar

pack:
	tar -cvf xmihol00.tar src tests Makefile manual.pdf README
