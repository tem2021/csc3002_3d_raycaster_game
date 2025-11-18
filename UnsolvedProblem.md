# 2025-11-06

Linzheng : Using the raycaster algorithm, under the 2D view,  if we set the resolution to be large enough, you can notice as the player moved down the map, there is a clear gap between the casting ray and the wall (if you looks down or right) or a clear overlaps between the casting ray and the wall (if you looks up or left). This problem is obvious in 2D view, but not for 3D view, 

---

Comment [Linzheng]: I remove the 2D view and preserve the 3D view only.

# 2025-11-16  [Solved]

Praemai: For some reason putting a wall to the top left of the player (1 is wall 0 is air and p is player)

```
100
0p0
000
```

This makes the player unable to move but you can still look around using the mouse.

This seems to be the only affected block? Maybe it's due to where the player is facing or how much space the player takes up? Not sure what's going on, right now just refraining from placing walls in the 8 blocks that surround the player could be safest.

--- 

Comment [Linzheng]: For this problem, I think it's because how we initialize the game based on the map (level1.h), I believe that is because some of the float point calculating problem. When we initialize the game, the player is generated inside the wall (i.e. 1) in your example instead of the original p place. I believe this is the reason. Let me check the code.

Comment [Linzheng] 2025-11-18: Update the logic of `Map.cpp` help me solve the bug

# 2025-11-17
Linzheng: For the Map Editor, if the texture files are too much, then the Map Editor can only show the first 48 textures I guess 

---

Comment [Linzheng]: I believe we won't meet such problem since our game don't need so many textures.
