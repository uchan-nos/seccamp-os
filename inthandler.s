.intel_syntax noprefix
.code64

.extern Inthandler00
.global AsmInthandler00
AsmInthandler00:
        call    Inthandler00
        iretq

.extern Inthandler01
.global AsmInthandler01
AsmInthandler01:
        call    Inthandler01
        iretq

.extern Inthandler03
.global AsmInthandler03
AsmInthandler03:
        call    Inthandler03
        iretq

.extern Inthandler04
.global AsmInthandler04
AsmInthandler04:
        call    Inthandler04
        iretq

.extern Inthandler05
.global AsmInthandler05
AsmInthandler05:
        call    Inthandler05
        iretq

.extern Inthandler06
.global AsmInthandler06
AsmInthandler06:
        call    Inthandler06
        iretq

.extern Inthandler07
.global AsmInthandler07
AsmInthandler07:
        call    Inthandler07
        iretq

.extern Inthandler08
.global AsmInthandler08
AsmInthandler08:
        call    Inthandler08
        iretq

.extern Inthandler0a
.global AsmInthandler0a
AsmInthandler0a:
        call    Inthandler0a
        iretq

.extern Inthandler0b
.global AsmInthandler0b
AsmInthandler0b:
        call    Inthandler0b
        iretq

.extern Inthandler0c
.global AsmInthandler0c
AsmInthandler0c:
        call    Inthandler0c
        iretq

.extern Inthandler0d
.global AsmInthandler0d
AsmInthandler0d:
        push    rbp
        mov     rbp, rsp

        push    rdi
        push    rsi
        push    rdx
        push    rcx
        push    rax

        mov     rdi, [rbp + 8]  # error code
        mov     rsi, [rbp + 16] # rip
        mov     rdx, [rbp + 24] # cs
        mov     rcx, [rbp + 32] # rflags

        call    Inthandler0d

        pop     rax
        pop     rcx
        pop     rdx
        pop     rsi
        pop     rdi

        pop     rbp
        iretq

.extern Inthandler0e
.global AsmInthandler0e
AsmInthandler0e:
        call    Inthandler0e
        iretq

.extern Inthandler21
.global AsmInthandler21
AsmInthandler21:
        call    Inthandler21
        iretq
