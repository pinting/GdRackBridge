# GdRackBridge

<img src="https://github.com/pinting/GdRackBridge/raw/master/screenshot.png" width="550">

## How to build

Only works on Windows!

* Have MSYS2 installed with MingGW and Visual Studio Build Tools 2019.
* Use `x86_x64 Cross Tools Command Prompt for VS 2019` to build GdRackClient by running `scons platform=windows headers_dir=../godot-headers`.
* Use `MSYS2 MinGW x64` to build GdRackPlugin by running `make` and `make install`.

## License

Copyright (c) 2022 Tornyi Dénes. Licensed under the MIT license.