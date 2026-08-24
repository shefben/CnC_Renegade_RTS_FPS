# Remote screenshots

`ssurl` and `sshot`, the two 4.8.4 console commands P05 held back. They were held
back because a picture of somebody's screen leaving their machine is not something to
add by copying code across, and the note recording that said the question would not be
answered by a silent port. This is the answer.

## What the feature is

An operator sets a URL with `ssurl`. From then on `sshot <player>` asks that one
client for a picture of its game window, which the client uploads to the URL. It
exists for the same reason it existed in 4.8.4: an admin watching somebody who appears
to be seeing through walls has no other way to look at what that player is looking at.

## What was decided

Four decisions, each of them structural rather than a promise in a comment.

**The game window, never the desktop.** The capture is `WW3D::Make_Screen_Shot`, which
reads the front buffer clipped to the game window rect. There is no code path that
captures anything else, so the exposure is the contents of Renegade -- which is exactly
what an operator needs to see and nothing more.

**The player is always told.** Before the upload starts, a message goes to the player's
own message window naming the host the picture is being sent to. It is not conditional
on a setting, and there is no quiet path: a build that dropped the notice would be a
different feature. The player is told even when the upload then fails, because their
window was captured either way.

**The feature is off until an operator turns it on, and off again at restart.** The URL
lives in memory on the server and nowhere else. It was tempting to make it an
`ssgm.ini` setting -- the tech level and the mine limit live there, by Q-011 -- and that
was rejected: a capability like this should require somebody to have decided today,
not once in a file two years ago. `sshot` with no URL set is refused and says so.

**Every request is answered, including refusals.** `cCsScreenshotResultEvent` carries
back one of: uploaded, deferred because the window is not in focus, busy, capture
failed, upload failed, or no uploader on this platform. An operator who cannot tell a
client that *could* not comply from one that *did* not has been handed a tool that
lies to them.

## How it is built

| Piece | Where |
| --- | --- |
| Capture, notice, upload thread, deferral | `Code/Commando/remotescreenshot.cpp` |
| Server asks one client | `Code/Commando/scscreenshotrequestevent.cpp` |
| Client answers | `Code/Commando/csscreenshotresultevent.cpp` |
| The two commands | `Code/Commando/ttconsole.cpp` |
| Per-frame client hook | `CombatGameModeClass::Think`, and `Shutdown` for the teardown |

4.8.4 sent both halves as private chat messages beginning `"j\n"` with an opcode
number, read back out by a hooked client. Directive 0.5 has no such client, so both
halves are ordinary network events -- which is also the client-to-server reply channel
`mapch` needed, and the reason the two arrived together.

The upload is WinINet on a worker thread. WinINet is on every Windows, follows the
machine's proxy settings, and makes `https` a flag rather than a TLS stack this tree
would otherwise have to grow. The thread is handed the picture already in memory and
touches no engine state, which is why the file is read on the main thread before the
thread starts rather than by the thread itself. On a non-Windows build the uploader
answers `unsupported`, which is a result the operator sees rather than a silent
nothing.

A picture that is not in focus is worth nothing, so a request that arrives while the
window is not on screen waits, and the operator is told it is waiting. That is the one
place the design follows 4.8.4 exactly, and for the same reason: without it, alt-tab is
a way to refuse.

## What it costs

The picture is an uncompressed Targa, because a Targa writer is what this tree has and
adding libpng for one feature is not a trade worth making. At 1920x1080 that is about
6 MB per request. A receiving endpoint should expect it; an operator should not run
`sshot` in a loop. If an image codec arrives for another reason, this is the first
caller that would use it.

## What is left

Nothing blocking. The pair has not been exercised against a real client and a real
endpoint -- that needs two processes and a web server, the same shape of manual check
as the map transition one. Its parts are simple enough that the interesting failure is
the one only a real run finds: which of the six results actually comes back.
