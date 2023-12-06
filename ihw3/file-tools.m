.macro fread(%file_name_label, %buffer_address_label, %buffer_size_reg)
	mv a2 %buffer_size_reg
	la a0 %file_name_label
	la a1 %buffer_address_label
	jal fread
.end_macro

.macro fwrite(%file_name_label, %buffer_address_label, %buffer_size_reg)
	mv a2 %buffer_size_reg
	la a0 %file_name_label
	la a1 %buffer_address_label
	jal fwrite
.end_macro
