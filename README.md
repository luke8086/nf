### nf ###

A minimal programming language, originally designed as an interactive
environment for my [os/64](https://github.com/luke8086/os64).
Inspired by Forth, but not compatible.  Easily portable to different
platforms, including DOS and bare-bones x86.
Main characteristics: imperative, stack-based, interpreted,
compiled to bytecode.

### Building for Linux / MacOS / POSIX ###

```
$ make
$ make test
$ ./build/nf
>>> "Hello World!!!\n" printf
Hello World!!!
>>> <Ctrl+D>
$
```

### Building for DOS / BIOS ###

- Install DOSBox and QEMU
- Install [Turbo C 2.01](https://edn.embarcadero.com/article/20841) to `C:\TC`
- Install NASM to `C:\NASM`
- Compile:
    ```
    Z:\>mount d ~/nf
    Z:\>D:
    D:\>MAKE -fMAKEFILE.DOS
    D:\>BUILD\NF.COM
    >>> exit
    D:\>
    ```

- Create and test the bootable version in QEMU:
    ```
    $ make qemu
    ```

- Optionally, if you have a PC with support for booting from USB in BIOS mode, and an unused USB stick:
    ```
    $ dd if=build/nf_x86.img of=/dev/<usb-stick>
    ```

### Usage ###

**Printing**
```
>>> cr

>>>

>>> 1 2 3 . . . cr
3 2 1
>>>

>>> 1 2 3 4 5
>>> .s
1 2 3 4 5
>>>

>>> "rld!!!" "llo wo" "He" "%s%s%s\n" printf . cr
Hello world!!!
15
>>>
```

**Stack manipulation**
```
>>> 1 2 3 .s
1 2 3
>>> dup .s
1 2 3 3
>>> drop .s
1 2 3
>>> swap .s
1 3 2
>>> rot .s
3 2 1
>>> over .s
3 2 1 2
>>>
```

**Operators**
```
>>> 4 2 >= . cr
1
>>> 4 2 < . cr
0
>>> 1 0 && . cr
0
>>> 1 2 || . cr
1
>>> 0 ! . cr
1
>>> 1 2 & . cr
0
>>> 1 2 | . cr
3
>>>
```

**Conditionals**
```
>>> 1 2 < if
...   "ok\n" printf
... then
ok

>>> 4 6 > if
...     "err\n" printf
... else
...     "ok\n" printf
... then
ok
>>>
```

**Loops**
```
>>> 5 do
...     dup
... while
...     dup .
...     1 -
... repeat cr
5 4 3 2 1
>>>

>>> 0 do
...     dup .
...     1 + dup
... 5 == until cr
0 1 2 3 4
>>>
```

**Variables and subroutines**
```
>>> 123 "x" var
>>> x "x = %d\n" printf
x = 123
>>> 321 "x" :=
>>> x "x = %d\n" printf
x = 321
>>>

>>> :
...   2 *
... ; "double" def
>>> 5 double . cr
10
>>>
```

### Example application (os64) ###

```
\ ------------------------------------
\ /apps/cat.nf - display file contents
\ ------------------------------------

\ global variables

32 "size" var
0 "buf" var
-1 "fd_in" var
-1 "fd_out" var
0 "count" var
"/dev/vt" "path_out" var

\ cleanup and exit

:
        buf 0 != if buf sys-free then
        fd_in 0 >= if fd_in sys-close drop then
        fd_out 0 >= if fd_out sys-close drop then
        sys-exit
; "cat-exit" def

\ check command line arguments

argc 2 < if
        "usage: cat.nf <filename>\n" printf drop
        -1 cat-exit
then

\ allocate buffer

size sys-malloc "buf" :=
buf 0 == if -1 cat-exit then

\ open input file

1 argv sys-open "fd_in" :=
fd_in 0 < if -1 cat-exit then

\ open virtual terminal device

path_out sys-open "fd_out" :=
fd_out 0 < if -1 cat-exit then

\ copy from input file to terminal until end of file

do
        size buf fd_in sys-read "count" :=
        count 0 < if -1 cat-exit then
        count 0 == if 0 cat-exit then
        count buf fd_out sys-write drop
0 until
```
