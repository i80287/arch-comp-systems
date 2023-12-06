.include "console-tools.m"
.include "file-tools.m"
.include "str-tools.m"

.data
.eqv FILE_NAME_SIZE 260
input_file_name:  .space FILE_NAME_SIZE
output_file_name: .space FILE_NAME_SIZE

# FREAD_BUFFER_MAX_SIZE = 4 * READ_BUFFER_SIZE because
# in worst case each char '8' will be replaced with 4 chars "VIII"
.eqv TEXT_BUFFER_SIZE 4096
.eqv TEXT_BUFFER_MAX_SIZE 16384
text_buffer:      .space TEXT_BUFFER_MAX_SIZE

.text
.globl main
main:
main_prologue:
	push(s1)      # Save s1 on top of the stack
	push(s2)      # Save s2 on top of the stack

	puts("Starting internal tests...\n")
	jal functions_tests
	jal macros_tests

	confirm_literal_msg_rwnd("Would you like program to print logs?")
	# a0 = 0 <=> answer is Yes
	seqz s2 a0    # s2 := a0 == 0

main_loop:
	# Read input file name:
	la t0 input_file_name
	li t1 FILE_NAME_SIZE
	read_string_rwnd("Input file name:", t0, t1)

	# Read text from the file
	li a0 TEXT_BUFFER_SIZE
	fread(input_file_name, text_buffer, a0)

	# if a0 = -1, an error has occured
	bgez a0 no_fread_failure

	beqz s2 no_print_label_1
	write_literal_error_rwnd("An error occured while reading the file")
no_print_label_1:
	b main_loop_ans

no_fread_failure:
	mv s1 a0

	beqz s2 no_print_label_2
	write_literal_string_rwnd("Read file successfully")
no_print_label_2:

	str_replace_digits(text_buffer, s1)
	mv s1 a0

	beqz s2 no_print_label_3
	write_literal_string_rwnd("Processed text")
no_print_label_3:

	# Read output file name:
	la t0 output_file_name
	li t1 FILE_NAME_SIZE
	read_string_rwnd("Output file name:", t0, t1)

	fwrite(output_file_name, text_buffer, s1)

	beqz s1 no_print_label_4
	bltz a0 fwrite_error
	write_literal_string_rwnd("Saved processed text to the file")
	b no_print_label_4
fwrite_error:
	write_literal_string_rwnd("Was not able to save processed text to the file")
no_print_label_4:

main_loop_ans:
	confirm_literal_msg_rwnd("Would you like to input another file?")
	# a0 = 0 <=> answer is Yes
	beqz a0 main_loop

main_epilogue:
	pop(s2)       # Restore s2 from the stack
	pop(s1)       # Restore s1 from the stack
	exit()        # Exit with default code 0
