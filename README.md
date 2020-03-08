# Dependencies

C++17 and DCMTK

On Debian/Ubuntu, just install DMCTK from apt
`sudo apt update && sudo apt install libdcmtk-dev`

On Windows, build [DCMTK 3.6.5](https://dcmtk.org/dcmtk.php.en) from source. Only the following modules are required:
"ofstd.lib", "oflog.lib", "dcmdata.lib", "dcmimgle.lib".

Then set the enviroment variable DCMTK_HOME to point to the folder containing
the DMCTK build's `cmake` folder.

# Building

```
mkdir build
cd build
cmake ../
cmake --build .
```
