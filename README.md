# XPilot CPP

XPilot is a multiplayer gravity war game originally created in 1991–92 by
Bjørn Stabell and Ken Ronny Schouten.

- **Links & resources:** [xpilot-links.md](xpilot-links.md)

## Building on Debian 13 (Trixie)

### Prerequisites

Install the required build tools and libraries:

```sh
# Build toolchain
sudo apt install build-essential autoconf automake pkg-config

# SDL2
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-gfx-dev

# OpenGL / GLU
sudo apt install libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev

# X11
sudo apt install libx11-dev libxext-dev libxi-dev libxrandr-dev

# zlib (for compressed map data)
sudo apt install zlib1g-dev
```

Or as a single command:

```sh
sudo apt install build-essential autoconf automake pkg-config \
  libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-gfx-dev \
  libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev \
  libx11-dev libxext-dev libxi-dev libxrandr-dev \
  zlib1g-dev
```

### Compile

```sh
./configure
make
make install
```

### Run

```sh
# Start the server
src/server/xpilot-cpp-server

# In another terminal, start the X11 client
src/client/x11/xpilot-cpp-client-x11

# Optionally, start the SDL client
src/client/sdl/xpilot-cpp-client-sdl
```

### Configure options

| Flag | Description |
|------|-------------|
| `--enable-sdl-client` | Build the SDL2 client (default: auto) |
| `--enable-sdl-gameloop` | Use generic SDL game loop instead of X11 game loop |

Example:

```sh
./configure --enable-sdl-client --enable-sdl-gameloop
```

## Requirements summary

| Dependency | Debian package | Required by |
|---|---|---|
| c++20 compiler | `build-essential` | Core |
| Autotools | `autoconf automake` | Build system |
| pkg-config | `pkg-config` | Build system |
| SDL2 ≥ 2.0 | `libsdl2-dev` | Client |
| SDL2_image | `libsdl2-image-dev` | Client (textures) |
| SDL2_ttf | `libsdl2-ttf-dev` | Client (fonts) |
| SDL2_mixer | `libsdl2-mixer-dev` | Client (sound) |
| SDL2_gfx | `libsdl2-gfx-dev` | Client (primitives) |
| OpenGL | `libgl1-mesa-dev` | Client (rendering) |
| GLU | `libglu1-mesa-dev` | Client (rendering) |
| X11 | `libx11-dev` | Client (clipboard/scrap) |
| zlib | `zlib1g-dev` | Server (map compression) |

## License

See the original XPilot license in the source tree.
