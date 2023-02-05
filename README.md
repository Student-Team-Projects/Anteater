# Debugger2 - eBPF

## Dependencies

On archlinux

```
sudo pacman -Sy bpf clang compiler-rt llvm llvm-libs spdlog
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

## Alias

You can add an alias to your `.bashrc` file or similar:

```
alias debugger='sudo {THIS_REPO_PATH}/bin/main'
```

And then simply use debugger like this:

```
debugger [flags] <cmd> <arg1> <arg2>
```

## Logs

By default, debugger creates logs from every execution in `/temp/debugger/logs/logs_{timestamp}.txt`. You can set custom path for output logs file using `-logp` flag:

```
debugger -logp /path/to/logs/file.txt ...
```
