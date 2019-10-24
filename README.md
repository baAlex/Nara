Nara
====

The terrain in the screenshots should work as an overworld for an action rpg, but is more a renderer that a videogame.

Check implemented features and future work in the [projects tab](https://github.com/baAlex/Nara/projects/). Any help is welcomed ⛰️📐️!.

![screenshot](./documentation/screenshot-terrain.jpg)
![screenshot](./documentation/screenshot-wire.jpg)


Dependencies
------------
At runtime Nara requires:
 - Portaudio
 - GLFW3

For compilation:
 - Python3
 - Pkg-config
 - Ninja
 - Git

On Ubuntu you can install all dependencies with:
```
sudo apt install libglfw3-dev portaudio19-dev python3 pkg-config ninja-build git
```

And don't forget the `git submodule` steps when cloning the repo.


Compilation
-----------
To clone and compile the repository:
```
git clone https://github.com/baAlex/Nara.git
cd Nara
git submodule init
git submodule update
ninja -f posix-release.ninja
```

Optionally you can compile a debug build with:
```
ninja -f posix-debug.ninja
```


Thanks to
---------
- erikd (['libsamplerate'](https://github.com/erikd/libsamplerate) library)
- Dav1dde ([GLAD](https://github.com/Dav1dde/glad) loader)
- And all contributors of [GLFW](https://github.com/glfw/glfw/graphs/contributors) and [Portaudio](http://portaudio.com/people.htmlm)

License
-------
Under MIT License.
