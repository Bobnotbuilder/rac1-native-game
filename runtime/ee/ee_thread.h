#ifndef RAC1_RUNTIME_EE_EE_THREAD_H
#define RAC1_RUNTIME_EE_EE_THREAD_H
// The EE's main thread as a coroutine of the host (ADR 0012).
//
// The game's code blocks in places: a loop of whole frames inside one function (the screen fade
// 0x1f9838: GS packets, a vsync wait, the frame counter, again), the caller waiting in it. The
// host runs the game one frame at a time and presents a picture between frames, so a blocking
// loop either runs all its frames inside one host frame (nothing of it is seen) or has to be cut
// into a machine resumed frame by frame, which every caller would have to be split for.
//
// Instead the game frame runs on a coroutine (a stack of its own, ucontext): a vsync wait inside a
// blocking loop (ee_thread_vsync) suspends it and the host frame ends there - the host presents
// the frame's packets as the PS2 would show them after that vsync - and the next host frame
// resumes it right after the wait, the callers' C++ frames untouched. Nothing of the game runs
// while it is suspended (the PS2's main thread is blocked in the loop), and the host frame's
// own work (the picture, the sound's output) goes on as after any other frame.
//
// Outside the coroutine (the oracle, the start-up before the main loop) ee_thread_vsync returns
// at once: the loop runs to its end inside the call, as the original does with no time passing.
//
// Hardware / kernel layer: no game decision here (the kernel's vsync semaphore).
#include <functional>

namespace rac1 {

// What the host shows for a frame the game's thread was suspended in (ee_thread_vsync's argument).
enum class EeVsyncFrame {
    Kicked,  // the loop's packets of this frame, built before the wait, are sent after it: present them
    Held,    // nothing was sent for this frame: the screen keeps the picture shown (the display buffer)
    // the loop's packets of this frame are a whole frame of the game (the frame's render 0x1f85b8: the world, the
    // mobys, the 2D), built before the wait and sent after it: the host draws them as a frame of the main loop (the Vendor's
    // screens draw one before they store the frame buffer's picture, 0x2a02b0)
    Scene,
};

// Runs `fn` on the game's coroutine until it returns (true) or the game waits in a vsync inside a
// blocking loop (false). While the coroutine is suspended, a call resumes it (the `fn` given is
// then ignored: the suspended one goes on). An exception thrown by the game is rethrown here.
bool ee_thread_run(const std::function<void()>& fn);
// The game's thread is suspended in a vsync wait (a blocking loop's frame): the host frame shows it.
bool ee_thread_suspended();
// The kind of the frame the game is suspended in.
EeVsyncFrame ee_thread_frame();
// True on the game's coroutine.
bool ee_thread_inside();
// A vsync wait inside a blocking loop: the vsync hook called (a vsync passes: the host advances
// what runs on the vsyncs, the IOP's sound), then, on the coroutine and `shown`, the
// coroutine suspended until the next ee_thread_run (the host's next frame shows this frame); else
// returns at once (`shown` false: the loop's frames run inside the host frame, none of them shown -
// the fast screen fade, the default since outside the coroutine: the oracle, the start-up).
void ee_thread_vsync(EeVsyncFrame kind, bool shown = true);
// The host's per-vsync work (nullptr: none): called by every ee_thread_vsync, shown or not, on the
// game's thread, with the wait's kind (a Kicked wait sent the loop's frame, shown or not,
// so the host knows that chain went out). The main loop's own vsync (the end of a game frame) is the
// host's: not called for it.
void ee_thread_set_vsync_hook(void (*hook)(EeVsyncFrame kind));

}  // namespace rac1

#endif
