# nf

An forth-like interactive programming environment for 8086+ PCs

## Prerequisites

- Working DOS environment (typically through DOSBox)
- [Turbo C 2.01](https://edn.embarcadero.com/article/20841)
- [NASM](https://www.nasm.us/pub/nasm/releasebuilds/3.01/dos/)

## Building

Just run `MAKE`

## Running in DOS

Run `BUILD\NF.COM`

## Running in QEMU

```
qemu-system-i386 -drive format=raw,file=build/NF_DISK.IMG
```

## Running on real hardware (on your own risk)

If you have a PC with support for booting from USB in BIOS mode, and an unused USB stick:

```
dd if=build/NF_DISK.IMG of=<your USB stick>
```

## Usage

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
