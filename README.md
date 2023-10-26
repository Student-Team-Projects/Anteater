# debugger

- [What is debugger](#introduction)
  - [Introduction](#introduction)
  - [Debugger output](#debugger-output)
  - [Architecture](#architecture)
  - [Providers](#providers)
    - [eBPF](#ebpf)
    - [Sysdig](#sysdig)
  - [Consumers](#consumers)
- [How to use debugger](#dependencies)
  - [Dependencies](#dependencies)
  - [Compilation and usage](#compilation-and-usage)
  - [Alias](#alias)
  - [Run traced programs as other user](#run-traced-program-as-other-user)
  - [Logs](#logs)
  - [Help](#help)

## Introduction

Debugger is a utility program that wraps other programs execution, captures all their outputs from `STDOUT` and `STDERR`, and prints them in human-friendly format using plain text or `html` (default).

## Debugger output

By default, Debugger creates html and js files with logs that are saved in `/var/lib/debugger/`. The root is `index.html` which will be automatically created if no previous Debugger executions have been recorded yet.

```
lynx /var/lib/debugger/index.html
```

## Architecture

Debugger architecture is base on simple provider-consumer pattern.

## Providers

Providers are responsible for capturing all interesting events regarding traced processes, like fork, exec, write and exit syscalls. 

After capturing an event, it can be consumed by a consumer.

Currently there are two different providers implemented and they differ in what utility they use for tracing system events: `eBPF` or `sysdig`.

### eBPF

Events are captured using [eBPF](https://ebpf.io/). Specifically, we used [libbpf](https://github.com/libbpf/libbpf), a C API for `eBPF`.

The main idea of `eBPF` is that it allows users to hook their own programs (called BPF porgrams) into a running kernel. These programs are subject to a high number of restrictions to ensure safety.

All the BPF programs are in `src/bpf` as `*.bpf.c` files. They are managed and loaded into the kernel in the appropriate Provider.

References and useful links regarding `eBPF` and `libbpf`:

[libbpf-bootstrap](https://github.com/libbpf/libbpf-bootstrap)

[bpf-helpers](https://man7.org/linux/man-pages/man7/bpf-helpers.7.html)

[wiki](https://ebpf.io/what-is-ebpf)

[Linux kernel structures](https://docs.huihoo.com/doxygen/linux/kernel/3.7/structtask__struct.html)

[Linux kernel source code](https://elixir.bootlin.com/linux/latest/source)

### Sysdig

There is a possibility to use [sysdig](https://sysdig.com/) as system events provider, though **it's not recommended**, as `ebpf` proved to be faster and more reliable.

For dynamic event filtering with sysdig, a chisel script `proc_tree.lua` us used. You can learn more about sysdig and chisels in links below.

[sysdig wiki](https://github.com/annulen/sysdig-wiki)

[sysdig github](https://github.com/draios/sysdig)

[examples of chisels](https://github.com/draios/sysdig/tree/dev/userspace/sysdig/chisels)

## Consumers

Consumers are responsible for processing events provided by providers. Currently there are two consumers implemented. 

`PlainConsumer` simply prints events to `STDOUT`. 

The default consumer is `HtmlConsumer`, which creates structured logs in form of a bunch of html and javascript files. You can find details in [output](#debugger-output) section.

## Dependencies

On archlinux

```
sudo pacman -S bpf sysdig clang compiler-rt llvm llvm-libs spdlog tclap
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

If you prefer using `sysdig` instead of `bpf`, use `--sysdig` flag (though it's probably not going to be a good decision).

```bash
sudo bin/main --sysdig <cmd> <arg1> <arg2> ...
```

## Alias

You can always move the `/bin/main` binary to your `PATH` or add an alias to your `.bashrc` file or similar:

```
alias debugger='sudo {THIS_REPO_PATH}/bin/main'
```

And then simply use debugger like this:

```
debugger [flags] <cmd> <arg1> <arg2>
```

## Run traced program as other user

To run the program you want to trace as a different user use `--user` flag:

```
debugger --user <username> ...
```

## Logs

By default, debugger creates logs from every execution in `/var/log/debugger/logs_{timestamp}.txt`. You can set custom path for output logs file using `--logp` flag:

```
debugger --logp /path/to/logs/file.txt ...
```

## Help

You can see help message with

```
debugger --help
```
