# Immich desktop sync

Small daemon that watches directories on your local filesystem and uploads new or updated files to Immich.

Despite the name, modifications are one-way. This program does not listen to the state of immich albums and only listens to local additions and updates. Local files will not be added, removed or modified by this program.

Linux only.

Incomplete, not ready for use yet.

## Libraries

libcurl.

## Building

run `make`, if your machine isn't missing stuff you'll find a `./build/main`.

