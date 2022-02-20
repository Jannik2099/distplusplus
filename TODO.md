### basic functionality:

- SERVER: debug info handling
    - filename: important!
    - cwd: not so important (distcc doesn't), could be done via namespaces

### required for 1.0.0:

- tidy terminology
- man pages
- document protocol properly
    - ready protocol for multiple files (lto, pgo), perhaps other languages
- more unit tests!!!
- CLIENT: implement queries & query storage
    - CLIENT: implement basic load balancing, requires queries
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
