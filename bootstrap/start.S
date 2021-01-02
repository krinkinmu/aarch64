.text
.global start
.extern main, relocate_kernel, vector_table

start:
    // Registers r0 and r1 contain arguments passed from the loader, so we
    // need to make sure that those are preserved until we call into main.
    // According to the aarch64 calling conventions register r19-r28 must be
    // preserved through function calls, so that's where we are copying r0 and
    // r1.
    mov x19, x0
    mov x20, x1

    // We don't know in advance in what ARM EL we start due to various reasons:
    //  * the hardware is not required to implement all the modes in the ARM
    //    specification
    //  * the firmware that loaded and transferred control to this code may
    //    have changed the EL.
    //
    // So we need to make sure that we are in the right EL and if not either
    // switch to the right one or fail. There are 4 EL numbered from 0 to 3
    // where the EL3 is the highest privlege level and EL0 is the lowest one.
    // The model is additionally complicated by the fact that ARM defines
    // secure and non-secure modes, so we can have a secure EL1 and non-secure
    // EL1.
    //
    // I want to experiment with virtualization and for virtualization
    // extensions I need non-secure EL2, so that's the target EL we want to end
    // up in.
    //
    // To change the EL there are only two options available:
    //  * take an interrupt/exception (can raise EL)
    //  * return from an interrupt/exception (can decrease EL).
    //
    // We can only reasonably simulate return from interrupt/exception using
    // the ERET instruction, so that implies that we can only go down from
    // higher EL toward the lower EL until we get to the one we need (it's
    // unsurprising, if anybody could raise their EL arbitrarily that would
    // defeat the purporse of hardware isolation between prvivilege levels).
    //
    // As a result we have the following possibilities:
    //  * we are already in EL2, then there is nothing we need to do
    //    additionally as it's the level we want
    //  * we are in EL3, so we need to preform minimal EL3 initialization and
    //    switch to EL2
    //  * we are in EL1/EL0 - nothing we can do as we cannot raise the level.
    //
    // The following code detects the current EL and performs the necessary
    // actions to get us to the EL we want and in the state we want. The state
    // we want in the end after all things are set and done is:
    //  - non-secure EL2
    //  - MMU is off
    //  - all interrupts are masked
    //
    // For all other paramters that we want to configure I will update this
    // comment when they become important.
    mrs x0, CurrentEL
    cmp x0, #0b1000  // the actual EL is stored in bits [2:3], thus 4 bit value
    beq in_el2
    blo in_el1

// WARNING: I have no confidence in the code between in_el3 and in_el2 as I
// wasn't able to test it. For example in QEMU, when I'm using the UEFI OVMF
// firmware that I have with -machine virt,virtualization=on,secure=on flags
// the VM just hangs before I get to load my code. I suspect that for the OVMF
// to be able to run in EL3 mode it might have to be built in a special way and
// I'm just using a wrong version, but that's just a guess.
in_el3:
    mrs x0, SCR_EL3
    // When RW is set to 1, the EL below the EL3 is AArch64
    #define SCR_EL3_RW  (1 << 10)
    // When EA is set to 0, external aborts and SErrors will not be taken to EL3
    #define SCR_EL3_EA  (1 << 3)
    // When FIQ is set to 0, FIQs are not taken to EL3
    #define SCR_EL3_FIQ (1 << 2)
    // When IRQ is set to 0, IRQs are not taken to EL3
    #define SCR_EL3_IRQ (1 << 1)
    // When NS is set to 1, levels below EL3 are non-secure
    #define SCR_EL3_NS  (1 << 0)
    and x0, x0, #(~SCR_EL3_EA)
    and x0, x0, #(~SCR_EL3_FIQ)
    and x0, x0, #(~SCR_EL3_IRQ)
    orr x0, x0, #(SCR_EL3_RW)
    orr x0, x0, #(SCR_EL3_NS)
    msr SCR_EL3, x0

    // When D bit is set, on eret debug interrupts will be masked
    #define SPSR_EL3_D (1 << 9)
    // When A bit is set, on eret abort exceptions will be masked
    #define SPSR_EL3_A (1 << 8)
    // When I bit is set, on eret IRQs will be masked
    #define SPSR_EL3_I (1 << 7)
    // When F bit is set, on eret FIQs will be masked
    #define SPSR_EL3_F (1 << 6)
    #define SPSR_EL3_DAIF (SPSR_EL3_D | SPSR_EL3_A | SPSR_EL3_I | SPSR_EL3_F)
    // 0b01000 value as a whole means that on eret we will jump to EL2 mode
    #define SPSR_EL3_M_AARCH64_EL2 (0b01000)
    // When SPSEL bit is set to 1, each level will use their own version of SP
    #define SPSR_EL3_M_SPSEL (1 << 0)
    #define SPSR_EL3_M_AARCH64_EL2H (SPSR_EL3_M_AARCH64_EL2 | SPSR_EL3_M_SPSEL)
    mov x0, xzr
    orr x0, x0, #(SPSR_EL3_D)
    orr x0, x0, #(SPSR_EL3_A)
    orr x0, x0, #(SPSR_EL3_I)
    orr x0, x0, #(SPSR_EL3_F)
    orr x0, x0, #(SPSR_EL3_M_AARCH64_EL2)
    orr x0, x0, #(SPSR_EL3_M_SPSEL)
    msr SPSR_EL3, x0

    // When we set SPSEL bit we also need to make sure that the version of SP
    // for the level we are jumping into is properly initialized. So here we
    // just copy the current value of SP to the SP for the EL2
    mov x0, sp
    msr SP_EL2, x0

    // Register ELR_EL3 is supposed to store an address where eret should
    // pass control to, thus we need to initialize it with the address of the
    // place where we want to jump.
    ldr x0, in_el2
    msr ELR_EL3, x0

    // Register SCTLR_EL2 control the execution on the EL2 including the
    // address translation parameters and caching. Specifically when it comes
    // to address translation we have a problem since the MMU in EL2 might not
    // be properly configured, so it's better to disable the translation.
    mrs x0, SCTLR_EL2
    // When 0 the data accesses at EL2 and PT walks are little endian
    #define SCTLR_EL2_EE (1 << 25)
    // When 0 makes all instruction accesses to memory from EL2 to be
    // non-cacheable, we should use 1 to control cacheability more granularly
    #define SCTLR_EL2_I (1 << 12)
    // When 0 makes all data accesses to memory from EL2 to be non-cacheable,
    // we should use 1 to control cacheability more granularly
    #define SCTLR_EL2_C (1 << 2)
    // When 0 EL2 MMU is disabled
    #define SCTLR_EL2_M (1 << 0)
    orr x0, x0, #(SCTLR_EL2_I)
    orr x0, x0, #(SCTLR_EL2_C)
    and x0, x0, #(~SCTLR_EL2_EE)
    and x0, x0, #(~SCTLR_EL2_M)
    msr SCTLR_EL2, x0

    eret
 
in_el2:
    // It may happen that we started in the EL2 and then all the code above
    // that prepares the jump from EL3 to EL2 didn't execute. So we cannot
    // really expect to run in the known state here and need to repeat a bit
    // of setup here.

    // Mask all the inpterrupts that we could possibly mask.
    msr DAIFSet, #0b1111

    // Install our own vector table to be able to catch whatever interrupts or
    // exceptions will come our way using our own handlers early. At this stage
    // the only way our own interrupt handlers should be called is if we did
    // something wrong and that resulted in an exception that cannot be masked.
    // If that happens we will be able to debug it using out own handler.
    ldr x0, vector_table
    msr VBAR_EL2, x0

    // Make sure that each EL uses a dedicated SP register and setup the SP
    // register for EL just in case. In order to setup the SP we first preserve
    // the old SP value, then, potentially, perform the switch to the separate
    // SP register per EL and then restore the value of SP using the saved
    // value.
    mov x0, sp
    mov x1, #0b1
    msr SPSel, x1
    mov sp, x0

    mrs x0, HCR_EL2
    // When 0, disables special hardware support for hosted hypervisors
    // (for example, features that help to support things like KVM vs Xen).
    // Basically boils down to emulating EL1 behavior in EL2, so that the code
    // that was created to work in EL1 (like Linux Kernel that supports KVM) can
    // work unmodifed in EL2.
    #define HCR_EL2_E2H (1 << 34)
    // When 1 it causes exceptions that would generally be routed to EL1 to be
    // routed to EL2. That is basically also needed for hosted hypervisor
    // support. We normally want it to be 0 as with the E2H above.
    #define HCR_EL2_TGE (1 << 27)
    // When 1 routes physical SError to EL2
    #define HCR_EL2_AMO (1 << 5)
    // When 1 routes physical IRQ to EL2
    #define HCR_EL2_IMO (1 << 4)
    // When 1 routes physical FIQ to EL2
    #define HCR_EL2_FMO (1 << 3)
    orr x0, x0, #(HCR_EL2_AMO)
    orr x0, x0, #(HCR_EL2_IMO)
    orr x0, x0, #(HCR_EL2_FMO)
    and x0, x0, #(~HCR_EL2_E2H)
    and x0, x0, #(~HCR_EL2_TGE)
    msr HCR_EL2, x0

    // It's a duplicate of the similar configuration above, so please take a
    // look there fore the explanation of what's going on. Even though the
    // same reasoning doesn't apply here I repeat this configuration here to
    // endup in the same state no matter what code path we take.
    mrs x0, SCTLR_EL2
    orr x0, x0, #(SCTLR_EL2_I)
    orr x0, x0, #(SCTLR_EL2_C)
    and x0, x0, #(~SCTLR_EL2_EE)
    and x0, x0, #(~SCTLR_EL2_M)
    msr SCTLR_EL2, x0

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

// If we are in the EL1 it means that either the hardware doesn't support the
// non-secure EL2 we need or for some reason it's not available to us to use.
// There is nothing we can do with this, so we reached a failure point. And in
// the absense of a better option every time it happens I just go to an
// infinite loop. The system will hang when it happens and it should be
// possible to debug it by checking where exectly the execution gets stuck.
// Given that it's not a random failure (the system either allows access to EL2
// or not), it should be ok-ish to leave without a reach failure reporting.
in_el1:
    b in_el1
