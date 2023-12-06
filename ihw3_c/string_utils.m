.macro string_swap_words_chars(%string_label, %string_size)
	mv a1 %string_size
	la a0 %string_label
	jal string_swap_words_chars
.end_macro
