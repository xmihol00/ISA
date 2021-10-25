
.PHONY: all clean run pack

all:
	@cd src && make -j

run:
	@cd src && make -j && cd - && ./mytftpclient

clean:
	@rm -rf mytftpclient build xmihol00.tar

pack:
	tar -czvf xmihol00.tar src tests Makefile manual.pdf README
