This is a cross assembler for the LINC computer, compatible with Mary
Allen Wilke's LAP6.

There is a separate tool for managing LINCtape images in LAP6 format.
It can list the index, and read or write manuscripts and binaries.
Forensic support include seaching for index and listing unused blocks.

The original LAP6 was more than just an assembler; it was also a text
editor and file system for magnetic tapes.  It was essentially an
integrated development environment and operating system for the LINC.
It managed all this with only 1024 instructions in memory at a time.

`lap6` commands:
| Command | Arguments | Description
| ---     | ---       | ---
| dx      | tape      | Display index
| sx      | tape      | Search for possible index blocks
| sm      | tape name | Save named manuscript on tape, from stdin
| am      | tape name | Add named manuscsript from tape, to stdout
| sb      | tape name | Save named binary to tape, from stdin
| lo      | tape name | Load named binary from tape, to stdout
| dm      | tape name | Delete named manuscript
| db      | tape name | Delete named binary
| du      | tape      | Display unused blocks
| mx      | tape      | Make empty tape index
| mk      | tape      | Mark tape
| rb      | tape b n  | Read n binary blocks starting from b
| wb      | tape b n  | Write n binary blocks starting from b
| un      | tape name | Disassemble binary from tape, to stdout
| ub      | tape b n  | Disassemble binary blocks starting from b
