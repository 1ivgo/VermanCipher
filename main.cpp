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
    size_t indexOfThread;
    size_t indexOfMas;
};

struct KeygenParams
{
    size_t a;
    size_t m;
    size_t c;
    unsigned char seed;
    unsigned char* key;
    size_t keyLenght;
};

void* keygen(void*);
void* crypt(void*);
void errorExit(const char*);
void readText(CryptParams*, char*);

int main(int argc, char *argv[]) {

    int status = 0;
    pthread_t keyGenThread;
    pthread_t thread[10];

    KeygenParams keygenParams;
    keygenParams.a = 5;
    keygenParams.m = 127;
    keygenParams.c = 3;
    keygenParams.seed = 0;

	CryptParams cryptParams;
    cryptParams.indexOfThread = 0;

    readText(&cryptParams, argv[1]);
    keygenParams.key = new unsigned char[cryptParams.size];
	keygenParams.keyLenght = cryptParams.size;
    
    if(pthread_create(&keyGenThread, NULL, keygen, &keygenParams) != 0)
        errorExit("pthread_create() error");
    if(pthread_join(keyGenThread, NULL) != 0)
        errorExit("pthread_join() error");

    cryptParams.key = keygenParams.key;

    status = pthread_barrier_init(&barrier, NULL, NUM_THREAD);
    if(status != 0)
        errorExit("pthread_barrier_init() error");
    for(int i=0; i<10; i++)
    {
        pthread_create(&thread[i], NULL, crypt, &cryptParams);
    }
    status = pthread_barrier_wait(&barrier);
    if(status == PTHREAD_BARRIER_SERIAL_THREAD)
        pthread_barrier_destroy(&barrier);
    else if(status != 0)
        errorExit("pthread_barrier_destroy() error");


	int fd = open(argv[2], O_WRONLY | O_CREAT, 0666);
    if (fd == -1)
        errorExit("Open() error");
	size_t test = write(fd, cryptParams.outText, cryptParams.size);

    delete (keygenParams.key);
    delete (cryptParams.msg);
    delete (cryptParams.outText);

    return EXIT_SUCCESS;
}

void* keygen(void * keygenParams)
{
    KeygenParams *kp = (KeygenParams *)keygenParams;
    size_t a = kp->a;
    size_t m = kp->m;
    size_t c = kp->c;
    unsigned char* key = kp->key;
    size_t keyLenght = kp->keyLenght;

    unsigned char Xprev = kp->seed;

    for (size_t i = 0; i < keyLenght; i++)
    {
        key[i] = (a * Xprev + c) % m;
        Xprev = key[i];
    }

    pthread_exit(0);
}

void* crypt(void * cryptParams)
{
    int status = 0;

    CryptParams* cryptPar = (CryptParams*)cryptParams;
    unsigned char* msg = cryptPar->msg;
    unsigned char* key = cryptPar->key;
    unsigned char* outText = cryptPar->outText;
    size_t size = cryptPar->size;
    size_t indexOfThread = cryptPar->indexOfThread;
    size_t indexOfMas = cryptPar->indexOfMas;

    indexOfThread++;
    cryptPar->indexOfThread = indexOfThread;

    if(indexOfThread == 10)
    {
        indexOfMas = size;
    }
    else
    {
        indexOfMas = size/10 * indexOfThread;
    }
    for (size_t i = 0; i < indexOfMas; i++)
    {
        outText[i] = key[i] ^ msg[i];
    }

    cryptPar->outText = outText;
    cryptPar->indexOfMas = indexOfMas;

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

void readText(CryptParams *cryptParams, char* fileName)
{
    int fd = 0;
    int size = 0;

    fd = open(fileName, O_RDONLY, 0666);
    if (fd == -1)
        errorExit("Open() error");

    size = lseek(fd, 0, SEEK_END);
    if(size == -1)
        errorExit("lseek() error");

    cryptParams->size = size;
    cryptParams->msg = new unsigned char[size];
    cryptParams->key = new unsigned char[size];
    cryptParams->outText = new unsigned char[size];

    if(lseek(fd, 0, SEEK_SET) == -1)
		errorExit("lseek() error");
    size = read(fd, cryptParams->msg, size);
	if(size == -1)
		errorExit("read() error");
}
