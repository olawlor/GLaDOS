; Assembly language utility functions for GLaDOS
;   x86-64 CPU, NASM assembler
bits 64
section .text


; ---------- stack handling ---------

; start_function_with_stack
;   rcx: function f to run with new stack
;   rdx: pointer to the new stack
global start_function_with_stack
start_function_with_stack:
    push rdi ; save all the Windows preserved registers, so we can jump out of the program
    push rsi
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    mov rax,kernel_main_stack
    mov QWORD[rax],rsp
        mov rsp,rdx ; load up new stack

        sub rsp,15 ; round up stack to multiple of 16 bytes
        and rsp,~0xf
        
        call rcx ; call f using new stack
        
    ; f returned, go back to main (like exit)
    jmp return_to_main_stack

global return_to_main_stack
return_to_main_stack:
    ; Restore main stack, exit from start_function_with_stack
    mov rax,kernel_main_stack
    mov rsp,QWORD[rax]
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop rsi
    pop rdi
    mov rax,3
    ret

; -------------- syscall handling ---------
; See https://wiki.osdev.org/SYSENTER
; See MSRs at: https://www.sandpile.org/x86/msr.htm

; Sets up syscall handling
global syscall_setup
syscall_setup:
    
    ; Set the address of our syscall handler
    mov edx,0
    mov rax, syscall_entry
    mov rcx, 0xC0000082 ; LSTAR: address of syscall handler
    wrmsr ; see https://www.felixcloutier.com/x86/wrmsr
    
    ; Set bit 0 of IA32_EFR to allow "syscall" to work on Intel:
    ;   https://wiki.osdev.org/CPU_Registers_x86-64#IA32_EFER
    mov rcx, 0xC0000080 ; IA32_EFER
    rdmsr ; see https://www.felixcloutier.com/x86/rdmsr
    or rax,1 ; set bit 0
    wrmsr ; write it
    
    ; GDT magic: Set the segment numbers for the kernel
    mov rax,0 ; segment numbers get stored here
    mov rax,0x00100010; <-SUPERHACK works with UEFI GDT, data segment is +8 from here
    shl rax,32 ; code segment starts at bit 32 of STAR
    mov rcx, 0xC0000081 ; STAR: syscall segments for code and stack
    wrmsr 
    
    mov rax,0
    mov rcx,0xC0000084 ; FMASK: mask flag bits for SYSCALL 
    ; FIXME: mask the trace flag?
    
    cli ; Avoid inconsistent failures by turning off interrupts?
    
    ret

global syscall_finish
syscall_finish:
    ; We need to restore CS here for UEFI stuff to work again,
    ; but we crash trying to restore UEFI code segment.
    ; (We need our own GDT / IDT!)
    ;mov rax, restore_uefi
    ;push 0x38 
    ;push rax
    ;rex64 retf
    ;restore_uefi:
    
    mov ax,0x30
    mov ss,ax
    
    sti ; restore interrupts
    
    ret


; The address of this function gets loaded into the CPU's
;  machine-specific register to handle syscall instructions.
;  On entry: 
;     rsp is the user's stack
;     rcx contains the user's return address
;     r11 contains RFLAGS
extern handle_syscall
global syscall_entry
syscall_entry:
    sub rsp,128 ; avoid Linux "red zone" next to user code
    push rcx ; <- CPU saved the user code address here
    
    ; Save the user's syscall parameters where we can read them
    mov rcx,syscall_parameters
    mov QWORD[rcx+0x00],rdi ; Linux syscall arg 0
    mov QWORD[rcx+0x08],rsi ; Linux syscall arg 1
    mov QWORD[rcx+0x10],rdx ; Linux syscall arg 2
    mov QWORD[rcx+0x18],r10 ; Linux syscall arg 3
    mov QWORD[rcx+0x20],r8 ; Linux syscall arg 4
    mov QWORD[rcx+0x28],r9 ; Linux syscall arg 5
    
    ; Call our high level syscall handler
    sub rsp,32+8 ; <- win64 call convention wants this
    ; win64 call: (rcx,rdx,...)
    mov rdx,rcx; <- pointer to syscall args
    mov rcx,rax; <- store syscall number
    call handle_syscall
    add rsp,32+8
    
    pop rcx ; pop the user return address we saved
    add rsp,128
    jmp rcx ; <- return back to user code
    ; (could use "sysret" to change permission segments instead)



section .data

; Saved pointer to the UEFI-allocated main stack (efi_main)
kernel_main_stack:
    dq 0

align 16
; This space is used for storing the parameters to a syscall.
syscall_parameters:
    times 6 dq 0
