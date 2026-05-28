# Immich desktop sync

Small daemon that watches directories on your local filesystem and uploads new or updated files to [Immich](https://immich.app/).

Despite the name, modifications are one-way. This program does not listen to the state of immich albums and only listens to local additions and updates. Local files will not be added, removed or modified by this program.

This program never trashes or deletes anything from Immich.

Linux only. No plans to support other OSes, but it should be relatively simple to do in a fork.

## Running

Run the executable. It does not accept any arguments. It will create a config template at `~/.config/immich-desktop-sync.conf`.

## Config

This is the auto-generated config template.

```
# Remove this line once you're done setting all values.
template=true

# An immich API key.
# Get one in Account Settings > API Keys > New API Key.
# immich-desktop-sync requires 2 permissions: asset.upload and albumAsset.create.
apiKey=changeMe

# The URL base of your immich server.
# This is the start of the URL you use to open Immich from your browser.
# If the "home" URL is http://192.168.1.240:2283/photos, the URL base is http://192.168.1.240:2283.
url=http://localhost

# Lines starting with / or ~ are directories to track.
# On the left, the location on your filesystem.
# On the right, the destination album ID.
# Multiple directories are allowed to point to the same album.

# ~/Pictures=5e98144a-357d-4289-af57-f4fe0a2063ca

# /tmp/example=dddfbedb-aeca-4c6d-8336-ece5471bcce6
```

## Libraries

libcurl to perform HTTP requests.

## Building

You'll need make, gcc and libcurl.

Run `make release`, if your machine isn't missing stuff you'll find a `./build/immich-desktop-sync`.

