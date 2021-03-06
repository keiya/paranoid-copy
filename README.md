paranoid copy tool
==================
A Robust Copy Tool for Paranoids. This `cp` provides complete-exact-perfect file copying.
## Usage
```
cp [OPTION]... SOURCE DEST
cp [OPTION]... SOURCE... DIRECTORY
```
### Option
* `-c` : paranoid mode
  * verify the equality of copied file and original file.

## Build
just type `make`.
### Macros
* DEBUG
Prints very annoying debug messages.
* ERRORTEST
destroys the data while copying. this is useful for test CRC functionality.
```
dux@alchymia:~/Dropbox/2015fall/syspro/prob1/paranoid-copy$ gcc -DERRORTEST cp.c -lpthread -o cp && ./cp -c cp.c /tmp/test.c
'cp.c'->'/tmp/test.c' CRC doesn't match
```

## Copyright
### cp.c
```
Copyright (C) 2015 Keiya Chinen
s1011420@coins.tsukuba.ac.jp
```
### crc32c.c 
```
Copyright (C) 2013 Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu
```
