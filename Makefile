CFLAGS= -Wall -Wextra -std=c89 -Werror -fsanitize=address,undefined -g

build/out: vm.c
	@mkdir -p build
	cc vm.c $(CFLAGS) -o build/out

run: build/out 
	./build/out meta2.asm meta2.meta
	
debug: build/out 
	lldb ./build/out meta2.asm meta2.meta
