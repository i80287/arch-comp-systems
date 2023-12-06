.include "array_tools.i"

# Heap sort algorithm realisation.
#
# Takes pointer to (address of) first element of the array in a0
# (in the comments array / pointer is called 'a')
# Takes size of the array in a1
# (in the comments length is called 'n')
# Returns: N/A
#
# After the end of the function for all i, j:
# (0 <= i < j <= n - 1) -> (a[i] <= a[j])
.global heap_sort
	.text
heap_sort:
	push(ra) # Let's follow convention

	li t0 1
	bleu a1 t0 heap_sort_end # if n <= 1, then a is already sorted.

	# Heapify
	# Make a max-heap out of a given array
heapify_for_init:
	srli t1 a1 1            # t1: i = n / 2
heapify_for_iter:
	beqz t1 heapify_for_end # if (i == 0) -> end heapify_for cycle
	addi t1 t1 -1
	mv t2 t1                # t2: parent_index = i
	read_at(a0, t2, t3)     # t3: current_elem = a[parent_index]
	slli t4 t2 1				# t4: son_index = parent_index * 2
	ori t4 t4 1             # son_index = son_index + 1
	read_at(a0, t4, t5)     # t5: son = a[son_index]

	# if (son_index + 1 < n && son < a[son_index + 1])
	addi t6 t4 1            # t6: son_index + 1
	bgeu t6 a1 dont_change_son_1 # if (son_index + 1 >= n)
	read_at(a0, t6, t0)     # t0: a[son_index + 1]
	bge t5 t0 dont_change_son_1  # if (son >= a[son_index + 1])
	mv t5 t0                # son = a[son_index + 1]
	mv t4 t6                # son_index = son_index + 1
dont_change_son_1:

sieve_current_elem_while_cycle:
	bge t3 t5 heapify_for_iter # if (current_elem >= son) -> end sieve_current_elem_while_cycle, go to heapify_for_iter

	write_at(a0, t2, t5)    # a[parent_index] = son
	write_at(a0, t4, t3)    # a[son_index] = current_elem

	mv t2 t4                # parent_index = son_index
	slli t4 t2 1				# son_index = parent_index * 2
	ori t4 t4 1             # son_index = son_index + 1
	bgeu t4 a1 heapify_for_iter # if (son_index >= n) -> end sieve_current_elem_while_cycle

	read_at(a0, t4, t5)     # son = a[son_index]

	# if (son_index + 1 < n && son < a[son_index + 1])
	addi t6 t4 1            # t6: son_index + 1
	bgeu t6 a1 sieve_current_elem_while_cycle # if (son_index + 1 >= n) -> continue sieve_current_elem_while_cycle
	read_at(a0, t6, t0)     # t0: a[son_index + 1]
	bge t5 t0 sieve_current_elem_while_cycle # if (son >= a[son_index + 1]) -> continue sieve_current_elem_while_cycle
	mv t5 t0                # son = a[son_index + 1]
	mv t4 t6                # son_index = son_index + 1
	j sieve_current_elem_while_cycle
sieve_current_elem_while_cycle_end:
heapify_for_end:

heap_sort_for_init:
	mv t1 a1 				   # t1: i = n
heap_sort_for_iter:
	li t2 1
	sub t1 t1 t2            # i = i - 1
	beq t1 t2 heap_sort_for_end # if (i == 1) -> end for cycle

	# Pop max elem from the top of the pyramid (heap) and add to the end.
	read_at(a0, t1, t2)     # t2: sifting_elem = a[i]
	lw t3 (a0)              # t3: a[0]
	write_at(a0, t1, t3)    # a[i] = a[0]
	sw t2 (a0)              # a[0] = sifting_elem

	# Back pyramide (heap) to the balance state.
	mv t3 zero              # t3: parent_index = 0
	li t4 1					   # t4: son_index = 1
	lw t5 4(a0)				   # t5: son = a[1]

	# if (i > 2 && son < arr[2]) { son = arr[2]; son_index = 2; }
	li t0 2
	bleu t1 t0 dont_change_son_2 # if (i <= 2)
	lw t6 8(a0)             # t6: a[2]
	bge t5 t6 dont_change_son_2  # if (son >= a[2])
	li t4 2                 # son_index = 2
	mv t5 t6                # son = a[2]
dont_change_son_2:

restore_heap_while_cycle:
	bge t2 t5 heap_sort_for_iter # if (sifting_elem >= son) -> end restore_heap_while_cycle, go to heap_sort_for_iter

	write_at(a0, t3, t5)    # a[parent_index] = son
	write_at(a0, t4, t2)    # a[son_index] = sifting_elem
	mv t3 t4 				   # parent_index = son_index
	slli t4 t3 1            # son_index = parent_index * 2
	ori t4 t4 1					# son_index = son_index + 1
	bgeu t4 t1 heap_sort_for_iter # if (son_index >= i) -> end restore_heap_while_cycle, go to heap_sort_for_iter
	read_at(a0, t4, t5) 		# son = a[son_index]

	# 	if (son_index + 1 != i && son < arr[son_index + 1]) { son_index += 1; son = arr[son_index]; }
	addi t6 t4 1            # t6: son_index + 1
	beq t6 t1 restore_heap_while_cycle
	read_at(a0, t6, t0)     # t0: a[son_index + 1]
	bge t5 t0 restore_heap_while_cycle
	mv t4 t6						# son_index = son_index + 1
	mv t5 t0                # son = a[son_index]
	j restore_heap_while_cycle
restore_heap_while_cycle_end:
	
heap_sort_for_end:

	# swap(a[0], a[1])
	lw t0 (a0)  # t0: a[0]
	lw t1 4(a0) # t1: a[1]
	sw t1 (a0)  # a[0] = t1
	sw t0 4(a0)	# a[1] = t0
heap_sort_end:
	pop(ra)
	ret
