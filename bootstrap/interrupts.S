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

// The following four entries are for exceptions and interrupts generated when
// the code was executing in EL2 and using SP_EL2. In other words we will be
// using the same stack pointer as the code that was interrupted.
.balign 0x80
    b exception_entry
.balign 0x80
    b interrupt_entry
.balign 0x80
    b interrupt_entry
.balign 0x80
    b exception_entry

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

// The following code is written to preserve all the general purpose registers
// plus some not-quite general purpose registers like SP and LR. All the data
// will be saved on stack, so it should be large enough to handle it.
//
// Note that the implementation doesn't save all the general purpose registers
// on stack because there is no need for that. According to the calling
// convention x19-x28 registers must be preserved through the function calls,
// so we just have to be careful to not modify them inside this code and all
// the functions this code calls must do the same, thus their values will be
// preserved even if we don't store them on stack in this code.
//
// Also, looking at other implementation of the similar logic I found that the
// existing implementation do not really care about the situations when the SP
// value is not aligned on the 16-byte boundary. So it's likely that most of
// the complexity in this function related to the SP alignment isn't needed at
// all, and I just don't understand yet why that's the case. I think that the
// principle at play here is that lower ELs should use their own SP and we just
// need to make sure that our code never viaolates the SP alignment, but I have
// no clue how to check/make sure that the compiler generated code conforms to
// this principle.
interrupt_entry:
    // SP may or may not be aligned to 16-byte boundary at this point, so we
    // store the minimum amount of data required to fix the SP alignment
    // without loosing the state.
    stp x20, x21, [sp, #-16]!

    // At this point we preserved the old values of the x20 and x21 registers on
    // stack. We need to keep track of the place where those values are
    // stored, so that will take one register and the other register is free
    // for us to use as we see fit. I will keep the x21 as a pointer to where
    // the original x20 and x21 values are stored and also x21 can be used to
    // calculate the original value of the SP register.
    mov x21, sp

    // We need space to store the following:
    //   - GPRs: x0-x15
    //   - semi-GPRs: x16, x17, x18
    //   - x29 or frame pointer
    //   - x30 or link register
    //   - the original sp or stack pointer
    // In total 176 bytes, aligning SP may additionally take some space, but
    // that should not be more than additional 15 bytes
    sub x20, sp, #192
    and sp, x20, #~0b1111

    stp x0, x1, [sp, #0]

    // Now when we saved the values of x0 and x1 we can use them for
    // somethething else. I need x0 to pass the pointer to the saved registers
    // state to the interrupt as per calling convention, so I'm going to copy
    // the current value of SP there.
    mov x0, sp

    // I will use the register x1 to store the original value of the SP
    // register so that it could be saved on stack later.
    add x1, x21, #16

    // Now registers x20 and x21 aren't used for anything else, so we can
    // restore their original values from the place where x21 points to.
    ldp x20, x21, [x21]

    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x29, [sp, #144]
    stp x30, x1, [sp, #160]
    // The code we call should not use these values, but both interrupts and
    // exceptions (see the exception_entry below) use the same structure for
    // stored registers, so I'm zeroing out here unused parts.
    stp xzr, xzr, [sp, #176]

    // We pass a pointer to the saved registers to the interrupt handling
    // routine as a parameter in register x0. All the data there is properly
    // aligned and can be accessed freely. Additionally, all the modification
    // performed by the interrupt will be restored as the original values of
    // the registers, so the handler has the freedom to make some alterations.
    bl interrupt

    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x29, [sp, #144]

    // We use x0 and x1 registers to restore the original value of the SP
    // register, since we cannot directly restore to sp register using ldp.
    // So the sequence as follows:
    //  1. restore the orignal stack pointer into x0
    //  2. save the current SP into x1 for the future ldp instruction
    //  3. copy the original stack pointer from x0 to SP
    //  4. restore the original values of x0 and x1 from the stack using x1 as
    //     a base.
    ldp x30, x0, [sp, #160]
    mov x1, sp
    mov sp, x0
    ldp x0, x1, [x1, #0]

    eret

// A lot of what is written above about the interrupt_entry applies to the
// exception_entry as well. The main difference is that for exception_entry
// we additionally save a few registers that are needed to understand the
// cause of the exception.
exception_entry:
    stp x20, x21, [sp, #-16]!

    mov x21, sp
    sub x20, sp, #192
    and sp, x20, #~0b1111

    stp x0, x1, [sp, #0]

    add x1, x21, #16
    ldp x20, x21, [x21]

    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x29, [sp, #144]
    stp x30, x1, [sp, #160]

    mrs x0, ESR_EL2
    mrs x1, FAR_EL2
    stp x0, x1, [sp, #176]

    mov x0, sp
    bl exception

    ldp x2, x3, [sp, #16]
    ldp x4, x5, [sp, #32]
    ldp x6, x7, [sp, #48]
    ldp x8, x9, [sp, #64]
    ldp x10, x11, [sp, #80]
    ldp x12, x13, [sp, #96]
    ldp x14, x15, [sp, #112]
    ldp x16, x17, [sp, #128]
    ldp x18, x29, [sp, #144]
    ldp x30, x0, [sp, #160]
    mov x1, sp
    mov sp, x0
    ldp x0, x1, [x1, #0]

    eret

interrupt:
    ret

exception:
    ret
