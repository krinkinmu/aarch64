OUTPUT_FORMAT(elf64-aarch64)
ENTRY(start)

PHDRS
{
    headers PT_PHDR PHDRS;
    text PT_LOAD FILEHDR PHDRS;
    rodata PT_LOAD;
    data PT_LOAD;
    dynamic PT_DYNAMIC;
}

SECTIONS
{
    . = SIZEOF_HEADERS;
    . = 0x1000;
    _IMAGE_START = .;

    .text : {
        _TEXT_BEGIN = .;
        *(.text)
        _TEXT_END = .;
    } :text

    .init_array : ALIGN(0x1000) {
        _INIT_BEGIN = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        _INIT_END = .;
    } :rodata

    .rodata : {
        _RODATA_BEGIN = .;
        *(.rodata)
        _RODATA_END = .;
    } :rodata

    .data.rel.ro : {
        _RELRO_BEGIN = .;
        *(.data.rel.ro)
        _RELRO_END = .;
    } :rodata

    .rela.dyn : {
        _RELA_BEGIN = .;
        *(.rela.dyn)
        _RELA_END = .;
    } :rodata

    .dynamic : {
        _DYNAMIC = .;
        *(.dynamic)
    } :rodata :dynamic

    .data : {
        _DATA_BEGIN = .;
        *(.data)
        _DATA_END = .;
    } :data

    .bss : {
        _BSS_BEGIN = .;
        *(.bss)
        _BSS_END = .;
    } :data
}
