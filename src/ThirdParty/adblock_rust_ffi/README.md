*This is a slightly adjusted version (for Desktop+ compatibility) of adblock-rust-ffi as found in the [Brave Browser repository](https://github.com/brave/brave-core/tree/master/components/adblock_rust_ffi)*.  
*Binaries have also been included for convenience.*

**A FFI crate C++ wrapper to expose functionality from [adblock-rust](https://github.com/brave/adblock-rust)**

The `adblock-rust` crate implements an Adblock Plus (ABP) filter parser and matcher. This
crate exposes C FFI functions and is configured to produce a static library so that the functionality
can be used in other languages.

Currently there are only bindings for C++.

# Development

Working on this repository requires having Rust, g++, and valgrind installed.

## Testing

There are end to end test binaries for C++ bindings, when run under
valgrind we can ensure memory is being properly freed.

## C++

### Running tests

```
make sample
```

## Regenerating the C header

```
cbindgen -o src/lib.h
```
