#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <string>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_THREAD 11

using namespace std;

pthread_barrier_t barrier;

struct CryptParams
{
    unsigned char* msg;
    size_t size;
    unsigned char* key;
    unsigned char* outText;
    size_t downIndex;
    size_t topIndex;
};

struct KeygenParams
{
    size_t a;
    size_t m;
    size_t c;
    unsigned char seed;
    size_t keyLenght;
};

void* keygen(void*);
void* crypt(void*);
void errorExit(const char*);
unsigned char* readText(KeygenParams*, char*);

int main(int argc, char *argv[])
{
    int status = 0;
    pthread_t keyGenThread;
    pthread_t thread[10];
    int inFile, outFile = 0;
    int size = 0;

    KeygenParams keygenParams;
    keygenParams.a = 5;
    keygenParams.m = 127;
    keygenParams.c = 3;
    keygenParams.seed = 0;

    inFile = open(argv[1], O_RDONLY, 0666);
    if (inFile == -1)
        errorExit("Open() error");

    size = lseek(inFile, 0, SEEK_END);
    if(size == -1)
        errorExit("lseek() error");

    unsigned char* msg = new unsigned char[size];
    unsigned char* key = new unsigned char[size];
    unsigned char* outText = new unsigned char[size];

    if(lseek(inFile, 0, SEEK_SET) == -1)
		errorExit("lseek() error");
    size = read(inFile, msg, size);
	if(size == -1)
		errorExit("read() error");

    if(pthread_create(&keyGenThread, NULL, keygen, &keygenParams) != 0)
        errorExit("pthread_create() error");
    if(pthread_join(keyGenThread, (void**)&key) != 0)
        errorExit("pthread_join() error");

    status = pthread_barrier_init(&barrier, NULL, NUM_THREAD);
    if(status != 0)
        errorExit("pthread_barrier_init() error");
    for(int i=0; i<10; i++)
    {
        CryptParams* cryptParams = new CryptParams;
        cryptParams->key = key;
        cryptParams->msg = msg;
        cryptParams->size = size;
        cryptParams->outText = outText;
        if(i == 0)
        {
            cryptParams->downIndex = 0;
        }
        else
        {
            cryptParams->downIndex = size / 10 * i;
        }
        if(i == 9)
        {
            cryptParams->topIndex = size;
        }
        else
        {
            cryptParams->topIndex = size / 10 * (i + 1);
        }
        pthread_create(&thread[i], NULL, crypt, cryptParams);
    }
    status = pthread_barrier_wait(&barrier);
    if(status == PTHREAD_BARRIER_SERIAL_THREAD)
        pthread_barrier_destroy(&barrier);
    else if(status != 0)
        errorExit("pthread_barrier_destroy() error");

	outFile = open(argv[2], O_WRONLY | O_CREAT, 0666);
    if (outFile == -1)
        errorExit("Open() error");
	write(outFile, outText, size);

    delete[] key;
    delete[] msg;
    delete[] outText;

    close(inFile);
    close(outFile);

    return EXIT_SUCCESS;
}

void* keygen(void * keygenParams)
{
    KeygenParams *kp = (KeygenParams *)keygenParams;
    size_t a = kp->a;
    size_t m = kp->m;
    size_t c = kp->c;
    size_t keyLenght = kp->keyLenght;

    unsigned char* key = new unsigned char[keyLenght];    
    unsigned char Xprev = kp->seed;

    for (size_t i = 0; i < keyLenght; i++)
    {
        key[i] = (a * Xprev + c) % m;
        Xprev = key[i];
    }

    return key;
}

void* crypt(void * cryptParams)
{
    int status = 0;
    
    CryptParams* cryptPar = (CryptParams*)cryptParams;
    unsigned char* msg = cryptPar->msg;
    unsigned char* key = cryptPar->key;
    unsigned char* outText = cryptPar->outText;
    size_t topIndex = cryptPar->topIndex;
    size_t downIndex = cryptPar->downIndex;

    while (downIndex < topIndex)
    {
        outText[downIndex] = key[downIndex] ^ msg[downIndex];
        downIndex++;
    }

    cryptPar->outText = outText;

    delete(cryptPar);

    status = pthread_barrier_wait(&barrier);
    if(status == PTHREAD_BARRIER_SERIAL_THREAD)
        pthread_barrier_destroy(&barrier);
    else if(status != 0)
        errorExit("pthread_barrier_destroy() error");
}

void errorExit(const char* error)
{
    perror(error);
    exit(0);
}