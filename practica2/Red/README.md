# Comunicaciones en red

Este ejercicio implementa una aplicación cliente-servidor TCP. El servidor
escucha en el puerto `9999`; cuando se conecta un cliente, crea un proceso hijo
para enviarle el contenido de `Google.html`. El cliente recibe esos bytes y los
escribe en la salida estándar: muestra el código HTML, no renderiza la página.

## Archivos

| Archivo | Función |
|---|---|
| `Servidor.c` | Crea el socket de escucha, acepta conexiones y delega cada transferencia a un hijo creado con `fork()`. |
| `Cliente.c` | Se conecta a la dirección IP indicada, lee los datos hasta el cierre de la conexión y los muestra por pantalla. |
| `Google.html` | Archivo que el servidor abre y transmite. Debe estar en el directorio desde el que se inicia el servidor. |

## Compilar y ejecutar

Desde esta carpeta, compila cada programa:

```sh
gcc -Wall -o Servidor Servidor.c
gcc -Wall -o Cliente Cliente.c
```

Inicia el servidor en una terminal:

```sh
./Servidor
```

En otra terminal, inicia el cliente con la IP del servidor:

```sh
./Cliente 127.0.0.1
```

`127.0.0.1` sirve para probar ambos programas en la misma máquina. Si se
ejecutan en máquinas distintas, usa la IP del equipo donde escucha el servidor.

## Flujo del servidor y el cliente

1. El servidor crea un socket TCP, lo asocia al puerto `9999` con `bind()` y
   queda disponible para conexiones con `listen()`.
2. `accept()` espera una conexión. Al recibirla, `fork()` crea un hijo para
   atenderla, mientras el proceso padre vuelve a esperar otros clientes.
3. El hijo lee `Google.html` y envía el contenido al socket del cliente.
4. El cliente establece la conexión con `connect()` y lee hasta que el servidor
   cierre la conexión.

`SO_REUSEADDR` permite reutilizar el puerto tras reiniciar el servidor, y la
disposición de `SIGCHLD` evita acumular procesos hijos terminados.
