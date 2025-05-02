# MLInpxPlugin

Plugin for [MyLibrary](https://github.com/ProfessorNavigator/mylibrary). This plugin imports books collections from .inpx files.

## Installation
### Linux
`git clone https://github.com/ProfessorNavigator/mlinpxplugin.git`\
`cd mlinpxplugin`\
`cmake -DCMAKE_BUILD_TYPE=Release -B _build`\
`cmake --build _build`\
`cmake --install _build`

You may need superuser privileges to execute last command. 

Also you may need to set prefix by CMAKE_INSTALL_PREFIX option (default prefix is `/usr/local`).

### Windows
You may need [MSYS2](https://www.msys2.org/) project for building and installation. Also you need to install dependencies from `Dependencies` section. After all dependencies have been installed, open MinGW terminal and execute following commands (code will be downloaded to C:\MLInpxPlugin in example):

`mkdir -pv /c/MLInpxPlugin`\
`cd /c/MLInpxPlugin`\
`git clone https://github.com/ProfessorNavigator/mlinpxplugin.git`\
`cd mlinpxplugin`\
`cmake -DCMAKE_BUILD_TYPE=Release -B _build`\
`cmake --build _build`\
`cmake --install _build`

Also you must set prefix by CMAKE_INSTALL_PREFIX option (it can be /uctr64 or /mingw64 for example).

## Dependencies
You need [MyLibrary](https://github.com/ProfessorNavigator/mylibrary) (version >= 4.0), built with option USE_PLUGINS set to `ON`. Also you may need git (to clone repository). 

### Windows
[MyLibrary](https://github.com/ProfessorNavigator/mylibrary) libraries must be in one of the system paths (indicated in `Path` system variable). Another option is to install MyLibrary by MSYS2.

## Usage
MLInpxPlugin may be installed to any directory. After installation, launch [MyLibrary](https://github.com/ProfessorNavigator/mylibrary) and open plugins window. Set full path to libmlinpxplugin, then launch plugin. Set path to .inpx file, path to books directory and new collection name. Plugins work can take some time: it needs to calculate hash sums of all collection files. After plugins work has been finished, new collection will appear in collections list of MyLibrary.

## License

GPLv3 (see `COPYING`).

## Donation

If you want to help to develop this project, you can assist it by [donation](https://yoomoney.ru/to/4100117795409573)

## Contacts

You can contact author by email \
bobilev_yury@mail.ru