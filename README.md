Nara
====

The terrain in the screenshots should work as an overworld for an action rpg, but at the moment is more a renderer that a videogame.

![screenshot](./documentation/screenshot-04.jpg)
![screenshot](./documentation/screenshot-05.jpg)

Definitely it doesn't looks like the next Frostbite engine, but using only OpenGL ES2 I want to see this game running on mobile devices (with old-school lightmaps it still looks pretty). And yes, the screenshots delate that there are no lod mechanism on the terrain rendering, I'm working on it ⛰️📐️.


Compilation
-----------
Runtime dependencies are:
 - GLFW3
 - *An OpenGL ES2 implementation*

Compilation dependencies:
 - Python3
 - Pkg-config
 - Ninja
 - Git

On Ubuntu (and most Posix variants) *Mesa* provide the OpenGL implementation, so you can install all them with:
```
sudo apt install libglfw3-dev libgles2-mesa-dev python3 pkg-config ninja-build git
```

To clone and compile the repository, with:
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
Under MIT License.
