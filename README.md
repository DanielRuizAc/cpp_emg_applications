# CPP BTS FREEEMG sensors applications repository

## Basic configuration:

Copy or create the files `PostBuild.cmd` `Prebuild.cmd` on the inside of the projects, uncomment the line:

```cmd
set IMPORT_ROOT=C:\Users\MADEUC5_1\Documents\BTS BioDAQ SDK 1.2\Bin
```

Also change the replace the path with the real path of the sdk into your laptop.

For each case, and change the application name at line:

```cmd
set TARGET_NAME=Cpp_manager
```

