.include "stack_utils.m"

# Swap chars in all words, where word is contiguous sequence
#  of chars in set ['a'; 'z'] U ['A'; 'Z']
# a0 - address of the null-terminated ascii string
# a1 - length of the string
.text
.globl string_swap_words_chars
string_swap_words_chars:
	push(ra)
	push(s1)
	push(s2)
	push(s3)

	add s1 a0 a1       # s1 - pointer to the last char + 1
	mv s2 a0

swwc_loop:
	mv a0 s2
	jal find_next_alpha

	mv s2 a0
	# if string has no alphabetic symbols left, exit
	bge s2 s1 swwc_epilogue

	addi a0 s2 1       # find next non alpha starting from the next char
	jal find_next_non_alpha
	mv s3 a0

	mv a0 s2
	addi a1 s3 -1
	jal swap_string    # swap all chars in substring [s2; s3 - 1]
	addi s2 s3 1       # move starting pointer
	blt s2 s1 swwc_loop

swwc_epilogue:
	pop(s3)
	pop(s2)
	pop(s1)
	pop(ra)
	ret

.macro isalpha(%reg)
	addi t0 %reg -97
	sltiu t0 t0 26
	addi t1 %reg -65
	sltiu t1 t1 26
	or t0 t0 t1
.end_macro

find_next_non_alpha:
	addi a0 a0 -1
fnna_loop:
	addi a0 a0 1
	lbu t2 (a0)
	isalpha(t2)
	bnez t0 fnna_loop
	ret

find_next_alpha:
	addi a0 a0 -1
fna_loop:
	addi a0 a0 1
	lbu t2 (a0)
	beqz t2 fna_return
	isalpha(t2)
	beqz t0 fna_loop
fna_return:
	ret

swap_string:
sst_loop:
	lbu t0 (a0)
	lbu t1 (a1)
	sb t0 (a1)
	sb t1 (a0)
	addi a0 a0 1
	addi a1 a1 -1
	blt a0 a1 sst_loop
	ret
