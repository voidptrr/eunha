# eunha

Small C23 base library.

## Nix

Enter the development shell and build:

```sh
nix develop
make
```

Run the tests:

```sh
make test
```

Build the Nix package:

```sh
nix build
```

The package installs headers under `include/eunha` and the static library as
`lib/libeunha.a`.

## Without Nix

Clang, GNU Make, and binutils are required.

```sh
make
make test
```

Install to `/usr/local`:

```sh
sudo make install
```

Use a different prefix when needed:

```sh
make install PREFIX=/path/to/prefix
```

## Use

Include headers with the `eunha` prefix:

```c
#include <eunha/arena.h>
#include <eunha/string.h>
```

Link against `libeunha.a`:

```sh
clang -std=c23 main.c -leunha -o app
```
