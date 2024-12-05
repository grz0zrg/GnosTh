# GnosTh

[Forth](https://en.wikipedia.org/wiki/Forth_(programming_language)) based dialect / compiler for 32-bit ARM processors. (ARMv6 as dev. target CPU)

Built upon a minimal Forth interpreter called [ARM-ForthLite](https://github.com/grz0zrg/ARM-ForthLite/) which was revamped into this Forth dialect / compiler with syntactic changes to suit my needs, goal was to balance minimalism with features.

Dialect is slightly oriented for graphics stuff (prototyping) but is still quite generic. It deviate from the Forth way a bit. (also deviate from traditional Forth dictionary on some words)

[Das U-Boot](https://en.wikipedia.org/wiki/Das_U-Boot) API is used for I/O, this part may be of special interest for baremetal enthusiasts as it can be reused easily and demonstrate full usage of U-Boot API from assembly. (see *uboot* directory)

Also works without U-Boot but the I/O words must be reimplemented as needed.

Compiler code is commented and is fairly small with ~350 lines of ARM assembly plus some more for the words. (~500 total)

See a write-up [here](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_2_arm_forth_dialect_implementation/)

This compiler does not handle errors at all. (by design; simpler but has gotchas !)

Compiler size is ~6kB (binary), most of which is taken by the dictionary words especially U-Boot API related words (~3kB binary when all U-Boot API words are removed), can easily reach 1 to ~2kB with small amount of words. Generated code might take a lot of space since it use inline expansion optimization by default.

Mostly created for fun, a practical exploration / experiment of Forth / compiler.

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

### Syntax highlighting for Visual Studio Code

See *vscode* directory, syntax highlighting was quickly hacked up based on Filippo Tortomasi VSCode works.

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

Previous benchmarcks were early and had unoptimized words, speed may have improved a little bit since then.

CACHE ON (ms; most variables replaced by register access / stack words) :
* Forth : 177
* GCC -O0 : 213
* GCC -O1 : 89
* GCC -O2 : 39
* GCC -O3 : 35

Note : The actual benchmark *examples/logo.th* now display 189ms probably due to some changes i made to the code for readability.

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

r5 is used to store the data stack top value for extra speedup

r6, r7, r8, r9 are used by non immediate words as general purpose registers

r1, r10, r11, r12 are used by register access words

r2, r3, r4, r14 are untouched

## Target hardware

Test / development was all done on a RPI Zero 1.3 (ARM1176JZF-S), it probably works on other 32 bits ARM that support conditional instructions (untested), side goal was ARMv2 support so it only use a subset of ARM instructions.

ARMv7 support might introduce issues with PC jumps, it probably require adaptations like a +4 offset for certain instructions (e.g. rpush pc). Features like quotations may not work correctly without these adjustments and other aspects may also be affected.

Compiler generate inline code by default, making it optional through a definition flag is easy.

## U-Boot note

My U-Boot version has a custom patch to makes U-Boot CPU exception handling works which makes for easier debugging on real hardware, the code still works without this patch but it will crash on an exception without any reports. (U-Boot exception report print various useful informations about register states etc.)

My U-Boot also has an extended API which has cache maintenance stuff exposed. (used by *cache_care* word)

See my U-Boot [write-up](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_1_easy_io_with_uboot_for_baremetal_usage/) for the patch and API extension.

## Examples

Examples cover most of the features.

* *logo.th* is a procedural graphics example showing a pseudo 3D logo, also serve as a benchmark
* *wireframe_cube.th* is a realtime graphics example showing an animated pseudo 3D wireframe cube
* *wireframe_cube_rpi.th* same as above but way faster with RPI 0 hw based double buffering, it use mailbox (CPU / VideoCode GPU interface) so may be adapted to work on others PI
* *uboot_storage.th* is a U-Boot API I/O storage device example
* *fib.th* is a recursion example

*lib* directory has shared code which is included into other files on build with an awk pre-processing step to mimic C like #include directives. (see `Makefile`)

Note : Samples were tested with / without CPU cache on a RPI 0 1.3 board (ARM1176JZF-S), caching may introduce predictability issues in some programs so it may be good to disable CPU caches to rule these issues out first, cache maintenance can be performed with the *cache_care* word and is used in some example such as *fib.th* or *wireframe_cube_rpi.th*.

Also note that the graphics example works on a pre initialized framebuffer (U-Boot framebuffer in my case), the RPI cube require to change U-Boot framebuffer setup to double the virtual height at initialization or do a full framebuffer setup. (see my U-Boot [write-up](https://www.onirom.fr/wiki/blog/30-11-2024_writing_a_small_forth_based_rpi_os_part_1_easy_io_with_uboot_for_baremetal_usage/))

Sources are converted to an assembly source on build. (see Makefile)

## Build

It assemble with [GNU Assembler](https://en.wikipedia.org/wiki/GNU_Assembler) and associated tools.

* `sudo apt-get install gcc-arm-linux-gnueabihf`

See `Makefile`, it use Raspberry PI toolchain by default :

* https://github.com/raspberrypi/tools

## License

BSD3