# hexpeek

hexpeek is a hex editor designed for efficient use with any file, especially
huge files. It
[does not map files into memory](https://mreillydev.github.io/hexpeek/man/index.html#DESCRIPTION)
or assume [file offsets](https://mreillydev.github.io/hexpeek/man/index.html#cmd_margin)
are 32 bits.
Both an interactive interface with command history
and a
[scriptable command interface](https://mreillydev.github.io/hexpeek/man/index.html#option_-x)
are available, making the tool more accessible than a typical curses tool.
Working with [generic file descriptors](https://mreillydev.github.io/hexpeek/man/index.html#option_-d)
is also supported.  

hexpeek is a fully featured editor: you can quickly jump to and work with
[specific parts](https://mreillydev.github.io/hexpeek/man/index.html#cmd_Numeric),
[repeatedly write or copy](https://mreillydev.github.io/hexpeek/man/index.html#cmd_replace)
data with ease,
[insert](https://mreillydev.github.io/hexpeek/man/index.html#cmd_insert)
and
[delete](https://mreillydev.github.io/hexpeek/man/index.html#cmd_kill)
arbitrary data,
and
[search](https://mreillydev.github.io/hexpeek/man/index.html#cmd_search)
for data in hexadecimal or bits. You can easily
[create](https://mreillydev.github.io/hexpeek/man/index.html#option_-dump)
or
[revert](https://mreillydev.github.io/hexpeek/man/index.html#option_-pack)
a hex dump.
Surgically moving data from one file to another is supported as is
[showing the differences](https://mreillydev.github.io/hexpeek/man/index.html#cmd_diff)
between two files.  

A nice variety of
[formatting](https://mreillydev.github.io/hexpeek/man/index.html#option_-format)
[options](https://mreillydev.github.io/hexpeek/man/index.html#cmd_endian)
is available, and hexpeek interprets
[scalar input](https://mreillydev.github.io/hexpeek/man/index.html#cmd_scalar)
as hexadecimal by default &mdash; no need for ugly 0x prefixes.
Paging through the file line by line can be done simply by repeatedly
pressing Enter at an empty command prompt.  

Finally, hexpeek implements a
[multi-level](https://mreillydev.github.io/hexpeek/man/index.html#option_-backup)
[backup](https://mreillydev.github.io/hexpeek/man/index.html#BACKUP_AND_RECOVERY)
mode with a
[live undo](https://mreillydev.github.io/hexpeek/man/index.html#cmd_undo).

# Platforms

This software is tested on 64 bit Debian and FreeBSD.

# Usage

See the [man page](https://mreillydev.github.io/hexpeek/man/index.html) for complete usage information.

# Examples

Print the first line (0x10 octets by default) starting at file offset 0:
```
> 0
0000000000000000: 00112233 44556677 8899aabb ccddeeff  .."3DUfw........
```

Hit Enter at an empty command prompt to advance the offset to 0x10 and
print the next line:
```
> 
0000000000000010: 00112233 44556677 8899aabb ccddeeff  .."3DUfw........
```

Now show verbose information (hexadecimal, decimal, octal, bits, high bit / low bit / bit count, text) about the 5 octets at the current offset:
```
> ,5v          # hex dec oct     bits H/L/C text
0000000000000010: 00 000 000 00000000 ././0 '\0'
0000000000000011: 11 017 021 00010001 4/0/2 DC1
0000000000000012: 22 034 042 00100010 5/1/2 '"'
0000000000000013: 33 051 063 00110011 5/0/4 '3'
0000000000000014: 44 068 104 01000100 6/2/2 'D'
```

Print 0x40 octets starting at file offset 0x10 (autoskip enabled):
```
> 10,40
0000000000000010: 00112233 44556677 8899aabb ccddeeff  .."3DUfw........
*
0000000000000040: 00112233 44556677 8899aabb ccddeeff  .."3DUfw........
```

Print out the whole file (autoskip enabled):
```
> 0:max
0000000000000000: 00112233 44556677 8899aabb ccddeeff  .."3DUfw........
*
0000000000000ff0: 00112233 44556677 8899aabb ccddeeff  .."3DUfw........
```

Within domain starting at file offset 0x7 and ending at 0x1B inclusively, search for the first octet the second nibble of which is 1:
```
> 7:1C/.1
At 11 (10 octets requested, 10 per line, hexadecimal) :
0000000000000011: 11223344 55667788 99aabbcc ddeeff00  ."3DUfw.........
```

Set the first 0x100 octets to 0xff:
```
> 0:100 r ff
```

Zero out the whole file:
```
> 0:len r 0
```

At file offset 0x23, insert 2 octets with values 0xee and 0xff:
```
> 23 i eeff
```

Append a nul delimiter to the end of the file:
```
> len i 0
```

Remove (kill) 3 octets at offset 0x10 (0x10 through 0x12 inclusive):
```
> 10,3 k
```

Replace the 0x40 octet chunk starting at 0x20 with the values located in 0x30 through 0x32 repeated:
```
> 20:60 r @30,3
```

# Developer information

[Doxygen documentation](https://mreillydev.github.io/hexpeek/doxygen-html/index.html)

A test suite is provided in the test/ directory.  

Test coverage includes fixed input patterns, multi-gigabyte files, randomized
fuzz testing, and simulated death & recovery testing of backup functionality.

# License

Licensed under the BSD-3 License.

# Author

This software was created by Michael Reilly (mreilly@mreilly.dev)
