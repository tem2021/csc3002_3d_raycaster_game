# 3D Raycaster Game (we don't have a name for it yet)

_A game where you feed hungry animals_

---

## How to build (using command line)

Windows:

```
gcc raycaster.c -o raycaster -lfreeglut -lopengl32 -lglu32

./raycaster
```

MacOS:

```
clang raycaster.c -o raycaster -framework OpenGL -framework GLUT

./raycaster
```

If you're using VS Code (haven't tested other apps), you should be able to just press run and it will compile and run the raycaster.

## Keyboard controls

a = turn left

d = turn right

w = move forward

s = move backward

i = toggle debug info

## Credits

- [@tem2021](https://github.com/tem2021) - Map design team
- [@prae-mai](https://github.com/prae-mai) - Map design team
- [@Julia047](https://github.com/Julia047) - Enemy design team