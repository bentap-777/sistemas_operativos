# Ejercicio 2: Tuberías (`hacha.c`)

Divide un archivo en trozos con el mismo nombre y extensión `.h00`, `.h01`, ...

Uso: `./hacha <archivo> <tamaño>` (el tamaño es en bytes)

## Ideas básicas

- **Tubería (`pipe`)**: un tubo en memoria con dos extremos. `fds[1]` es donde se **escribe** y `fds[0]` es donde se **lee**. Lo que entra primero sale primero.
- Al hacer `fork`, el hijo hereda la tubería. Después cada uno cierra el extremo que no usa: el padre cierra el de lectura y el hijo el de escritura.
- Cuando el padre cierra su extremo de escritura, el hijo recibe "fin de archivo" (`read` devuelve 0) y sabe que ya no hay más datos.
- Todo el acceso a archivos es con llamadas al sistema: `open`, `read`, `write`, `close`, `lseek`. No se usa `printf`, `scanf` ni nada de `stdio`.

## Cómo funciona

1. El padre abre el archivo y mide su tamaño con `lseek`. Calcula cuántos trozos salen: `ceil(tamaño total / tamaño de trozo)`.
2. Por cada trozo crea una tubería y un hijo. Los hijos se lanzan **secuencialmente**: uno detrás de otro, para no llenar la memoria de procesos.
3. El padre lee del archivo original hasta `tamaño` bytes y los mete en la tubería.
4. El hijo lee de la tubería y los escribe en el archivo de destino, que crea él (`nombre.h00`, `nombre.h01`, ...).
5. El padre cierra la tubería, espera al hijo (`waitpid`) y pasa al siguiente trozo.

El último trozo es más pequeño si el tamaño no es múltiplo exacto. Con extensiones de dos cifras, el máximo es 100 trozos (`h00` a `h99`).

## Funciones

| Función | Qué hace |
|---|---|
| `fail` | Escribe un error con `write` y termina. |
| `safe_fork` | `fork` con comprobación de error. |
| `write_all` | Insiste con `write` hasta escribir todo (puede escribir menos de lo pedido). |
| `copy_bytes` | Copia de un descriptor a otro, con límite de bytes o hasta el final. La usan el padre (archivo → tubería) y el hijo (tubería → archivo). |
| `parse_args` | Valida los argumentos. |
| `open_source` | Abre el archivo a dividir. |
| `file_size`, `count_chunks` | Tamaño total y número de trozos. |
| `build_chunk_name` | Construye `nombre.h00`, `nombre.h01`... |
| `run_child` | Lo que hace el hijo: crear el archivo y copiar desde la tubería. |
| `create_chunk_process` | Lo que hace el padre por trozo: tubería, `fork`, enviar datos, esperar. |

## Comprobar

```
gcc -Wall -o hacha hacha.c
./hacha at_madrid.mp3 50000
ls
cat at_madrid.mp3.h* | cmp - at_madrid.mp3 && echo IDENTICO
```

`cmp` no debe imprimir ninguna diferencia: los trozos unidos son el original.
