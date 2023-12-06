.include "stack_utils.m"

.eqv READ_CHUNK_SIZE 512

##########################################################
# This function will open, read to buffer and close file #
#	whose name is passed in the a0. Buffer is passed in   #
#  the a1, size of the buffer is passed in the a2.       #
#                                                        #
# Input:                                                 #
#  a0 - address of the ascii string with the name of     #
#       the file                                         #
#                                                        #
#  a1 - address of the buffer                            #
#                                                        #
#  a2 - size of the buffer                               #
#                                                        #
# Return:                                                #
#  a0 = -1 if any error occured and read size otherwise  #
#                                                        #
##########################################################
.text
.globl fread
fread:
fread_prologue:
	push(s1)      # Save s1 on top of the stack
	push(s2)      # Save s2 on top of the stack
	push(s3)      # Save s3 on top of the stack
	push(s4)      # Save s4 on top of the stack
	push(ra)      # Save ra on top of the stack
	mv s1 a1      # Save address of the buffer
	addi s2 a2 -1 # Save (size of the buffer - 1) to the s2
					  # (subtract 1 in order to add '\0' to the end)
	li s3 -1      # Initially s3 = -1, no file descriptor

	li a1 0x0     # Flag to open file in read-only mode
	li a7 1024    # Open file syscall
	ecall
	# Error check: if a0 == -1, an error occured while opening file
	beq a0 s3 fread_epilogue

	mv s3 a0      # Save file descriptor
	li s4 0       # Total chars read = 0
read_loop:
	mv a0 s3      # Move file descriptor to the a0
	add a1 s1 s4  # a1 := buffer address + total chars read = position to write
	li a2 READ_CHUNK_SIZE
	sub t0 s2 s4  # t0 := buffer size - total chars read (max read)
	ble a2 t0 do_no_shrink_read_size
	mv a2 t0
do_no_shrink_read_size:
	li a7	63      # Read syscall
	ecall
	# Now a0 = the length read or -1 if error

	# Error check:
	bltz a0 fread_epilogue

	add s4 s4 a0  # Total chars read += current chars read
	# If current chars read == READ_CHUNK_SIZE 0 && total chars read < buffer size, continue reading
	addi a0 a0 -READ_CHUNK_SIZE
	seqz a0 a0    # a0 := a0 == READ_CHUNK_SIZE
	slt a1 s4 s2  # a1 := s4 < s2 = total chars read < buffer size
	and a0 a0 a1  # a0 := a0 & a1
	# if a0 != 0, continue reading. Otherwise, return with success exit code
	bnez a0 read_loop
	mv a0 s4      # Success exit code equals total chars read
	# Write '\0' to the end of the buffer
	add s1 s1 s4  # Now s1 = address of the buffer + total chars read
	sb zero (s1)
fread_epilogue:
	# If fd in s3 = -1, file was not opened
	bltz s3 file_is_not_opened
	mv s1 a0      # Save exit code in a0 to s1
	mv a0 s3      # Move fd to a0
	li a7 57      # Close file syscall
	ecall
	mv a0 s1      # Restore exit code from s1 to a0
file_is_not_opened:
	pop(ra)       # Restore ra from the stack
	pop(s4)       # Restore s4 from the stack
	pop(s3)       # Restore s3 from the stack
	pop(s2)       # Restore s2 from the stack
	pop(s1)       # Restore s1 from the stack
	ret

##########################################################
# This function will open, write to and close file whose #
#	name is passed in the a0. Buffer is passed in the a1, #
#  size of the buffer is passed in the a2.               #
#                                                        #
# Input:                                                 #
#  a0 - address of the ascii string with the name of     #
#       the file                                         #
#                                                        #
#  a1 - address of the buffer                            #
#                                                        #
#  a2 - size of the buffer                               #
#                                                        #
# Return:                                                #
#  a0 = negative number if any error occured and         #
#       0 otherwise                                      #
#                                                        #
##########################################################
.text
.globl fwrite
fwrite:
fwrite_prologue:
	push(s1)      # Save s1 on top of the stack
	push(s2)      # Save s2 on top of the stack
	push(s3)      # Save s3 on top of the stack
	push(ra)      # Save ra on top of the stack
	mv s1 a1      # Save address of the buffer
	mv s2 a2      # Save size of the buffer
	li s3 -1      # Initially s3 = -1, no file descriptor

	li a1 0x1     # Flag to open file in write-only (write-create) mode
	li a7 1024    # Open file syscall
	ecall

	# Error check: if a0 == -1, an error occured while opening file
	beq a0 s3 fwrite_epilogue
	mv s3 a0

	mv a1 s1      # Move buffer address to the a1
	mv a2 s2      # Move buffer size to the a2
	li	a7 64      # Write to file syscall
	ecall

	sub s2 a0 s2  # s2: write size - buffer size

	mv a0 s3      # Move fd to a0
	li a7 57      # Close file syscall
	ecall

	mv a0 s2      # Exit code in s2
fwrite_epilogue:
	pop(ra)       # Restore ra from the stack
	pop(s3)       # Restore s3 from the stack
	pop(s2)       # Restore s2 from the stack
	pop(s1)       # Restore s1 from the stack
	ret
