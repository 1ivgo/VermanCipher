all: main test
main: main.cpp
	g++ main.cpp -o main.out -pthread
test: crypt encrypt check
crypt:
	./main.out source.txt crypt.txt
encrypt:
	./main.out crypt.txt encrypt.txt
check:
	diff source.txt encrypt.txt
clean:
	rm crypt.txt 
	rm encrypt.txt
	rm main.out
