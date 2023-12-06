.include "console-tools.m"
.include "macro-string.m"

.data
.eqv BUF_SIZE 256
input_buffer: .space BUF_SIZE
.eqv COPY_BUF_SIZE 264
copy_buffer: .space COPY_BUF_SIZE

out_prompt: .asciz "Copied string: "
.align 2
.eqv OUT_PROMPT_STRLEN 15

.text
main:
	puts("Starting internal strcpy tests...\n")
	# Starts tests that will check strcpy function
	jal run_strcpy_tests

	la t0 input_buffer
	li t1 BUF_SIZE
	read_string_rwnd("Input string", t0, t1)

	# Call strcpy via the macro
	strcpy(copy_buffer, out_prompt)

	# Call strcpy as function
	la a0 copy_buffer
	addi a0 a0 OUT_PROMPT_STRLEN
	la a1 input_buffer
	jal strcpy

	write_label_rwnd(copy_buffer)
	
	exit()
