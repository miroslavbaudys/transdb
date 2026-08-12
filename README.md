# TransDB

A transactional key/value storage server written in C++11 — a single-process network daemon that stores opaque binary records under a two-level key, keeps hot data in memory, and flushes it to a pair of append-managed files on disk.

Records are addressed by a pair of 64-bit keys: **X** (the partition, or "record index") and **Y** (the record inside that partition). Clients speak a small binary protocol over TCP; an embedded CPython interpreter allows server-side scripts and exposes a small web dashboard.

> **Provenance.** This repository was moved from `github.com/transdb/transdb`, where it was last touched in December 2014. It is presented as-is: the code targets C++11, Python 2, and Intel TBB 4.2. It has not been modernised. Expect to do work before it builds on a current toolchain — see [Caveats](#caveats).

---

## Data model

```
X (uint64)  ─┬─ Y (uint64) ─> record  (opaque bytes, max 4081 B)
             ├─ Y ─────────> record
             └─ Y ─────────> record
```

* An **X** owns a chain of 4 KB blocks managed by a `BlockManager`.
* Each block holds records packed from the front and a table of `RDF` descriptors (key + length) growing from the back, with a 5-byte `CIDF` control field in the last bytes tracking free space. This bounds a single record at `4096 - sizeof(CIDF) - sizeof(RDF)` = **4081 bytes**.
* Reading with `Y = 0` returns every record under that X.
* An optional per-X record limit (`EnableRecordLimit` / `RecordLimit`) makes an X behave as a bounded log. Y keys are held in an AVL tree, and once the limit is reached a write evicts the lowest keys to make room — the design assumes **Y is a timestamp**. A write whose key is older than the oldest surviving record is rejected with `eBMS_OldRecord` rather than evicting anything.

## Architecture

| Component | Source | Role |
|---|---|---|
| `Storage` | `src/console/Storage.cpp` | Owns the in-memory index (`RecordIndexMap`), memory accounting, defragmentation, CRC32 verification. Runs as its own thread. |
| `Block` / `BlockManager` | `src/console/Block.c`, `BlockManager.c` | The 4 KB block format and the block chain for one X. Written in C, not C++. |
| `IndexBlock` | `src/console/IndexBlock.cpp` | The on-disk index (`.idx`), also 4 KB blocks, plus the free-space list for the data file. |
| `DiskWriter` | `src/console/DiskWriter.cpp` | Background flusher. Collects dirty X's and writes them out every `DiskFlushCoef` seconds; also handles free-space dumps and defragmentation. |
| `LRUCache` | `src/console/LRUCache.cpp` | Tracks recently used X's so `CheckMemory` knows what to drop when over `MemoryLimit`. |
| `ClientSocketWorker` / `…Task` | `src/console/ClientSocketWorker*.cpp` | Thread pool that dequeues parsed packets and dispatches on opcode. Read and write tasks have separate queues and limits. |
| `PythonInterface` | `src/console/PythonInterface.cpp` | Embedded CPython. Runs a long-lived script and executes ad-hoc scripts sent over the wire. |
| `StatGenerator` | `src/console/StatGenerator.cpp` | Server counters and CPU/memory usage, returned by `C_MSG_STATUS`. |
| `src/shared/` | — | Vendored support library: sockets (epoll/kqueue/IOCP), threading, logging, config parser, byte buffers, zlib. |

Two files are created per storage, named from `DataFileName`:

* `<name>.dat` — data blocks, grown in `ReallocSize` MB steps, with a free-space list so deleted regions get reused.
* `<name>.idx` — index blocks mapping X to its start offset, block count, and CRC32.

On startup the index is loaded fully into memory; with `StartupCrc32Check = yes` every X's data is read back and checksummed. Setting `DiskFlushCoef = -1` disables flushing entirely, giving an in-memory-only server.

## Wire protocol

TCP, little-endian, one packet per request. Every packet is a 6-byte header followed by the payload:

```
uint32  payload size   (does not include the header)
uint16  opcode
uint8[] payload
```

Most payloads begin with `uint32 token` and `uint32 flags`; the server echoes the token back so clients can correlate responses. `flags` carries `ePF_COMPRESSED = 1`, set by the server when a response body was gzip-compressed (it does this for read results larger than `DataSizeForCompression`).

Example — `C_MSG_WRITE_DATA` (1):

```
token(u32) flags(u32) X(u64) Y(u64) record(bytes to end of packet)
```

replied to with `S_MSG_WRITE_DATA` (2):

```
token(u32) flags(u32) X(u64) Y(u64) status(u32)
```

### Opcodes

Client opcodes are odd, the matching server reply is the next even number. Defined in `src/shared/Enums.h`.

| C→S | S→C | Meaning |
|---|---|---|
| 1 | 2 | Write one record at (X, Y) |
| 3 | 4 | Read (X, Y); `Y = 0` reads all records under X |
| 5 | 6 | Delete record (X, Y) |
| 7 | 8 | List all X keys |
| 9 | 10 | Pong / Ping (server-initiated keepalive) |
| 11 | 12 | List all Y keys under an X |
| 13 | 14 | Server status and statistics |
| 15 | 16 | Get the configured Activity ID |
| 17 | 18 | Delete an entire X |
| 19 | 20 | Defragment the blocks of one X |
| 21 | 22 | Dump the free-space list (full or compact) |
| 23 | 24 | Write N records in one packet |
| 25 | 26 | Read the server log |
| 27 | 28 | Read the config file |
| 29 | 30 | Defragment free space |
| 31 | 32 | Execute a Python script server-side |

`Python/transDB.py` is a working client for this protocol and the practical reference for framing.

## Building

### Linux

```sh
make release      # -> transdb
make debug        # -> transdb_d
make              # both
```

The Makefile needs `g++`/`gcc` with C++11, `python-config` on PATH (Python 2), and links `-ltbb -ltbbmalloc -lz`. It compiles `src/console/` plus `src/shared/`, and builds with `-DCONFIG_USE_EPOLL`, so it is Linux-specific as written. Release adds `-DINTEL_SCALABLE_ALLOCATOR -DNDEBUG -O2`.

Note that both targets end with `cp $(EXE) ../`, copying the binary to the *parent* of the repository. Drop that line if you don't want it.

### macOS

Open `TransDB/TransDB.xcodeproj`. `tbb/lib/` carries prebuilt TBB 4.2 dylibs (including a `libc++` variant). The socket layer falls back to kqueue when `CONFIG_USE_EPOLL` is not defined.

### Windows

Open `win/TransDB.sln` (Visual Studio 2010 projects: `TransDB`, `shared`, `tester`, `zlib`). The Windows build additionally compiles `ServiceWin32.cpp` and the crash handler / `StackWalker`.

### Tester

`TransDBTester/TransDBTester.xcodeproj` builds `src/tester/`, a load generator that opens a socket and hammers the server with writes. Most of its scenarios are commented out in `src/tester/main.cpp` — uncomment the one you want.

## Running

```sh
transdb -c /etc/transdb.conf -l /var/log/transdb/ -d -p /var/run/transdb.pid
```

| Flag | Meaning |
|---|---|
| `-c <path>` | Config file. Defaults to `transdb.conf` in the working directory. |
| `-l <path>` | Directory for log files. Omit to log to stdout only. |
| `-f <n>` | Force startup despite errors (`E_FSA`); use when a damaged data file would otherwise abort the boot. |
| `-d` | Daemonize (double fork, POSIX only). |
| `-p <path>` | Write a pid file (POSIX only). |
| `-s install\|uninstall\|run [name]` | Windows service control. |

`cfg/transdb` is a Debian-style init script expecting the binary at `/usr/sbin/transdb`.

## Configuration

`cfg/transdb.conf` is the annotated template; every option is documented inline there. The sections:

* **Activity** — an ID string returned by `C_MSG_GET_ACTIVITY_ID`.
* **Storage** — file paths and name, record limits, `DiskFlushCoef`, `ReallocSize`, `StartupCrc32Check`, defragmentation thresholds.
* **Memory** — `MemoryLimit` (a *soft* limit, in MB), LRU cache reserve, index block cache size.
* **PublicSocket** — listen host/port (default `0.0.0.0:5555`), plus `WebSocketHost`/`WebSocketPort` (default `8888`). Despite the name the latter is not a WebSocket: all four values are passed straight to the Python `run()` method, which uses them to dial the server and to bind its HTTP dashboard. Also buffer sizes, ping timeouts, the Nagle latency threshold, and the parallel-task / queue-depth limits.
* **Python** — enable flag, `PythonHome`, script folder, and the module / class / run method / shutdown method to load.
* **Log** — level from `-1` (off) to `3` (debug).
* **Compression** — gzip level, buffer size, and the size thresholds above which data and records get compressed.

`ConfigWatcher` reloads the file when it changes on disk, and subscribers (including the Python interface) are notified.

The shipped config contains absolute paths from the original author's machine (`/Volumes/EXTERNAL/`, a `PythonScriptsFolderPath` under `/Users/...`). Change these before running.

## Python integration

Two distinct things share the embedded interpreter:

**A long-lived script.** `PythonModuleName` / `PythonClassName` are instantiated at startup and `PythonRunableMethod` is called on its own thread, receiving `(listenHost, listenPort, webHost, webPort)`. Bumping `PythonScriptVersion` in the config causes the script to be reloaded, with `PythonShutdownMethod` called first. `Python/loadtransDB.py` is the supplied one: it serves a small Bootstrap-styled web dashboard (`Python/template_src/`, baked into `templates.py` by `make-templates.py`) with pages for stats, fragmentation, config, logs, and a script editor. It is protected by a hardcoded HTTP Basic credential in `loadtransDB.py` — change it, or don't expose the port.

**Ad-hoc scripts.** `C_MSG_EXEC_PYTHON_SCRIPT` runs a snippet inside the server, wrapped in `execScriptSkeleton.py`. Scripts get a `g_transDB` object exposing `ReadData`, `WriteData`, `DeleteData`, and `GetAllY`, plus a `cfunctions` module with `Log_Notice` / `Log_Warning` / `Log_Error` / `Log_Debug`. `Python/scriptSkeleton.py` is a minimal example. This is arbitrary code execution by design — treat the listen port as trusted-network-only.

## Repository layout

```
Makefile              Linux build
cfg/                  annotated transdb.conf + init script
src/console/          the server: storage engine, sockets, python interface
src/tester/           load-generating test client
src/shared/           vendored support library (sockets, threads, logging, zlib)
Python/               protocol client, web dashboard, script skeletons
tbb/                  Intel TBB 4.2 (headers, docs, prebuilt macOS dylibs)
win/                  Visual Studio 2010 solution and projects
TransDB/              Xcode project for the server
TransDBTester/        Xcode project for the tester
```

## Caveats

Things you will hit if you try to run this today:

* **Python 2 only.** The Makefile calls `python-config`, and `Python/` uses `BaseHTTPServer`, `urlparse`, and Python 2 print/except syntax.
* **TBB 4.2**, vendored. The bundled binaries are macOS dylibs; Linux expects a system `libtbb`.
* **`-Werror` with `-Wall -pedantic`** on C++11 — a modern compiler will find new warnings and stop.
* **`src/console/svnversion.h`** is a leftover from Subversion; it hardcodes `"255M"` and `win/version.bat` still points at a `SlikSvn` path on a machine that no longer exists. The banner line printed at startup is cosmetic.
* **No authentication on the data port.** Anyone who can reach port 5555 can read, write, and execute Python.
* `src/shared` was previously a git submodule pointing at `transdb/shared`; it is now vendored directly into this repository.

## License

Licensed under the **GNU Affero General Public License v3.0** — see [LICENSE](LICENSE). Note section 13: running a modified version to serve users over a network obliges you to offer them its source.

Copyright © 2012–2014 Miroslav Kudrnac.

Two exceptions, both vendored third-party code that keeps its own terms:

* `tbb/` — Intel Threading Building Blocks, see `tbb/COPYING`.
* `src/shared/zlib/` — zlib, see the headers in that directory.

Some files under `src/console/` and `src/tester/` still carry older `All rights reserved` headers predating this license file. The repository-wide AGPL v3 above governs.
