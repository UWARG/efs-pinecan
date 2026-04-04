- External build system (run builds from top level with support for things like bootloaders)
- Add buffer so that messages aren't handled in the interrupt 
- run the same linter as zp

- get all the .vscode/.metadata/.settings type folders figured out, so that setup is minimal

decide if there's any benefit in leaving the canard pool size as an init parameter, can we handle that as a build config that we also assert to make sure its large enough for whatever it needs to do?

https://www.st.com/resource/en/application_note/an5952-how-to-use-cmake-in-stm32cubeide-stmicroelectronics.pdf
Resource for getting the cubeide to work with an existing cmake project to some degree, to be looked into still but would be rly nice if this project worked both in vscode and cubeide, so that can take advantage of all the cmake stuff without losing cubeide ability overall

Bootloader

I think Ill be able to get stm32 to work with vscode just ok, but maybe the concern is moreso around how to handle multiple different targets and picking between them? could just be by opening the relevant project in vscode, but maybe there is something like eclipse projects in vscode where i can just click on one and it understands or sometihng. This is all in concern of the build folder generation correctly. Also really set up all the .vscodes correctly no unneeded files if possible please.
Alongside all this stuff it would be nice if I could get clang intellisense to be happy with editing pinecan and stuff from the top of the repo

Full guide to set up work environment to work on this stuff
Then full guide to adding a new project
And a subguide for porting an existing project


A debug option for pinecan that outputs some extra stuff could be nice





I'm gonna make sure pincanBoard.h doesn't include pinecan.h, but pinecan.h will include pinecanBoard.h as it will also have some pincan* API functions. This is because some things like initialization has to be passed board specific objects
all pinecan board specific interfaces that are to be used by pinecan common but NOT the user are in the pinecanBoard_internals.h so that they can be included by pinecan.c but not pinecan.h









What i'm currently thinking is the board .h is only included by pinecan.c and not pinecan.h. The user only has access to pinecan.h functions in common, and pincan.h routes to relevant board specific when necessary

TAO support
Better/correct handling of transfer id

Rename pinecan.h to pinecanCommon.h and have pinecan.h include everything, thus preventing circular includes while still having pinecan.h be the only thing that needs to be included by the target

guidance on how to set canardmempool size
setup for enabling/disabling debug macro on debug, and guidance on how to use debug vs release builds

long term:
- rtos support (probably only when needed)
- cpp support (similar to how libcanard did it likely with some extern Cs in there and stuff)