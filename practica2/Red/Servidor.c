/*
 * Servidor.c - Practica 2 (Comunicaciones en red)
 * Espera conexiones en el puerto 9999. Por cada cliente crea un hijo (fork)
 * que envia el contenido del archivo Google.html y termina.
 *
 * Compilar: gcc Servidor.c -o Servidor
 * Ejecutar: ./Servidor
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PUERTO 9999
#define COLA   5
#define BUFSIZE 1024
#define ARCHIVO "Google.html"

/* Tarea del hijo: transferir el archivo al cliente */
void atender_cliente(int sock_cliente)
{
    char buffer[BUFSIZE];
    ssize_t leidos;
    int fd = open(ARCHIVO, O_RDONLY);

    if (fd < 0) {
        perror("Hijo: error al abrir Google.html");
        return;
    }

    while ((leidos = read(fd, buffer, BUFSIZE)) > 0) {
        if (write(sock_cliente, buffer, leidos) < 0) {
            perror("Hijo: error al enviar");
            break;
        }
    }

    close(fd);
}

int main(void)
{
    int sock_escucha, sock_cliente;
    struct sockaddr_in dir_servidor, dir_cliente;
    socklen_t long_cliente = sizeof(dir_cliente);
    pid_t pid;

    /* Evita procesos zombie: el kernel recoge a los hijos al terminar */
    signal(SIGCHLD, SIG_IGN);

    /* 1. socket */
    sock_escucha = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_escucha < 0) {
        perror("socket");
        exit(1);
    }

    /* Permite reutilizar el puerto al reiniciar el servidor */
    int opt = 1;
    setsockopt(sock_escucha, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. bind */
    memset(&dir_servidor, 0, sizeof(dir_servidor));
    dir_servidor.sin_family = AF_INET;
    dir_servidor.sin_addr.s_addr = htonl(INADDR_ANY);
    dir_servidor.sin_port = htons(PUERTO);

    if (bind(sock_escucha, (struct sockaddr *)&dir_servidor,
             sizeof(dir_servidor)) < 0) {
        perror("bind");
        exit(1);
    }

    /* 3. listen */
    if (listen(sock_escucha, COLA) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Servidor escuchando en el puerto %d...\n", PUERTO);

    /* 4. accept (bucle infinito: siempre preparado para aceptar) */
    while (1) {
        sock_cliente = accept(sock_escucha,
                              (struct sockaddr *)&dir_cliente, &long_cliente);
        if (sock_cliente < 0) {
            perror("accept");
            continue;
        }

        printf("Conexion recibida de %s\n", inet_ntoa(dir_cliente.sin_addr));

        pid = fork();
        if (pid < 0) {
            perror("fork");
            close(sock_cliente);
        } else if (pid == 0) {
            /* HIJO: cierra el socket de escucha y atiende al cliente */
            close(sock_escucha);
            atender_cliente(sock_cliente);
            close(sock_cliente);
            exit(0);
        } else {
            /* PADRE: cierra su copia del socket del cliente y sigue */
            close(sock_cliente);
        }
    }

    close(sock_escucha);
    return 0;
}
