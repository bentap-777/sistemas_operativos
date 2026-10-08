# Ejercicio 3: Memoria compartida (`hijos.c`)

Crea un árbol de procesos formado por una **cadena vertical** de `x` procesos y, colgando del último, `y` **hijos finales**. Cada proceso muestra un mensaje con los PIDs que le corresponden.

Uso: `./hijos <x> <y>`

```
hijos                  <- superpadre
  |
  p1                   \
  |                     |  x procesos en cadena
 ...                    |
  |                     |
  px                   /
 / | \ ...
y hijos finales
```

Salida:

- Cada hijo final: `Soy el subhijo <pid>, mi padres son: pid1, pid2, ..., pidx`
- El superpadre: `Soy el superpadre (<pid>) : mis hijos finales son: pidx1, pidx2, ..., pidxy`

## Ideas básicas

- Tras un `fork`, padre e hijo tienen **copias** de las variables: si el hijo cambia una, el padre no se entera. Por eso el superpadre no puede conocer los PIDs de los hijos finales (son nietos suyos, muy por debajo).
- **Memoria compartida**: un trozo de memoria que ven todos los procesos. Lo que escribe uno, lo leen los demás.
- Pasos (llamadas de la práctica):
  1. `shmget`: crea el segmento y devuelve su identificador.
  2. `shmat`: lo vincula al proceso mediante un puntero.
  3. Se usa como un array normal de enteros.
  4. `shmdt`: lo desvincula.
  5. `shmctl(..., IPC_RMID, ...)`: lo elimina del sistema.
- El segmento se crea **antes** de los `fork`, así los hijos lo heredan ya vinculado.

## Cómo funciona

La memoria compartida es un array de `x + y` enteros:

```
[ pid1 | pid2 | ... | pidx | pidx1 | pidx2 | ... | pidxy ]
  \_____ cadena _______/    \______ hijos finales _______/
```

1. El superpadre crea la memoria compartida y el primer proceso de la cadena.
2. Cada proceso de la cadena **anota su PID** en su posición y crea al siguiente. El último crea los `y` hijos finales.
3. Cada hijo final anota su PID en la parte baja del array y muestra su mensaje, leyendo de la cadena los PIDs de sus padres.
4. Cada padre espera a su hijo con `waitpid`, así que cuando el superpadre despierta ya están todos anotados. Entonces muestra su mensaje y elimina la memoria compartida.

La cadena y los hijos finales se crean con recursión, igual que en `malla.c`.

## Funciones

| Función | Qué hace |
|---|---|
| `safe_fork`, `wait_for_child` | `fork` con comprobación de error y espera a un hijo concreto. |
| `parse_args` | Valida `x` e `y`. |
| `print_pids` | Escribe una lista de PIDs separados por comas. |
| `create_shared_memory` | `shmget` + `shmat`. |
| `remove_shared_memory` | `shmdt` + `shmctl`. Solo la llama el superpadre. |
| `chain_pids`, `leaf_pids` | Devuelven la parte del array de la cadena y la de los hijos finales. |
| `create_chain_process`, `run_chain_process` | Un nivel de la cadena: crear el proceso y lo que hace al nacer. |
| `create_leaves`, `run_leaf_process` | Los hijos finales: crearlos y lo que hace cada uno. |

## Comprobar

```
gcc -Wall -o hijos hijos.c
./hijos 3 4
ipcs -m
```

- Debe haber una línea `Soy el subhijo ...` por cada hijo final (4 en el ejemplo), todas con los mismos 3 padres, y al final la línea del superpadre con los 4 PIDs de los hijos finales.
- `ipcs -m` no debe mostrar ningún segmento: significa que la memoria compartida se ha eliminado bien.
- El orden de las líneas de los hijos finales puede variar, porque se ejecutan a la vez.
