# DISTPLUSPLUS
a distcc-like distributed compilation service with an open protocol

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
- comparatively big codebase (15k LoC vs 1.4k right now in distplusplus)

### How does this compare to icecream
I looked into icecream multiple times before coming up with this, and while I think it does a great job, I found it terribly complicated to use and a layering violation on all fronts. Particularly, there's no reason for it to handle toolchain installation in the age of containers.  
I cannot give a detailed comparison as this perceived needless complexity always turned me away from giving it a proper try.

### Things not implemented yet
- Debug info handling:
	the binary will probably work, but show garbage or missing file names in gdb
- Compression
- A halfway intelligent client-side load balancing protocol
- Logging and error reporting is rather rudimentary and inconsistent
- Authentication via gRPC certs - only unauthenticated traffic right now
- good documentation, man pages
- proper description of the protocol

## How to build
Distplusplus requires gRPC and boost. Build via cmake:  
`mkdir build; cmake -B build -DCMAKE_BUILD_TYPE=Release -GNinja; cd build; ninja`  
Distplusplus is tested with both gcc and clang and should compile without any warnings (please report anything you find)

## Instructions
Should you have come this far and still have the motivation to try distplusplus at this stage for some godforsaken reason, using it is (hopefully) pretty simple:

- server:
	- similar to distcc, the server uses a list of allowed compilers implemented as symlinks. It scans both `/usr/lib/distcc` and `/usr/libexec/distplusplus` for links. Please note that this process is NOT yet fully robust: distplusplus-server does not yet check whether the directory in question is writeable by other users, which would allow injecting binaries for remote code execution
	- the log level can be set via the environment variable `DISTPLUSPLUS_LOG_LEVEL`. Recognized levels are `trace, debug, info, warning, error, fatal`. The default log level is `warning`.
	- the listen address is configured via the `DISTPLUSPLUS_LISTEN_ADDRESS` environment variable as a `ip:port` string.
		The string is directly parsed by gRPC. See gRPC docs for details.
		- this will be done via a command line argument in the future.
- client:
	- usage is identical to distcc: either prefix the compiler with distplusplus or call it directly via a symlink in `/usr/libexec/distplusplus`
	- the log level can be set via the environment variable `DISTPLUSPLUS_LOG_LEVEL`. Recognized levels are `trace, debug, info, warning, error, fatal`. The default log level is `warning`.
		- a lot of events are not properly logged yet - you may not get useful metrics or debugging assistance from this.
	- servers are configured in `/etc/distplusplus/distplusplus.toml` as `hostname:port` strings in the `[servers]` array. Example:

			[servers] = {
				"localhost:1234",
				"127.0.0.1:4321"
			}

		The string is directly parsed by gRPC. See gRPC docs for details.

Should you encounter any issues, please rebuild with `-DCMAKE_BUILD_TYPE=Debug` (this enables some extra asserts plus sanitizers) and report anything you find. A stack trace or even coredump will also be tremendeously helpful
