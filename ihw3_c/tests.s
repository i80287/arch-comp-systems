.include "io_utils.m"

.data
.eqv TEXT_BUF_SIZE 4096
tests_buffer:
	.align 2
	.space TEXT_BUF_SIZE

.macro test_function(%fname_input, %fname_output, %test_num)
.data
	.align 2
	fname_input_l: .asciz %fname_input
	.align 2
	fname_output_l: .asciz %fname_output
.text
	la a0	fname_input_l
	la a1 tests_buffer
	li a2 TEXT_BUF_SIZE
	jal fread

	bltz a0 test_function_error
	mv s2 a0

	mv a1 a0
	la a0 tests_buffer
	jal string_swap_words_chars

	mv a2 s2
	la a0 fname_output_l
	la a1 tests_buffer
	jal fwrite
	bltz a0 test_function_error

	print_string("Func test ")
	print_int(%test_num)
	print_string(" passed\n")
	b test_function_end
test_function_error:
	print_string("Func test ")
	print_int(%test_num)
	print_string(" not passed\n")
test_function_end:
	addi %test_num %test_num 1
.end_macro

.global tests
.text
tests:
	push(ra)
	push(s1)
	push(s2)

	li s1 1
	test_function("tests/t1_i.txt", "tests/t1_o.txt", s1)
	test_function("tests/t2_i.txt", "tests/t2_o.txt", s1)
	test_function("tests/t3_i.txt", "tests/t3_o.txt", s1)
	test_function("tests/t4_i.txt", "tests/t4_o.txt", s1)
	test_function("tests/t5_i.txt", "tests/t5_o.txt", s1)

	pop(s2)
	pop(s1)
	pop(ra)
	ret
