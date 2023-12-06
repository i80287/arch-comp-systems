.include "console-tools.m"

.data
# FREAD_BUFFER_MAX_SIZE = 4 * READ_BUFFER_SIZE because
# in worst case each char '8' will be replaced with 4 chars "VIII"
.eqv TEXT_BUFFER_SIZE 4096
.eqv TEXT_BUFFER_MAX_SIZE 16384
func_tests_buffer:   .space TEXT_BUFFER_MAX_SIZE

############################################################################
# This macro runs tests on functions fread, str_replace_digits and fwrite  #
#                                                                          #
# %fname_in_str - literal string with input file name                      #
#                                                                          #
# %fname_out_str - literal string with output file name                    #
#                                                                          #
# %test_number_reg - register with counter of the tests                    #
#                    (will be increased by 1 after the end of this macro)  #
#                                                                          #
# %expected_length_imm - integer literal with expected string length after #
#                        all replacements                                  #
#                                                                          #
############################################################################
.macro run_function_test(%fname_in_str, %fname_out_str, %test_number_reg, %expected_length_imm)
.data
	.align 2
	fname_in_flabel: .asciz %fname_in_str
	.align 2
	fname_out_flabel: .asciz %fname_out_str
.text
	la a0	fname_in_flabel
	la a1 func_tests_buffer
	li a2 TEXT_BUFFER_SIZE
	push_canary(t0)
	jal fread
	pop_canary(t0)
	bnez t0 ftest_error_occured   # Stack access error
	bltz a0 ftest_error_occured   # ? a0 >= 0

	mv s1 a0                      # Save string length

	mv a1 a0
	la a0 func_tests_buffer
	push_canary(t0)
	jal str_replace_digits
	pop_canary(t0)
	bnez t0 ftest_error_occured   # Stack access error
	blt a0 s1 ftest_error_occured # ? Length after replacement >= length before replacement
	li a1 %expected_length_imm
	bne a0 a1 ftest_error_occured # ? Length after replacement != expected length (test precomputed constant)

	mv a2 a0
	la a0 fname_out_flabel
	la a1 func_tests_buffer
	push_canary(t0)
	jal fwrite
	pop_canary(t0)
	bnez t0 ftest_error_occured   # Stack access error
	bltz a0 ftest_error_occured   # ? a0 >= 0	

	puts("Functions test ")
	print_int(%test_number_reg)
	puts(" passed.\n")
	b ftest_macro_epilogue
ftest_error_occured:
	puts("Functions test ")
	print_int(%test_number_reg)
	puts(" not passed.\n")
ftest_macro_epilogue:
	addi %test_number_reg %test_number_reg 1
.end_macro

.global functions_tests
.text
functions_tests:
	push(ra) # Save ra on top of the stack
	push(s1) # Save s1 on top of the stack
	push(s2) # Save s2 on top of the stack

	li s2 1 # Counter of the running tests
	run_function_test("test_files/t1.in", "test_files/t1.out", s2, 105)
	run_function_test("test_files/t2.in", "test_files/t2.out", s2, 4706)
	run_function_test("test_files/t3.in", "test_files/t3.out", s2, 1438)
	run_function_test("test_files/t4.in", "test_files/t4.out", s2, 4095)
	run_function_test("test_files/t5.in", "test_files/t5.out", s2, 4095)
	run_function_test("test_files/t6.in", "test_files/t6.out", s2, 4684)
	run_function_test("test_files/t7.in", "test_files/t7.out", s2, 8494)
	run_function_test("test_files/t8.in", "test_files/t8.out", s2, 16380)

	pop(s2) # Restore s2 from the stack
	pop(s1) # Restore s1 from the stack
	pop(ra) # Restore ra from the stack
	ret
