# Debugger2 - eBPF

## Dependencies

On archlinux

```
sudo pacman -Sy bpf clang llvm llvm-libs spdlog
```

## Usage

First, create a directory for logs. By default it is:

```
sudo mkdir -p /usr/share/debugger/logs
```

If you want to change it, modify `LOGSDIR` constant in `Makefile` and create corresponding directory in your system.


Then, to compile run 

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

## Alias

You can create add an alias to your `.bashrc` file or similar:

```
alias debugger='sudo {THIS_REPO_PATH}/bin/main'
```

And then simply use debugger like this:

```
debugger [--sysdig] <cmd> <arg1> <arg2>
