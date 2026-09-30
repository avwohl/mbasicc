# Installation

Every way to install or build mbasicc: packages, the pre-built binary, source builds on Linux and macOS, a build without editline, and the FreeDOS/DJGPP build. Back to the [README](../README.md).

## From Package (Recommended)

**Debian/Ubuntu:**
```bash
# Download the latest .deb from releases
sudo dpkg -i mbasicc_X.Y.Z_amd64.deb
sudo apt-get install -f  # Install dependencies if needed
```

**Fedora/RHEL/CentOS:**
```bash
# Download the latest .rpm from releases
sudo rpm -i mbasicc-X.Y.Z-1.*.rpm
```

## From Pre-built Binary

```bash
# Download and extract the binary tarball
tar xzf mbasicc-X.Y.Z-linux-amd64.tar.gz
sudo cp mbasicc-X.Y.Z-linux-amd64/mbasicc /usr/local/bin/
```

## Building from Source

**Requirements:**
- C++17 compiler (g++ 7+ or clang++ 5+)
- libedit development files
- make

**Debian/Ubuntu:**
```bash
sudo apt-get install build-essential libedit-dev
git clone https://github.com/avwohl/mbasicc.git
cd mbasicc
make
sudo cp mbasicc /usr/local/bin/
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ make libedit-devel
git clone https://github.com/avwohl/mbasicc.git
cd mbasicc
make
sudo cp mbasicc /usr/local/bin/
```

**macOS:**
```bash
# libedit is included in macOS
git clone https://github.com/avwohl/mbasicc.git
cd mbasicc
make
sudo cp mbasicc /usr/local/bin/
```

## Building Without editline

If libedit is not available on your system, build with `-DMBASIC_NO_EDITLINE`
and drop `-ledit` from the link line. For example:

```bash
make CXXFLAGS="-std=c++17 -O2 -DMBASIC_NO_EDITLINE" LDFLAGS=
```

This provides basic line input without history or line editing features.

## FreeDOS / DJGPP

`Makefile.djgpp` cross-compiles a 32-bit DJGPP `mbasicc.exe` that runs on
FreeDOS, MS-DOS, and DOSBox. The build automatically uses the editline-free
fallback (no extra dependencies).

**Cross-compile from Linux/macOS:**
```bash
# Install the DJGPP cross-compiler toolchain (one-time):
#   Linux: download from https://github.com/andrewwutw/build-djgpp/releases
#          and prepend its bin/ directory to PATH
#   macOS: brew install andrewwutw/extras/djgpp
make -f Makefile.djgpp
# Output: mbasicc.exe
```

**Native build under FreeDOS** (with DJGPP installed):
```bash
make -f Makefile.djgpp CROSS_PREFIX=
```

The CI builds this target on every push and uploads `mbasicc.exe` as an
artifact. To run the resulting binary, copy `mbasicc.exe` (and `cwsdpmi.exe`,
shipped with DJGPP, if needed) to your FreeDOS system.
