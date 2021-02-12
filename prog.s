; Super simple demo Linux program, does bare syscalls to print and exit.
bits 64
global _start
_start:

mov rax,1 ; the (syscall) system call number of "write".
mov rdi,1 ; first parameter: 1, the stdout file descriptor
mov rsi,myStr ; data to write
mov rdx,3 ; bytes to write
syscall ; Issue the system call

mov rdi,123
mov rax,60 ; "exit" syscall
syscall 

; leave syscall's return value in rax
ret

section .data
myStr:
	db "Yo",0xa



