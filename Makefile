
.PHONY: all clean

all:
	@cd src && make -j

clean:
	@rm -rf mytftpclient build