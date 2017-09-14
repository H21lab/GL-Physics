# GLPhysics
Gravity and collision simulator using OpenGL in C++ compiled for Windows. Android Cubes Live Wallpaper has been built based on this code.

![](https://github.com/H21lab/GL-Physics/blob/master/img/glphysics.jpg)

## Compilation
The application is build using Borland Builder (brcc32.exe). For compilation modify the BUILD.BAT

## Known limitations
Only last rotation apply to objects. The rotation matrix are not yet multiplied
The energy of objects is not shared between rotation and translation movement. Rotation can't cause movement after impact.

## Attribution
Copyright 2011, 2012 Martin Kacer

All the content and resources have been provided in the hope that it will be useful. 
Author do not take responsibility for any misapplication of it. The software is distributed
in the hope that will be useful, but WITHOUT ANY WARRANTY.

## License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
