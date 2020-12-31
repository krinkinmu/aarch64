.text
.global start
.extern main, relocate_kernel

start:
    // Mask IRQ and FIQ interrupts, so that execution will not be interrupted
    // by any external device that we can't know about yet.
    msr daifset, #0b0011

    // Registers r0 and r1 contain arguments passed from the loader, so we
    // need to make sure that those are preserved until we call into main.
    // According to the aarch64 calling conventions register r19-r28 must be
    // preserved through function calls, so that's where we are copying r0 and
    // r1.
    mov x19, x0
    mov x20, x1

    // Calculate the difference between the runtime and linktime addresses in
    // x0. The calculatation is based on calculating the difference between
    // known linktime address of _IMAGE_START (0x1000, hardcoded in the linker
    // script) and the actual runtime address of the _IMAGE_START calculated
    // using PC and the known offset of _IMAGE_START from the instruction
    // location.
    adr x0, _IMAGE_START
    sub x0, x0, 0x1000

    // Obtain the array of relocation entries that are conviniently marked
    // using _RELA_BEGIN and _RELA_END in the linker script.
    adr x1, _RELA_BEGIN
    adr x2, _RELA_END

    // Call the function that will actually relocate the kernel code by going
    // over the relocation entries and updating addresses there. The function
    // takes 3 parameters:
    //  * x0 - difference between runtime and linktime addresses
    //  * x1 - begining of the array of the relocation entries
    //  * x2 - end of the array of the relocation entries
    bl relocate_kernel

    // All the preparations are complete now, so we can call main.
    mov x0, x19
    mov x1, x20
    b main
