# Debugger2 - eBPF

## Dependencies

On archlinux

```
sudo pacman -Sy bpf clang llvm
```


## Usage

To compile run 

```bash
sudo make chisel
make
```

To do cleanup run

```bash
make clean
```

This creates an executable in `bin`.

Usage from repository base:

```bash
sudo bin/main <cmd> <arg1> <arg2> ...
```

If you prefer using `sysdig` instead of `bpf`, use `--sysdig` flag:

```bash
sudo bin/main --sysdig <cmd> <arg1> <arg2> ...
```