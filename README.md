# Anteater

![Logo (beautiful vector graphics anteater)](./anteater.svg)

## Introduction

Anteater is a utility program that wraps execution of another program, monitors commands it executes, captures output written to `STDOUT`/`STDERR`, and prints the summary in human-friendly html logs.


## Anteater output

During execution Anteater creates html logs in `$HOME/.local/share/debugger/logs/html` directory. Each execution creates a separate directory, however all executions are available in the `index.html` file.

The html is both browser-friendly and lynx-friendly, although some information (e.g. preview of children exit codes) is unavailable in lynx due to lack of javascript support.

## Architecture

### Event model

There are four kinds of events we monitor:
- fork
- write to `STDOUT` or `STDERR`
- exec
- exit

Rather than monitoring processes, we are interested in monitoring *programs*.
A program is created upon `exec`, whereas `fork` creates a new process belonging to the same program.

Similarily to the process tree, we keep track of a program tree where each node represents a group of processes (a subtree in the process tree). When a process emits an event it is logged to the program it belongs to.


#### fork
The `fork` event is transparent in this model. Its only purpose is to trace the newly created process, which belongs to the same program as the parent.

#### write
The `write` event is pretty straightforward - the written data is logged to the current group of the process.

#### exec
The `exec` event creates a new program and hence a new node in our tree.
This new node represents the source process and all descendant processes which are still in the same group.


The interaction between forks and execs may be not trivial, so let's consider some examples.

Let's consider the following sequence of events:
- process A execs
- process A writes "a"
- process A forks, creating process B
- process B writes "b"
- process B forks, creating process C
- process C writes "c"

Process tree:
```
A
|
B
|
C
```

Program tree:
```
[A, B, C]
```

Logs contain a single page:
```
WRITE a
WRITE b
WRITE c
```

Let's consider the following sequence of events:
- process A execs
- process A writes "a"
- process A forks, creating process B
- process B forks, creating process C
- process B execs
- process B writes "b"
- process C writes "c"

Process tree:
```
A
|
B
|
C
```

Program tree:
```
A
|
[B, C]
```

Logs contain two pages:
```
WRITE a
```
and
```
WRITE b
WRITE c
```

Let's consider the following sequence of events:
- process A execs
- process A writes "a"
- process A forks, creating process B
- process B forks, creating process C
- process C execs
- process C writes "c"
- process B execs
- process B writes "b"

Process tree:
```
A
|
B
|
C
```

Program tree:
```
  A
 / \
C   B   
```

Logs contain three pages:
```
WRITE a
```
and
```
WRITE b
```
and
```
WRITE c
```


#### exit
The `exit` event is logged to all groups that logged all `exec`s of the process.

### Backend

The backend of Anteater consists of:
- `event.h` and `tracer.bpf.c` which collect and sends the events on the kernel side
- `bpf_provider` which receives the events on the debugger side, and exposes them to other parts of the program

### Frontend

The frontend consists of:
- `structure_provider` - which organizes the events from the `bpf_provider` into program tree described in the [Event model](#event-model) section. The structure is then displayed in a concrete format by a `structure_consumer`
- `structure_consumer` which displays the events in a concrete format. It also acts like an abstract factory by creating children consumers upon consuming an `exec` exec.
- `structure/html` - `structure_consumer` that outputs logs in html format.
The `html_event_consumer_root` is used as an entrypoint to the structure. In some sense it represents the debugger itself since the only meaningul event for this class is the very first `exec` created when starting the Anteater program.
The `html_event_consumer` represents a consumer for an actual program.

- `structure/plain` - `structure_consumer` that outputs logs in plain text format
- `console_logger` - `event_consumer` that simply prints the logs on the `STDOUT`

## eBPF

Events are captured using [eBPF](https://ebpf.io/). Specifically, we used [libbpf](https://github.com/libbpf/libbpf), a C API for `eBPF`.

The main idea of `eBPF` is that it allows users to hook their own programs (called BPF porgrams) into a running kernel. These programs are subject to a high number of restrictions to ensure safety.

All the BPF programs are in `src/bpf` as `*.bpf.c` files. They are managed and loaded into the kernel in the appropriate Provider.

References and useful links regarding `eBPF` and `libbpf`:

[libbpf-bootstrap](https://github.com/libbpf/libbpf-bootstrap)

[bpf-helpers](https://man7.org/linux/man-pages/man7/bpf-helpers.7.html)

[wiki](https://ebpf.io/what-is-ebpf)

[Linux kernel structures](https://docs.huihoo.com/doxygen/linux/kernel/3.7/structtask__struct.html)

[Linux kernel source code](https://elixir.bootlin.com/linux/latest/source)

## Dependencies

On archlinux

```
sudo pacman -Sy bpf clang compiler-rt llvm llvm-libs spdlog tclap gtest boost
```

## Compilation

To compile simply run
```
make
```

To run tests run
```
sudo make test
```

## Usage

The executable file is `bin/main`.

To debug a program run
```
bin/main <command> [arg]...
```
