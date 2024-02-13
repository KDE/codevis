## Linux build

- [Assisted build using kdesrc-build](#assisted-build-using-kdesrc-build)
- [Manual build](#manual-build)

### Assisted build using kdesrc-build

You can use [kdesrc-build](https://invent.kde.org/sdk/kdesrc-build/) to download the necessary dependencies, and build the ones we need in a multitude of linux distributions.
If you have never used `kdesrc-build` before, it's quite simple, and this is the startup point:

1. [Clone the kdesrc-build repository](https://invent.kde.org/sdk/kdesrc-build/), or install via your package manager.

2. Run `kdesrc-build --initial-setup` to install the necessary packages for your linux distribution. 

**Note:** Even if you already have `kdesrc-build`, you are encouraged to run `--initial-setup` often, to install newer dependencies that are added to the list from time to time, running this again will not override your configuration file.

**Note:** Open and edit `~/.config/kdesrc-buildrc` to your liking, specially the `kdedir`, `source-dir` and `build-dir` packages.

3. Run `kdesrc-build codevis`

**Note**: If you are a developer, after you build the software for the first time with `kdesrc-build`, you can load the source on any IDE, and point the build folder to `$build-dir/codevis`, the IDE should pickup the `CMakeCache.txt` and the `compile_commands.json` and you can go from there.


### Manual build

In order to build Codevis manually, you need to download and install all the dependencies. The easiest way to get the dependencies is to take in consideration the [available docker images](../packing/). Choose the closest image to your system and follow the steps in the Dockerfile. **You don't need docker**, the idea is to copy the commands inside the Dockerfile and run them manually.

Once you have the dependencies installed, clone the Codevis repository, and run the following commands inside the just-clonned directory (This will install Codevis in an isolated folder):

```
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/codevis/
cmake --build . -j$(nproc)
cmake --install .
```

The binaries will be installed in the `build/codevis/bin/` folder. There are several binaries there: The Command Line Tools and the Desktop Application (`codevis_desktop`). To run it, make sure the libraries are reacheable either by installing them in your system or pointing them manually:

```
LD_LIBRARY_PATH=../lib/x86_64-linux-gnu/ ./codevis_desktop
```
