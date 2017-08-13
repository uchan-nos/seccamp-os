.intel_syntax noprefix
.code64

.extern Inthandler21
.global AsmInthandler21
AsmInthandler21:
        call    Inthandler21
        iretq
