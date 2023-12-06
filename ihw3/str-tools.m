.macro str_replace_digits(%str_label, %str_len_reg)
	mv a1 %str_len_reg
	la a0 %str_label
	jal str_replace_digits
.end_macro
