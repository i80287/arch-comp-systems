.include "console-tools.m"
.include "macro-string.m"

.data
str1: .asciz "" # empty string test
str2: .asciz "a"
str3: .asciz "abc"
str4: .asciz "Abcdef"
str5: .asciz "123bn3bn"
str6: .asciz "asdnadbnadbnm321314"
str7: .asciz "DeadBeef0x001"
str8: .asciz "abcdefghijklmnopqrstuvwxyz"
str9: .asciz "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
str10: .asciz "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
str11: .asciz "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ"
str12: .asciz "0123456789101112131415161718192021222324252627282930313233343536373839404142"
.align 2
.eqv COPY_BUF_SIZE 128
copy_buffer: .space COPY_BUF_SIZE

.macro run_test(%str_label, %test_number_reg)
	push(s1)

	strcpy(copy_buffer, %str_label)
	la s1 copy_buffer
	sub s1 s1 a0
	# s1 is set to 1 if return value of strcpy == address of copy_buffer. Otherwise, s1 is set to 0
	seqz s1 s1

	strcmp(copy_buffer, %str_label)

	# a0 is set to 1 if string in copy_buffer == string in %str_label. Otherwise, it a0 set to 0
	seqz a0 a0
	# s1 will be 1 if and only if strcpy returned correct address and string in copy_buffer == string in %str_label
	and s1 s1 a0

	puts("Test ")
	print_int(%test_number_reg)
	bnez s1 test_passed
	puts(" not")
test_passed:
	puts(" passed.\n")
	addi %test_number_reg %test_number_reg 1
	pop(s1)
.end_macro

.globl run_strcpy_tests
.text
run_strcpy_tests:
	push(ra) # save ra to the stack
	push(s3) # save s3 to the stack

	li s3 1 # counter of the running tests
	run_test(str1, s3)
	run_test(str2, s3)
	run_test(str3, s3)
	run_test(str4, s3)
	run_test(str5, s3)
	run_test(str6, s3)
	run_test(str7, s3)
	run_test(str8, s3)
	run_test(str9, s3)
	run_test(str10, s3)
	run_test(str11, s3)
	run_test(str12, s3)

	pop(s3) # restore s3 from the stack
	pop(ra) # restore ra from the stack
	ret
