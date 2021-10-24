
.PHONY: all clean run pack

run:
	@cd src && make -j && cd - && ./mytftpclient

all:
	@cd src && make -j

clean:
	@rm -rf mytftpclient build xmihol00.tar

pack:
	tar -czvf xmihol00.tar src tests Makefile manual.pdf README
