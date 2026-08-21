# Project Instructions

## Project Structure

- `main.c` contains process startup.
- `config.c` and `config.h` contain defaults and environment configuration.
- `http/` contains networking and HTTP-facing code.
- `datastruct/` contains internal generic data structures.
- `tests/` contains in-repo C unit tests and its own Makefile.
- `nix/` contains flake package, shell, hook, and workflow support.
- `.github/workflows/` contains generated CI workflows.
- `Makefile` is the primary local build entry point.

## Style

- Use the full MIT license header at the top of source-controlled files.
- Prefer explicit C definitions such as `struct vector` over typedef aliases.
- Use `typedef` for function pointer types when it improves API readability.
- Do not use `typedef` to hide struct, enum, or union definitions.
- Add comments to functions and types when they clarify non-obvious behavior or public API intent.
- Avoid comments that only restate the implementation.
- Avoid public names that collide with standard C or POSIX APIs. For example, use `server_listen` instead of `listen`.
- Use `uint8_t` for byte-wise pointer arithmetic and raw byte APIs. Use `void*` for generic storage and `char` for C strings.
- Put internal `static` functions at the top of `.c` files, then define public functions in the same order as the matching header.
- Prefer `#define` for simple compile-time constants.
- Keep the build simple and Makefile-driven.
