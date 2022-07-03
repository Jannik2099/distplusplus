# DISTPLUSPLUS
a distcc-like distributed compilation server with an open protocol

## Status of this project
Distplusplus is currently in a very early alpha state and under heavy active development. Furthermore, the protocol is not finalized yet.
Although it should create working binaries from my limited testing, I recommend against using this yet as I haven't been able to test but a minuscule amount of possible configurations.
Additionally, the use semantics & configuration are not finalized yet.

### How does this compare to distcc
Distcc does a lot of things really well, the usage and configuration in particular are rather simple.
However distcc suffers from a few things that motivated me to try my own solution:

- terrible error messages
- protocol is incapable of distinguishing between a connection timeout and a job timeout (this was my main motivation)
- weird defaults (e.g. four jobs per host? why no auto discovery?)
- an unorthodox auth method (GSSAPI)
- a proprietary protocol
- comparatively big codebase (15k LoC vs 2.3k right now in distplusplus)

### How does this compare to icecream
I looked into icecream multiple times before coming up with this, and while I think it does a great job, I found it terribly complicated to use and a layering violation on all fronts. Particularly, there's no reason for it to handle toolchain installation in the age of containers.
I cannot give a detailed comparison as this perceived needless complexity always turned me away from giving it a proper try.

### Things not implemented yet
- Debug info handling:
    - the binary will probably work, but show garbage or missing file names in gdb
    - this also means builds are not reproduceable yet
- Support for distributed LTO, PGO, and anything else that may emit or depend on additional files
- Support for languages beyond C and C++, as feasible
- Logging and error reporting is rather rudimentary and inconsistent
- Authentication via gRPC certs - only unauthenticated traffic right now
- good documentation, man pages
- proper description of the protocol

## How to build
Distplusplus requires gRPC and boost. Build via cmake:
`cmake -B build -DCMAKE_BUILD_TYPE=Release -GNinja; cd build; ninja`

Distplusplus requires rather recent versions of Boost (at least 1.72 I think) and protobuf (3.15).
Distplusplus requires the C++17 version of lmdb++ (hoytech/lmdbxx on github).
Distplusplus requires the Microsoft GSL library (Microsoft/GSL on github).
If your distro does not provide these versions, you can use the bundled versions by downloading them with `git submodule update --recursive --init` and build with `-DSYSTEM_BOOST=FALSE -DSYSTEM_GRPC=FALSE -DSYSTEM_LMDBXX=FALSE -DSYSTEM_GSL=FALSE` as required.
Building with bundled versions is not frequently tested and was mostly added to build in CI - feedback welcome.

Distplusplus is tested with both gcc and clang and should compile without any warnings (please report anything you find)
Distplusplus is written in C++20 and probably requires at least Clang 12 or gcc 10 - I haven't tested precisely.

### CMake options overview

| name              | description                                           | default value |
|-------------------|-------------------------------------------------------|---------------|
| USE_DEFAULT_FLAGS | use builtin flags over user provided ones             | ON            |
| SYSTEM_BOOST      | whether to use system or bundled boost                | ON            |
| SYSTEM_LMDBXX     | whether to use system or bundled lmdb++               | OFF           |
| SYSTEM_GSL        | whether to use system or bundled GSL                  | OFF           |
| SYSTEM_GRPC       | whether to use system or bundled grpc and protobuf    | ON            |
| COVERAGE          | build with coverage information & add coverage target | OFF           |
| SANITIZE          | build with sanitizer (asan and ubsan) instrumentation | OFF           |
| LLVM_PROFDATA     | path to llvm-profdata - needed for coverage           | llvm-profdata |
| LLVM_COV          | path to llvm-cov - needed for coverage                | llvm-cov      |

## Instructions
Should you have come this far and still have the motivation to try distplusplus at this stage for some godforsaken reason, using it is (hopefully) pretty simple:

- server:
    - similar to distcc, the server uses a list of allowed compilers implemented as symlinks. It scans both `/usr/lib/distcc` and `/usr/libexec/distplusplus` for links. Distplusplus assumes /usr to be only modifiable by root!
    - the log level can be set via the environment variable `DISTPLUSPLUS_LOG_LEVEL` or the `--log-level` argument. Environment takes priority over command line argument. Recognized levels are `trace, debug, info, warning, error, fatal`. The default log level is `warning`.
        - a lot of events are not properly logged yet - you may not get useful metrics or debugging assistance from this.
    - the listen address is configured via the `DISTPLUSPLUS_LISTEN_ADDRESS` environment variable or the `--listen-address` argument as a `ip:port` string. Environment takes priority over command line argument.
        The string is directly parsed by gRPC. See [gRPC docs](https://grpc.github.io/grpc/cpp/md_doc_naming.html) for details.
    - the compression algorithm is specified via `--compress` - currently implemented are `NONE` and `zstd` - this is likely final.
        - this can be overridden via the `DISTPLUSPLUS_COMPRESS` env.
    - the compression level is specified via `--compression-level` and defaults to 1. The value is not checked beforehand and thus will likely cause a crash if it goes beyond zstds accepted range. Negative levels are currently not accessible in the Boost API.
        - this can be overridden via the `DISTPLUSPLUS_COMPRESSION_LEVEL` env.
    - the protocol works by first trying to reserve a job slot, and then sending the job payload. The server will periodically free unused reservations. The default timeout is 1 second, this can be set via `--reservation-timeout`
        - this can be overridden via the `DISTPLUSPLUS_RESERVATION_TIMEOUT` env.
- client:
    - usage is identical to distcc: either prefix the compiler with distplusplus or call it directly via a symlink in `/usr/libexec/distplusplus`
    - the log level can be set via the environment variable `DISTPLUSPLUS_LOG_LEVEL`. Recognized levels are `trace, debug, info, warning, error, fatal`. The default log level is `warning`.
        - a lot of events are not properly logged yet - you may not get useful metrics or debugging assistance from this.
        - some build systems (autotools) may be very picky with the stdout of compiler invocations. Reducing the log level to error may be required in some scenarios.
    - servers are configured in `/etc/distplusplus/distplusplus.toml` as `hostname:port` strings in the `[servers]` array. Example:
    ```
    [servers] = {
    "localhost:1234",
    "127.0.0.1:4321",
    "unix:/run/distplusplus.sock"
    }
    ```
    - The string is directly parsed by gRPC. See [gRPC docs](https://grpc.github.io/grpc/cpp/md_doc_naming.html) for details.
        - This means you can also use unix sockets, see gRPC docs.
        - The config file syntax is not finalized yet.
    - the compression algorithm and level are set via the `DISTPLUSPLUS_COMPRESS` and `DISTPLUSPLUS_COMPRESSION_LEVEL` environment, or the `compress` and `compression-level` config values in `/etc/distplusplus/distplusplus.toml`. Environment takes priority over config.
        - The config file syntax is not finalized yet.

Should you encounter any issues, please rebuild with `-DCMAKE_BUILD_TYPE=Debug` (this enables some extra asserts) and report anything you find. A stack trace or even coredump will also be tremendeously helpful

## Known bugs / deficiencies
- encryption is not yet implemented. DO NOT USE ACROSS UNTRUSTED NETWORKS.
- the load balancing doesn't work very well yet.
- there may be a race condition in the client queryDB. I haven't been able to reproduce it.
- the client throws in some cases where no internal error occured, but the invocation failed for other reasons.