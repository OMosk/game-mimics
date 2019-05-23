# Game mimics
I want to acquire actually useful skills and understanding of how to do things
in gamedev so I've decided to make small games and demos from scratch
starting from something trivial and plain and gradually improve it
and make it more sophisticated. Making a whole game is probably too much but
single level demo is more than enough in my opinion.

This is some kind of progress log accompanied with notes.

Demos are developed for Linux/X11 machine.
I've tried to separate platform layer from game code
so other platforms should be possible to implement without much pain (probably).

## 01 - Snake

I needed to start with something so I've chosen snake.
I needed to accomplish few techinal achievements to make a demo:

1. Show a X11 window. It was relatively easy thanks to Xlib manual published
here [https://tronche.com/](https://tronche.com/gui/x/xlib/).
1. Close a X11 window. It was harder and it looks like a hack that everybody
implements.
1. Draw some pixels inside X11 window. After watching a
[HandMadeHero](https://handmadehero.org/) - a show where Casey implements a professional game
(with main dev environment on Windows machine)
from scratch I kinda know what to expect.
Filling a buffer pixel by pixel and we are done.
1. Grab keyboard input from X11. It was really a pleasure.
1. Implement a trivial game loop (200 ms per frame :) ) and logic.

I was really satisfied with X11 API and I have not seen Wayland API
so I don't know why people are complaining so much about X11.

As a C++ programmer and because gamedev is dominated by C++ I decided to use C++
but after first demo was ready I was not really satisfied.
I knew I will implement almost everything from scratch so I decided to program
next demos in C to get rid of almost everything compiler and standard library
containers do for me.
I am not time constrained so it is not really an issue.

After demo was doing reasonably well I decided to stop and keep it simple for now.

![Snake demo](./gifs/snake.gif)


## 02 - Arkanoid

I wanted second demo to be 60fps smooth but still trivial so I decided to make an arkanoid.
I still needed to rewrite platform code from C++ to C. I thought I knew what was C++ and what was compatible with C in my code and I was wrong :).

Few techical improvements here:

1. 60fps loop. Still way to go for a smooth image.
1. Switched to shared memory IPC for drawing a buffer. Old approach to transfer a big buffer over sockets 60 times per second was slightly inefficient.
1. Basic collision detection based on non-continuous movement. It is far from perfect.
1. Some basic vector and trigonometry stuff to know direction of collision.

Few notes on things that 100% needed improvement:

1. 60fps was not feeling smooth from time to time. I don't know exact reason (my usage of nanosleep, window manager compositor or X11). OpenGL or simillar allows to synchronize with screen so it should go away in future.
1. Non-continuous collision detection is very unstable but ok for now. More general and reliable approach should be found for next demos.

![Arkanoid demo](gifs/arkanoid.gif)


## 03 - Pacman-like

The main goal for next demo was loading and software rendering of bmp sprites
with support of alpha channel.
I wanted some CPU controlled entities also so pacman is the way to go.
I thought that these features will be most complex to create.
I was surpised that snap-to-grid movement became no. 1 in compexity for this demo but
it was fun to program and tweak.

Technical improvements in comparison with previous demos:

1. BMP sprites rendering. Alpha channel also supported.
1. Path finding in tiled environment.
1. Snap-to-grid movement for entities.
1. Record and replay input with according state restoring was added
to debug facilities. Inspired by [HandMadeHero](https://handmadehero.org/) implementation.
1. Trivial state machine logic for CPU entities.

Few notes:

1. Drawing sprites with semi-transparent pixels by hand is boring :) I was not able to make myself create other sprites for every direction of pacman and ghosts.
1. Good level design is hard :)

![Pacman demo](gifs/pacman.gif)

### Record-replay demo:

Here I press F2 to save state and start recording inputs.
I press F3 to stop recording and start replaying inputs. Inputs are looped
and state is restored at the beginning of every loop. F4 to exit loop and get control again. All current code is deterministic so it works perfectly for debugging and tweaking of behavior in combination with game code reloading using `dlopen`

![Record-replay demo](gifs/record-replay-loop.gif)

