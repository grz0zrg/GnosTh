# logo.th C sample and benchmark

C equivalent of the logo.th sample, used to benchmark my dialect against C.

Can be used to compare the code of the Forth dialect (untyped; explicit; stack machine model) vs C (typed; implicit; registers model) which is about the same but postfix with less syntactic noise for my dialect. :)

*logo.c* embed a shell script to compile itself so just run : `sh logo.c`

*start.s* has some quickly imported U-Boot API code to show the program run time in ms, also has a routine to print an integer, may have to change the API jump table offset to make it work for different U-Boot config. / build.