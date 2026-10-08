# Ejercicio 1: Gestión básica de procesos

Dos programas en C: `malla.c` (apartado a) y `ejec.c` (apartado b).

## Ideas básicas (leer primero)

- **Proceso**: un programa en ejecución. Cada uno tiene un número único, el **PID**.
- **`fork()`**: un proceso se "clona". El original es el **padre** y el clon es el **hijo**. `fork()` devuelve `0` en el hijo y el PID del hijo en el padre; así cada uno sabe quién es.
- **`wait`/`waitpid`**: el padre espera a que su hijo muera. Así el padre nunca muere antes que sus hijos.
- **Señal**: un aviso que un proceso manda a otro (`kill`). El que la recibe ejecuta una función suya llamada *manejador* (`handler`).
- **`alarm(n)`**: pide al sistema que mande una señal `SIGALRM` a este proceso dentro de `n` segundos. Sustituye a `sleep`.
- **`exec`**: el proceso deja de ser lo que era y pasa a ser otro programa (por ejemplo `pstree`).

## Apartado a: `malla.c`

Uso: `./malla <filas> <columnas>`

El árbol son **columnas** (hijos directos de `malla`) y cada columna es una **cadena** de procesos, uno colgando del otro. No hay ningún `for`: todo se hace con funciones que se llaman a sí mismas (recursión).

| Función | Qué hace |
|---|---|
| `safe_fork` | Hace `fork` y avisa si falla. |
| `wait_for_child` | Espera a un hijo concreto. |
| `parse_args` | Lee y valida filas y columnas. |
| `run_row(n)` | Crea un hijo y le dice que cree `n-1` por debajo. Si no quedan filas, se queda esperando 20 s. |
| `create_columns(c, f)` | Crea una columna, se llama a sí misma para crear las `c-1` restantes y después espera a su hijo. |

Los procesos de la última fila se quedan 20 segundos vivos para dar tiempo a ver el árbol.

Comprobar:

```
gcc -Wall -o malla malla.c
./malla 3 4 &
pstree -c
```

(O `pstree -c <PID>`, con el PID que muestra el shell, para ver solo tu árbol.)

## Apartado b: `ejec.c`

Uso: `./ejec <segundos>`

Árbol: `ejec → A → B → {X, Y, Z}`.

Qué pasa:

1. Se crea el árbol y cada proceso se presenta.
2. Z programa una alarma con `alarm()` (sin `sleep`).
3. Al sonar, Z manda `SIGUSR1` a A.
4. A ejecuta `pstree` y después manda `SIGUSR2` a B.
5. B manda `SIGUSR2` a Z, Y y X (en ese orden, esperando a cada uno) y muere.
6. Mueren A y `ejec`, siempre después de sus hijos.

| Función | Qué hace |
|---|---|
| `create_A_process`, `create_B_process` | Hacen el `fork` de A y de B; el padre espera. |
| `run_process_A`, `run_process_B` | Lo que hace cada uno al nacer: presentarse y registrar sus manejadores. |
| `create_leaf_process` / `run_leaf_process` | Lo mismo para X, Y y Z (la letra decide quién es; solo Z programa la alarma). |
| `handler_Z_alarm` | Z avisa a A. |
| `handler_A_exec_task` | A ejecuta `pstree` y avisa a B. |
| `handler_B_destroy_and_propagate` | B mata a sus hijos en orden y muere. |
| `handler_leaf_destroy` | X, Y o Z anotan que deben morir. |

Notas:

- Como `pause()` y `sleep()` no se usan, X, Y y Z esperan con un bucle sobre una variable `volatile sig_atomic_t`.
- Los PIDs de los antepasados se guardan en variables globales **antes** de cada `fork`, y el hijo hereda una copia.

Comprobar:

```
gcc -Wall -o ejec ejec.c
./ejec 15
```
