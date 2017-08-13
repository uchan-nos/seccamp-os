.intel_syntax noprefix
.code64

.global IoOut8
IoOut8:
        mov     rdx, rdi
        mov     rax, rsi
        out     dx, al
        ret

.global IoOut16
IoOut16:
        mov     rdx, rdi
        mov     rax, rsi
        out     dx, ax
        ret

.global IoOut32
IoOut32:
        mov     rdx, rdi
        mov     rax, rsi
        out     dx, eax
        ret

.global IoIn8
IoIn8:
        mov     rdx, rdi
        in      al, dx
        ret

.global IoIn16
IoIn16:
        mov     rdx, rdi
        in      ax, dx
        ret

.global IoIn32
IoIn32:
        mov     rdx, rdi
        in      eax, dx
        ret

.global CompareExchange
CompareExchange: # CompareExchange(mem_addr, expected, value)
        mov     rax, rsi
        # If [mem_addr] = expected, then [mem_addr] <- value
        # Otherwise rax <- [mem_addr]
        lock cmpxchg    [rdi], rdx
        ret

.global LFencedWrite
LFencedWrite:
        lfence
        mov     [rdi], rsi
        ret
