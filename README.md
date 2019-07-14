Nara
====

![screenshot](./documentation/screenshot-01.png)
![screenshot](./documentation/screenshot-02.png)

The terrain in the screenshots should work as an overworld for an action rpg, but at the moment is more a renderer that a videogame.


Compilation
-----------
Dependencies are:
 - *glfw*
 - *ruby* (for compilation)
 - *pkg-config* (for compilation)
 - *ninja* (for compilation)

On Ubuntu you can install them with:
```
sudo apt install libglfw3-dev ruby pkg-config ninja-build
```

To clone the repository and compile, with:
```
git clone https://github.com/baAlex/Nara.git
cd Nara
git submodule init
git submodule update
ninja
```

Optionally you can compile a debug build with:
```
ninja -f debug.ninja
```


License
-------
Under the MIT License.
