# Codevis

![Codevis](doc/images/mainwindow.png)

## Prebuilt binaries

- [**Windows (.exe)**](https://invent.kde.org/sdk/codevis/-/jobs/1440723/artifacts/file/.kde-ci-packages/codevis-master-477-windows-cl-msvc2019-x86_64.exe)
- [**Ubuntu 22.04 (.zip)**](https://invent.kde.org/sdk/codevis/-/wikis/uploads/77fc53bdc32d671ce8d27162f6b409d1/codevis-ubuntu-22.04.zip)
- [**Ubuntu 20.04 (.zip)**](https://invent.kde.org/sdk/codevis/-/wikis/uploads/16c7045a6f17401a80652765bac3228a/codevis-ubuntu-20.04.zip)

## Prebuilt plugins

- [**Windows plugins (.zip)**](https://invent.kde.org/sdk/codevis/-/wikis/uploads/de3522514134fbcd489bde3565ae8d0c/codevis-plugins.zip)
- [**Ubuntu 22.04 plugins (.zip)**](https://invent.kde.org/sdk/codevis/-/wikis/uploads/ccb78617dec348ad130ccd9fca177ee7/codevis-plugins-ubuntu-22.04.zip)
- [**Ubuntu 20.04 plugins (.zip)**](https://invent.kde.org/sdk/codevis/-/wikis/uploads/367f4bae7c3f3f2e6c0e43355ba272cc/codevis-plugins-ubuntu-20.04.zip)

[![build status](https://invent.kde.org/sdk/codevis/badges/master/pipeline.svg)](https://invent.kde.org/sdk/codevis/-/pipelines/?ref=master)

Codevis is an analysis and visualization tool designed to assist in code maintenance for C++ codebases.

The tool provides a graph representation of C++ source code and its external dependencies.
It is aimed at developers working primarily with large codebases, who would benefit from being able to visualise
source code in order to effectively understand it and its dependencies, for debugging their code or reviewing others' work.

This tool was developed by Codethink as part of a project sponsored by Bloomberg.
The tool is based on John Lakos' ideas, which can be found in his books ("[Large Scale C++ Software Design](https://www.amazon.com/Large-Scale-Software-Design-John-Lakos/dp/0201633620/)"
and "[Large Scale C++ Volume 1: Process and Architecture](https://www.amazon.com/Large-Scale-Architecture-Addison-Wesley-Professional-Computing/dp/0201717069/)"),
as a means of displaying logical and physical entities.

## Menu

- [Quick Start](#quick-start)
- [Building](#building)
- [Contributions](#contributions)
- [License](#license)
- [Code of Conduct](#code-of-conduct)
- [Security Vulnerability Reporting](#security-vulnerability-reporting)

## Quick Start

See [Getting started](doc/getting_started.md) for information about using the tool.
There's also [CLI documentation](doc/command_line_codebase_generation.md).

## Building

To build the tool, follow the documentation for your operating system: 

- [Linux](doc/build_linux.md)
- [Windows](doc/build_windows.md)
- [MacOS](doc/build_macos.md)

## Contributions

We :heart: contributions.

Have you had a good experience with this project? Why not share some love and contribute code, or just let us know about any issues you had with it?

We welcome issues reports. Just be sure to choose the proper issue template for your issue, so that we can be sure you're providing us with the necessary information.

Before sending a Pull Request, please make sure you read our [Contribution Guidelines](CONTRIBUTING.md).

## License

Please read the [LICENSE](LICENSE).

## Code of Conduct

This project has adopted a [Code of Conduct](CODE_OF_CONDUCT.md).

## Security Vulnerability Reporting

Please refer to the project [Security Policy](SECURITY.md).
