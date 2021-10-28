### basic functionality:

- SERVER: properly implement SECURE compiler allow list
- SERVER: proper input sanitization
- CLIENT: properly implement local fallback
- proper error handling & reporting!!!
	- custom error types instead of std::runtime_error
	- info / debug logging
- SERVER: debug info handling
	- filename: important!
	- cwd: not so important (distcc doesn't), could be done via namespaces

### required for 1.0.0:

- tidy terminology
- a README
- man pages
- document protocol properly
	- ready protocol for multiple files (lto, pgo), perhaps other languages
- unit tests!!!
- CLIENT: implement queries & query storage
	- CLIENT: implement basic load balancing, requires queries
- implement compression
	- for client
	- for server
- service files
- integration patchset for portage

### wishlist:

- structured logging?
- remove all string copies - should just be spans
- sandboxing via seccomp + landlock
- maybe basic LSM profiles?
- LTO support
	- clang ThinLTO
	- gcc LTO - is this even feasible?
- PGO support
