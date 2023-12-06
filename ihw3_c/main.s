.include "io_utils.m"
.include "fio_utils.m"
.include "string_utils.m"

.data
.eqv FILENAME_BUF_SIZE 512
input_file_name:
	.space FILENAME_BUF_SIZE
output_file_name:
	.space FILENAME_BUF_SIZE

.eqv BUF_SIZE 4096
string_buffer:
	.space BUF_SIZE

.text
.globl main
main:
	push(s1)
	push(s2)

	print_string("Running tests\n")
	jal tests
	jal macro_tests

	confirm_literal_msg_rwnd("Would you like program to print logs?")
	# a0 == 0 <=> answer is Yes
	seqz s2 a0

main_loop:
	# Read input file name:
	la t0 input_file_name
	li t1 FILENAME_BUF_SIZE
	read_string_rwnd("Input file name:", t0, t1)

	# Read text from the file
	li a0 BUF_SIZE
	fread(input_file_name, string_buffer, a0)

	# if a0 = -1, an error has occured
	bgez a0 no_fread_error

	beqz s2 no_print_1
	write_literal_error_rwnd("An error occured while reading the file")
no_print_1:
	b main_loop_ans

no_fread_error:
	mv s1 a0

	beqz s2 no_print_2
	write_literal_string_rwnd("Read file successfully")
no_print_2:

	string_swap_words_chars(string_buffer, s1)

	beqz s2 no_print_3
	write_literal_string_rwnd("Processed text")
no_print_3:

	# Read output file name:
	la t0 output_file_name
	li t1 FILENAME_BUF_SIZE
	read_string_rwnd("Output file name:", t0, t1)

	fwrite(output_file_name, string_buffer, s1)

	beqz s2 no_print_4
	bltz a0 fwrite_error
	write_literal_string_rwnd("Saved processed text to the file")
	b no_print_4
fwrite_error:
	write_literal_string_rwnd("Was not able to save processed text to the file")
no_print_4:

main_loop_ans:
	confirm_literal_msg_rwnd("Would you like to input another file?")
	# a0 = 0 <=> answer is Yes
	beqz a0 main_loop

main_epilogue:
	pop(s2)
	pop(s1)
	exit()
