#Game mimics
I want to acquire actually useful skills and understanding of how to do things
in gamedev so I've decided to make small games and demos from scratch
starting from something trivial and plain and gradually improve it
and make it more sophisticated. Making a whole game is probably too much but
single level demo is more than enough in my opinion.

This is some kind of progress log accompanied with notes.

Demos are developed for Linux/X11 machine.
I've tried to separate platform layer from game code
so other platforms should be possible to implement without much pain (probably).

##01 - Snake

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


##02 - Arkanoid

I wanted second demo to be 60fps smooth but still trivial so I decided to make an arkanoid.
I still needed to rewrite platform code from C++ to C. I thought I new what was C++ and what was compatible with C in my code and I was wrong :).

Few techical improvements here:

1. 60fps loop. Still way to go for a smooth image.
1. Switched to shared memory IPC for drawing a buffer. Old approach to transfer a big buffer over sockets 60 times per second was slightly inefficient.
1. Basic collision detection based on non-continuous movement. It is far from perfect.
1. Some basic vector and trigonometry stuff to know direction of collision.

Few notes on things that 100% needed improvement:

1. 60fps was not feeling smooth from time to time. I don't know exact reason (my usage of nanosleep, window manager compositor or X11). OpenGL or simillar allows to synchronize with screen so it should go away in future.
1. Non-continuous collision detection is very unstable but ok for now. More general and reliable approach should be found for next demos.

![Arkanoid demo](gifs/arkanoid.gif)
