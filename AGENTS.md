# Project Instructions

## Project Structure

- `src/main.c` contains process startup.
- `src/config.c` contains defaults and environment configuration.
- `src/http/` contains HTTP protocol parsing and request implementation code.
- `src/net/` contains socket and server implementation code.
- `src/datastruct/` contains internal generic data structure implementations.
- `include/` contains headers, grouped by the same subsystem directories as
  their implementations.
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
- Prefer returning small values and structs directly from value-producing functions.
- Use pointer parameters for mutation, ownership, and allocation-backed lifecycle operations.
- Prefer direct control flow, early returns, and simple cursor loops over
  bookkeeping-heavy helper logic.
- Keep function bodies on multiple lines unless a one-line definition is
  necessary for the surrounding construct.
- Prefer `#define` for simple compile-time constants.
- Keep the build simple and Makefile-driven.
