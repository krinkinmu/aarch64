.text
.global vector_table
.balign 2048
// The following four entries are for exceptions and interrupts when we use
// SP_EL0 as a stack pointer for all ELs and the interrupt/exception is taken
// from EL2 to EL2. Currently the expectation is that each EL will use their
// own SP register version, so this should not happen.
vector_table:
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .

// The following four entries are for exceptions and interrupts using SP_EL2
// as a stack pointer, so at least the current idea is that those four entires
// will be called when we take interrupt/exception from EL2 to EL2. However
// at least currently all the interrupts/exceptions should be masked, so if the
// code is correct these should not be used.
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .

// The following four entries are for exceptions and interrupts taken from
// lower ELs running in AArch64 and AArch32 mode. At least currently there
// is no code running at the lower ELs, so there is no way we should take
// exceptions or interrupts from the lower ELs. Similarly to the first four
// entries in the table I'm using infinite loops here to hang the execution
// if it happens.
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .
.balign 0x80
    b .

