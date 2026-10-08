# Práctica 1: Sistemas Operativos (2026/2027)

Gestión de procesos y archivos. Comunicación entre procesos: tuberías y memoria compartida. Todo el código está en C y usa llamadas al sistema de Unix/Linux.

## Ejercicios

| Ejercicio | Tema | Programas | Explicación |
|---|---|---|---|
| 1 | Gestión básica de procesos | `malla.c`, `ejec.c` | [ej1/README.md](ej1/README.md) |
| 2 | Tuberías | `hacha.c` | [ej2/README.md](ej2/README.md) |
| 3 | Memoria compartida | `hijos.c` | [ej3/README.md](ej3/README.md) |

Cada carpeta tiene su propio README, que explica **antes del código** qué hace el programa y cómo está organizado.

## Estructura

```
.
├── README.md
├── ej1/
│   ├── README.md
│   ├── malla.c
│   └── ejec.c
├── ej2/
│   ├── README.md
│   └── hacha.c
└── ej3/
    ├── README.md
    └── hijos.c
```

## Requisitos

- Linux con `gcc`.
- `pstree`, que viene en el paquete `psmisc` (`sudo apt install psmisc`). Lo usan los ejercicios 1a y 1b.

## Compilar y ejecutar

```
gcc -Wall -o malla ej1/malla.c
gcc -Wall -o ejec  ej1/ejec.c
gcc -Wall -o hacha ej2/hacha.c
gcc -Wall -o hijos ej3/hijos.c
```

| Programa | Ejemplo |
|---|---|
| `malla` | `./malla 3 4 &` y después `pstree -c` |
| `ejec` | `./ejec 15` |
| `hacha` | `./hacha at_madrid.mp3 50000` |
| `hijos` | `./hijos 3 4` |

Los detalles de cada prueba están en el README de su ejercicio.

## Criterios de diseño

- **Modularización**
- **Pocos procesos a la vez**: no se crean procesos de más; los padres esperan a sus hijos y ninguno muere antes que ellos.

## Aclaraciones
- He usado el Copilot para subir y guardar los archivos que he ido haciendo por mi cuenta. Ya que no he usado nunca el Github, y no tenía ni idea de nada en general.