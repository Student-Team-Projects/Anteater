# Debugger2

- [What is debugger2](#introduction)
  - [Introduction](#introduction)
  - [Architecture](#architecture)
  - [Providers](#providers)
    - [eBPF](#ebpf)
    - [Sysdig](#sysdig)
  - [Consumers](#consumers)
  - [HTML printed logs](#html-printed-logs)
- [How to use debugger2](#dependencies)
  - [Dependencies](#dependencies)
  - [Compilation and usage](#compilation-and-usage)
  - [Alias](#alias)
  - [Logs](#logs)
  - [Help](#help)

## Introduction

## Architecture

## Providers

### eBPF

### Sysdig

There is a possibility to use [sysdig](https://sysdig.com/) as system events provider, though **it's not recommended**, as `ebpf` proved to be faster and more reliable.

For dynamic event filtering with sysdig, a chisel script `proc_tree` us used. You can learn more about sysdig and chisels in links below.

[sysdig wiki](https://github.com/annulen/sysdig-wiki)

[sysdig github](https://github.com/draios/sysdig)

[examples of chisels](https://github.com/draios/sysdig/tree/dev/userspace/sysdig/chisels)

## Consumers

## HTML printed logs


## Dependencies

On archlinux

```
sudo pacman -Sy bpf clang compiler-rt llvm llvm-libs spdlog
```

## Compilation and usage

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

## Help

You can see help message with

```
debugger --help
```
