```
     ___                     _   _     
    / _ \  ____   ___   ___ | |_| |__  
   / /_\/ |  _ \ / _ \ / __│| __| '_ \ 
  / ╱ ╲ \ | | | | (_) |\__ \| |_| | | |
  \ ╲_/ / |_| |_|\___/ |___/ \__|_| |_|
   \___/ 32bit ARM Forth dialect by grz
```

[Forth](https://en.wikipedia.org/wiki/Forth_(programming_language)) based dialect / compiler / transpiler for 32-bit ARM processors. (ARMv6 as dev. target CPU)

Built upon a minimal Forth interpreter called [ARM-ForthLite](https://github.com/grz0zrg/ARM-ForthLite/) which was revamped into this Forth dialect / compiler with syntactic changes to suit my needs, goal was to balance minimalism with features.

Dialect is slightly oriented for graphics stuff (prototyping) but is still quite generic. It deviate from the Forth way a bit. (also deviate from traditional Forth dictionary on some words)

It is able to generate ARMv2+, [LLVM IR](https://en.wikipedia.org/wiki/LLVM#Intermediate_representation) or [p5js](https://en.wikipedia.org/wiki/Processing#p5.js) code at the moment with easy support for custom targets.

[Das U-Boot](https://en.wikipedia.org/wiki/Das_U-Boot) API is used for I/O, this part may be of special interest for baremetal enthusiasts as it can be reused easily and demonstrate full usage of U-Boot API from assembly. (see *uboot* directory)

Also works without U-Boot but the I/O words must be reimplemented as needed.

Compiler code is commented and is fairly small with \~350 lines of ARM assembly plus some more for the words. (~500 total in normal mode)

See a write-up [here](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_2_arm_forth_dialect_implementation/)

Transpiler write-up [here](https://www.onirom.fr/wiki/blog/02-08-2025_transpiling_forth_dialect_to_llvm_ir/)

This compiler does not handle errors at all. (by design; simpler but has gotchas !)

Compiler size is ~6kB (binary), most of which is taken by the dictionary words especially U-Boot API related words (~3kB binary when all U-Boot API words are removed), can easily reach 1 to ~2kB with small amount of words. Generated code might take a lot of space since it use inline expansion optimization by default.

Mostly created for fun, a practical exploration / experiment of Forth / compiler / transpiler.

[CPUlator](https://cpulator.01xz.net/) was useful to debug / iterate quickly

[godbolt.org](https://godbolt.org/) was useful to debug LLVM IR code

## Forth dialect

It is compile only so ';' is not used to end definitions, code is always into some definition and a definition end when another one starts, a definition can be marked as "entry point" and all code of the definition will go at the output program address, the compiler jump to this address when compilation is done.

Dialect is built around a small parser and has some syntactic sugar borrowed from C and various other sources.

[DSSP](https://concatenative.org/wiki/view/DSSP), [Joy](https://en.wikipedia.org/wiki/Joy_(programming_language)), [Factor](https://factorcode.org/) and [r3](https://github.com/phreda4/r3d4) were influential among other common sources such as [Moving Forth](https://www.bradrodriguez.com/papers/) and [colorForth](https://en.wikipedia.org/wiki/ColorForth).

Unknown words are interpreted as hexadecimal numbers so the compiler generate a data stack push instruction on them.

### Available words (~70)

* word definition *:*
* stack manipulation *dup*, *drop*, *swap*, *over*, *rot*
* 32-bit memory access *@* *!* also have the equivalent for byte access *db@* *db!*
* data allocation *allot*, a way to get data length *dlen* and a way to align next alloted data *align*
* logic : *0=* *0<>* *=* *<>* *<* *>* *<=* *=>* *~* *&* *|* *^* *<<* *>>* *>>>*
* flow controls *loop*, *again*, *until*, *for*, *do*, *next*, *if*, *else*, *then*, *call*, *case*
* arithmetic (* / % */ - + negate >>+ >>-)
* registers access *A!* *A@* *B!* *B@* *C!* *C@* *D!* *D@*
* special immediate words *entrypoint*, *#!*
* I/O provided by Das U-Boot API *getc*, *putc*, *tstc* etc.
* Das U-Boot specifics (use extended U-Boot API) *cache_care*
* graphics related speedup word *pix*

The return stack is not exposed, dialect only handle integer arithmetic, fractional numbers can be represented as fixed-point.

## Syntactic features

These are not implemeted as a word.

* strings : "..." (no escaping though)
* arrays definition (32-bit values) : [0 1 2 3 ...]
* character literal : 'a'
* quotations : {...}
* global variables definition : >my_variable
* immediate number (push a number on compiler stack) : #42
* verbatim value (compile verbatim; useful to compile some OP code directly) : $42
* line comment starts with ;
* parenthesis are ignored so they can be used as a way to group words / improve postfix readability

Strings are length prefixed and null terminated, arrays (and the ones allocated with *allot*) are length prefixed so *dlen* can be used to fetch data length.

Reflection features such as making definitions immediate are dropped by default / design to simplify / improve cohesion, some immediate primitives and literals are still supported so they can be used for reflection features, non immediate words such as arithmetic ones cannot be used for immediate though because the compiler TOS (stack top value) is not in a register (compared to runtime), putting stack top in a register on the compiler side would be the only required change to makes non immediate words works as immediate (dual) without having to define an immediate version, did not implement it as it lock a register which was handy to have.

Features can be dropped easily to simplify the language as needed such as removing all syntactic sugars. (data, string etc.)

Feature such as [quotation](https://concatenative.org/wiki/view/Quotations) can be leveraged to simplify the language even more, this simplify all "blocks" / control structures primitive such as conditionals, loops etc. (even variables !), a perfect example in this dialect is the implementation of the "switch" statement which is a quotation with a series of quotation inside, the only primitive needed is a "case" primitive which evaluate and call the associated quotation when the condition is true :

```
getc {
    'a' {
        "first case" puts
    } case
    'b' {
        "second case" puts
    } case
    "default case" puts drop
} call
```

As an example the array syntax can be replaced by a quotation by using verbatim literals, the small advantage of the array builtin is less clutter and automatically computed length (could also be emulated easily by diffing quotation start / end address though...) :

```
: some_static_data {
        $42 $deadc0de ; verbatim literal data
    } ; data start address on stack at this point
    ; could also be called to put all the data on stack when data is defined as regular values (not verbatim)
```

An early example which was eye opening to me can be found in the [DSSP](https://concatenative.org/wiki/view/DSSP) programming language, there is no quotation in DSSP but blocks are just a call to a definition which is similar although they are not anonymous. This simplify the language.

A more primitive way to define some static data (also valid for other syntactic constructs of this dialect) would be through a special definition type for words such as "data" (similar to "entrypoint") which would do the same as *[* (change parse state) so that any body numbers are considered verbatim, this remove some complexity although it may also have limitations in usage.

This dialect serve as a transitional step towards a minimalistic implementation akin to [colorForth](https://en.wikipedia.org/wiki/ColorForth) which may restore some minimalism; retaining most features but with a simpler / streamlined design.

### Oddities

Some of this is easy to overlook when porting programs from e.g. C :

* loop constructs push iteration value on stack, there is no `i` `j` etc. words to retrieve them ! (they must be moved through stack or captured into a variable or dropped explicitly)
* *for* and *do* are handled with the same end word which is *next*
* *for* iterate backward from n (user provided value) to 0 (included)
* *do* mimic a C for loop and is able to iterate from n to another given value at a given step size, the iteration direction is automatically detected
* no negative numbers (use *negate*)
* no way to exit a loop; see the [write-up](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_2_arm_forth_dialect_implementation/) for hints how to implement a *leave* word
* *>>+* and *>>-* words were solely implemented for fast integer circle algorithm (HAKMEM 149) which i am fond of; they don't have left equivalents
* words can be self called; i don't have a "recurse" word, this choice led to small edge cases in the compiler
* hexadecimal numbers as a default without a way to determine them might be slippery e.g. when a variable is defined with a name that can be interpreted as a hexadecimal number (e.g *>a*)

### Recursion

Supported but very limited, has no mutual recursion support due to lack of forward declaration, no tail-call optimization either and since variables are global... stack is the answer !

### Variables

Variable definitions starts with *>* followed by the variable name which must starts with an ASCII character higher than 64. (this prevent words such as *>=* and *>>* etc. to be interpreted as a variable definition)

Variables are global but since this is a single pass compiler... they need to be defined before using them, they are not stored into the dictionary or on stack (same for arrays and strings) but in a region of memory made to store data (akin to data segment), might be slow to use but they are only implemented as a convenience to reduce stack noise when speed isn't needed, speedy alternatives exist such as registers, stack or inline assembly.

Variables are defined like a word definition but are stored in the data area, they are parts of the dictionary (linked with it) so the compiler find their symbol through a regular dictionary search, the compiler distinguish between a regular word and a variable through word type flag.

Variables could be converted to local easily by limiting the word search to the current definition on a variable definition. It would be much better for things like recursion.

### Errors handling

This compiler deliberately omits error handling to simplify the code.

Some errors can still be added easily such as checking that the stack depth remain consistent after control flow blocks, this check is already implemented for LLVM IR in transpile mode to merge values from different control flow paths. (not used to generate errors but for phi nodes generation)

### Syntax highlighting for Visual Studio Code

See *vscode* directory, syntax highlighting was quickly hacked up based on Filippo Tortomasi VSCode works.

## Transpiler

This compiler has two independent code generation mode : a standard mode (default) and a transpile mode, the transpile mode use a small templating engine / "VM" with stack tracking (see `src/transpiler/transpiler.inc`), it was made to output in SSA form, initial goal was to generate LLVM IR code, it can be considered as the "optimizing" part of the compiler by leveraging modern tools to transform stack based programs to registers based programs for register machines.

Side use case for the transpiler is to check program correctness, failure to generate valid target language code may signal issues with the stack. 

The use of a small templating engine means that there is no differences in the way the dictionary is used / defined between the two modes except that words content are a bunch of ASCII strings with specific instructions in transpile mode.

The transpiler code adds ~400 lines. (not counting dictionary)

See write-up [here](https://www.onirom.fr/wiki/blog/02-08-2025_transpiling_forth_dialect_to_llvm_ir/)

The output code / mode is controlled by the `FORTH_OUTPUT_CODE` define which can be `ARM_OUTPUT` or `LLVM_IR_OUTPUT` or `P5JS_OUTPUT` at the moment, `FORTH_TRANSPILE` is set based on the output mode.

Adding another language such as C etc. as a target is easy (see p5.js), just have to craft a new dictionary for the target language and craft the output code of literals / variables in `src/transpiler/your_target/misc.inc`. (also may introduce another output mode definition and adjust `FORTH_TRANSPILE` based on the mode)

Immediate words don't change much so it is okay to use the ones already defined in the LLVM dictionary, the only exception is `entrypoint` as it append the first few lines (header) of the generated code such as main function.

All the code is generated into a single function.

Debug informations for pre defined words are generated in `utils.inc`.

Some features of the language are not yet supported in transpile mode :

* no recursion support (anecdotal even in the compiler implementation, didn't bother)
* no quotation support (`case` `call` words are not supported)

Some words are target dependent :

* I/O words highly depend of the API layer / target architecture that the code will run on, the only I/O words implemented as an example is `gett` and `puts`, it use inline ARM assembly to call (hardcoded address) U-Boot API stub (no stack version) located in `bare/start.s`, other I/O words can be implemented in the same way although the `UBOOT_API_SYSCALL` stub code should be adapted for API words with more than one parameters. (push / pop shouldn't be used for those, see `UBOOT_API_SYSCALL`)

* Verbatim literals are also target dependent. (see `src/transpiler/llvm/misc.inc` for the current inline ARM assembly definition)

LLVM IR / p5.js dictionary may have many improvements left, same for the transpiler, the code was quickly iterated.

Variables code generation support two modes set by the `TRANSPILE_GENERATE_GLOBAL_VARS` option :

* first mode is a legacy mode, it generate load / store instructions which target data section, it does not use LLVM IR global variables scheme so the generated code is slower as LLVM cannot optimize it well
* second mode generate load / store with LLVM IR global variables scheme, it is faster / tinier

Second mode is best, has usability disadvantages though :

* less flexible variables naming scheme (must comply with LLVM IR variables name rules)
* variables name may collide with the variables associated to the register words (`A@` `A!` `B@` `B!` `C@` `C!` `D@` `D!`) as they are implemented as global variables (`@rA` `@rB` `@rC` `@rD`)
* only generate load / store instructions, variables definition such as `@myvar = internal global i32 0` are missing at the start of the IR code so missing lines must be added manually. ([opt](https://llvm.org/docs/CommandGuide/opt.html) can help drive the process though)

### LLVM IR generator limitations

Phi nodes are generated for conditionals but not implemented for loop constructs so it may generate invalid LLVM IR when stack values are spilled out of loops. (considered as okay as spilling values out of loop may be hard to reason about)

There should be no depth mismatch in conditionals. (untested though, may works ?)

Phi nodes are actually not needed as [temporary variables / stack](https://stackoverflow.com/a/11487580/4766443) could be used instead, it is still unclear to me whether this method is simpler than phi nodes generation though...

There is probably some other limitations that i don't know of yet.

### p5js code generation

The transpiler emit JavaScript code in p5js output mode, p5js mode has a fixed setup which emulate the RPI0 1.3 device (512MB) with a 1024x600 32-bit display.

Emitted code is wrapped into a `draw` function with a copy pass at the start of the `draw()` function to copy the framebuffer RAM region to the p5js canvas.

Data area content should be manually placed into the `data` array when variables, strings etc. are used, `DATA_BASE` should also be updated accordingly.

The emulated device setup can be changed in `src/transpiler/p5js/dict.inc` and `src/transpiler/p5js/misc.inc` for the framebuffer copy code.

There is some limitations :

* JavaScript has stricter variable name so my_variable? may trigger errors (the transpiler doesn't check this)
* the code is wrapped into the *draw()* function due to being used for graphics, infinite loop (which is necessary on RPI) may bring issues as the *draw()* function is already repeatedly called so it should be removed and any setup code (that don't run per frame) should be moved as well
* only *gett* (timer) and *puts* (text drawing) API words are implemented, they emulate U-Boot API although much simpler (no ANSI escape codes support)

## Performances / Optimizations

Some simple optimizations were added so the generated code speed vary between GCC -O0 and -O1 (without variables)

* compiler generate a constant load instruction for 8-bits numbers
* inline expansion is the default, the compiler generate inlined code

Usage of variables is slow !

Some words are only there for extra speed such as the register ones or graphics related ones such as *pix*.

### Benchmark

The dialect was benchmarked against C, the *logo.th* example also have a C implementation (see *examples/C* directory) which was used for this benchmark.

CACHE OFF (ms; with variables usage) :
* Forth : 2681
* GCC -O0 : 1822
* GCC -O1 : 392
* GCC -O2 : 84
* GCC -O3 : 79

CACHE ON (ms; with variables usage) :
* Forth : 279
* GCC -O0 : 213
* GCC -O1 : 89
* GCC -O2 : 39
* GCC -O3 : 35

Benchmark above are early benchmarks with some unoptimized words, speed may have improved a little bit with variables since then.

Latest benchmark, most variables are replaced by register access / stack words :

CACHE OFF (ms) :

* GnosTh : 1778
* GnosTh transpiled to LLVM (-O2, 2nd var. mode) : 272
* GnosTh transpiled to LLVM (-O3, 2nd var. mode) : 145
* GCC -O0 : 1822
* GCC -O1 : 406
* GCC -O2 : 62
* GCC -O3 : 59
* CLANG -O3 : 53

CACHE ON (ms) :
* GnosTh : 189
* GnosTh transpiled to LLVM (-O3, 1st var. mode) : 47
* GnosTh transpiled to LLVM (-O3, 2nd var. mode) : 26
* GnosTh transpiled to p5.js : 6
* GCC -O0 : 213
* GCC -O1 : 89
* GCC -O2 : 39
* GCC -O3 : 35
* CLANG -O3 : 29 (28 with agressive code hints)
* p5.js (manual) : 30

Not sure why transpiled code sit between GCC -O1 and -O2 with cache off, it is perhaps due to the generated IR code (load / store specifically, `inttoptr`, lack of [GEP](https://llvm.org/docs/GetElementPtr.html) ?), the `strongarm` CPU target passed to LLVM (vs `arm1176jzf-s` for GCC) might be getting in the way as well although i didn't notice any differences with cache on when CPU target match.

p5.js benchmark were done on my desktop (i7 6700 3.4 ghz), transpiled code use typed array which may explain the difference between manual vs transpiled (perhaps it generate more pleasant code for the JavaScript optimizer as well)

## Registers usage

### Compiler

Usage of these registers is kept as-is for the compiler context :

| Reg | Forth context description |
| --- | --- |
| sp | data stack address |
| r0 | return stack address |
| r1 | program address (input source) |
| r2 | dict. last word address |
| r3 | compiler output (output program address) |
| r4 | data area address (variables, array, strings) |
| r14 | dictionary end address |

### Runtime

sp (r13) is used for the data stack

r0 is used for the return stack

r5 is used to store the data stack top value for extra speedup

r6, r7, r8, r9 are used by non immediate words as general purpose registers

r1, r10, r11, r12 are used by register access words

r2, r3, r4, r14 are untouched

## Target hardware

Test / development was all done on a RPI Zero 1.3 (ARM1176JZF-S), it probably works on other 32 bits ARM that support conditional instructions (untested), side goal was ARMv2 support so it only use a subset of ARM instructions (1987 one !), the compiler doesn't use Branch with Link instruction to save a register.

ARMv7 support might introduce issues with PC jumps, it probably require adaptations like a +4 offset for certain instructions (e.g. rpush pc). Features like quotations may not work correctly without these adjustments and other aspects may also be affected.

Compiler generate inline code by default, making it optional through a definition flag is easy. (note: transpiler was built with inlining by default as well so it may require adaptation if this change)

## U-Boot note

My U-Boot version has a custom patch to makes U-Boot CPU exception handling works which makes for easier debugging on real hardware, the code still works without this patch but it will crash on an exception without any reports. (U-Boot exception report print various useful informations about register states etc.)

My U-Boot also has an extended API which has cache maintenance stuff exposed. (used by *cache_care* word)

See my U-Boot [write-up](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_1_easy_io_with_uboot_for_baremetal_usage/) for the patch and API extension.

## Examples

Examples cover most of the features.

* *logo.th* is a procedural graphics example showing a pseudo 3D logo with two background layer, serve as a benchmark
* *wireframe_cube.th* is a realtime graphics example showing an animated pseudo 3D wireframe cube
* *wireframe_cube_rpi.th* same as above but way faster with RPI 0 hw based double buffering, it use mailbox (CPU / VideoCode GPU interface) so may be adapted to work on others PI
* *test/words.th* is testing code, it use all lang. features and is used to check compiler / transpiler behavior
* *uboot_storage.th* is a U-Boot API I/O storage device example
* *fib.th* is a recursion example
* *hakmem149.th* is a sizecoding example, does not use data nor API words and produce a 60 bytes binary on RPI 0 1.3 with LLVM (see Makefile `make assemble-llvm-tiny` rule)

*lib* directory has shared code which is included into other files on build with an awk pre-processing step to mimic C like #include directives. (see `Makefile`)

Note : Samples were tested with / without CPU cache on a RPI 0 1.3 board (ARM1176JZF-S), caching may introduce predictability issues in some programs so it may be good to disable CPU caches to rule these issues out first, cache maintenance can be performed with the *cache_care* word and is used in some example such as *fib.th* or *wireframe_cube_rpi.th*.

Also note that the graphics example works on a pre initialized framebuffer (U-Boot framebuffer in my case), the RPI cube require to change U-Boot framebuffer setup to double the virtual height at initialization or do a full framebuffer setup. (see my U-Boot [write-up](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_1_easy_io_with_uboot_for_baremetal_usage/))

.th sources are converted to an assembly source (bunch of .ascii directives) on build and integrated into `program.s`. (see Makefile)

## Build

It assemble with [GNU Assembler](https://en.wikipedia.org/wiki/GNU_Assembler) and associated tools.

* `sudo apt-get install gcc-arm-linux-gnueabihf`

See `Makefile`, it use Raspberry PI toolchain by default :

* https://github.com/raspberrypi/tools

### Transpile

#### LLVM IR

To get the transpiler output i use [CPUlator](https://cpulator.01xz.net/) tool :

* adjust memory layout if needed in `program.s`, change `input-file` in `Makefile` to point to the .th to compile for then run `make` (this generate `program.th.inc` for the next step)
* put GnosTh code in CPUlator starting from `program.s`, remove U-Boot part, replace include directives with inline files content
* uncheck most debugging checks in settings pane (stop being interrupted)
* run in CPUlator until it loop forever, note program start (r3) / end address (r14/lr)
* go into "load/save memory" pane in the sidebar of "memory" view, change type to "raw", put start / end address and get the .bin
* LLVM IR code is directly readable by opening the .bin as text file but the first few bytes should be removed (or adjusted in previous step, it start by +4 bytes), same at the end

Code can be edited in CPUlator directly for quick iterations.

To get the LLVM IR code to run on device (RPI0 + U-Boot in this case) :

* do the first 3 steps above
* run in CPUlator and note start data address (in r4, initialized before jumping to the compiler code)
* run until it loop forever, note end data address (in r4)
* go into "load/save memory" pane in the sidebar of "memory" view, change type to "raw", put start / end address and get the .bin (save as `program_data.bin`)
* same for LLVM IR code .bin (save as `program.ll`) but with r3 (at start) and r14/lr (end), adjust by +4 bytes on start / end (can also open as text file and do the adjust manually)
* update `bare/start.s` r2 constant with data start address from previous steps, `sp` may have to be adjusted in this file as well if needed
* copy files into root directory and run `make assemble-llvm`, this produce a `program.bin` runnable on RPI0 with U-Boot (see [write-up](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_1_easy_io_with_uboot_for_baremetal_usage/))

Other emulator tools can be used to run the code and get the data / IR output, data could also be dumped from real device.

LLVM header (platform ABI specification) is tailored for PI0 (`target triple` and `target datalayout` config.), can be changed in `src/transpiler/llvm/dict.inc` for other targets. (start of `entrypoint_head_code`), this perhaps can go into a macro.

Data export is only needed when variable, string etc are used.

`make assemble-llvm-tiny` can be used to produce small binaries, it assume no data and no API words are used.

#### p5.js

To get the transpiler output i use [CPUlator](https://cpulator.01xz.net/) tool :

* put GnosTh code in CPUlator starting from `program.s`, remove U-Boot part, replace include directives with inline files content
* uncheck most debugging checks in settings pane (stop being interrupted)
* run in CPUlator until it loop forever, note program start (r3) / end address (r14/lr)
* also note start data address (in r4, initialized before jumping to the compiler code) and data end address after it loop forever
* go into "load/save memory" pane in the sidebar of "memory" view, change type to "raw", put start / end address and get the .bin
* go into "load/save memory" pane in the sidebar of "memory" view, change file format to "Text, comma" / "Signed decimal" / "4 bytes", put start / end address and get the .txt
* p5.js code is directly readable by opening the .bin as text file but the first few bytes should be removed (or adjusted in previous step, it start by +4 bytes), same at the end
* paste the code into some editor such as [p5.js editor](https://editor.p5js.org/)
* open data .txt and paste its content into *data* array (at the beginning)
* adjust `DATA_BASE` variable to the data start address
* infinite loop may be removed as the code is wrapped into `draw()` which is called per frame

Data export is only needed when variable, string etc are used.

## License

BSD3