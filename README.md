# 3D Raycaster Game (we don't have a name for it yet)

_A game where you feed hungry animals_

---

## How to build (using command line)

You can simply use 

```
make 
```

or

Windows:

```
gcc raycaster.cpp -o raycaster -lfreeglut -lopengl32 -lglu32 -lm

./raycaster
```

MacOS:

```
clang raycaster.cpp -o raycaster -framework OpenGL -framework GLUT -lm

./raycaster
```

Linux: 

```
gcc raycaster.cpp -o raycaster -lGL -lGLU -lglut -lm

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
- [@QianBeiDeWangBa2157](https://github.com/QianBeiDeWangBa2157) - Weapon design team
