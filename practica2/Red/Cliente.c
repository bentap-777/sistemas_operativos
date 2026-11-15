/*
 * Cliente.c - Practica 2 (Comunicaciones en red)
 * Se conecta al servidor (puerto 9999), recibe el contenido de Google.html
 * y lo muestra por pantalla.
 *
 * Compilar: gcc Cliente.c -o Cliente
 * Ejecutar: ./Cliente IP_Servidor
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PUERTO 9999
#define BUFSIZE 1024

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in dir_servidor;
    char buffer[BUFSIZE];
    ssize_t recibidos;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s IP_Servidor\n", argv[0]);
        exit(1);
    }

    /* 1. socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    /* Direccion del servidor */
    memset(&dir_servidor, 0, sizeof(dir_servidor));
    dir_servidor.sin_family = AF_INET;
    dir_servidor.sin_addr.s_addr = inet_addr(argv[1]);
    dir_servidor.sin_port = htons(PUERTO);

    /* 2. connect */
    if (connect(sock, (struct sockaddr *)&dir_servidor,
                sizeof(dir_servidor)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    /* 3. recibir y mostrar por pantalla */
    while ((recibidos = read(sock, buffer, BUFSIZE)) > 0) {
        write(STDOUT_FILENO, buffer, recibidos);
    }

    if (recibidos < 0)
        perror("read");

    close(sock);
    return 0;
}
