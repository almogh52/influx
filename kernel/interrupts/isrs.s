%include "macros.s"

extern isr_handler

%macro create_isr 1
global isr_%1
isr_%1:
	push %1

	jmp common_isr
%endmacro

%macro create_isr_with_dummy_error 1
global isr_%1
isr_%1:
	push 0
	push %1

	jmp common_isr
%endmacro

create_isr_with_dummy_error 0
create_isr_with_dummy_error 1
create_isr_with_dummy_error 2
create_isr_with_dummy_error 3
create_isr_with_dummy_error 4
create_isr_with_dummy_error 5
create_isr_with_dummy_error 6
create_isr_with_dummy_error 7
create_isr 8
create_isr_with_dummy_error 9
create_isr 10
create_isr 11
create_isr 12
create_isr 13
create_isr 14
create_isr_with_dummy_error 15
create_isr_with_dummy_error 16
create_isr_with_dummy_error 17
create_isr_with_dummy_error 18
create_isr_with_dummy_error 19
create_isr_with_dummy_error 20
create_isr_with_dummy_error 21
create_isr_with_dummy_error 22
create_isr_with_dummy_error 23
create_isr_with_dummy_error 24
create_isr_with_dummy_error 25
create_isr_with_dummy_error 26
create_isr_with_dummy_error 27
create_isr_with_dummy_error 28
create_isr_with_dummy_error 29
create_isr_with_dummy_error 30
create_isr_with_dummy_error 31
create_isr_with_dummy_error 32
create_isr_with_dummy_error 33
create_isr_with_dummy_error 34
create_isr_with_dummy_error 35
create_isr_with_dummy_error 36
create_isr_with_dummy_error 37
create_isr_with_dummy_error 38
create_isr_with_dummy_error 39
create_isr_with_dummy_error 40
create_isr_with_dummy_error 41
create_isr_with_dummy_error 42
create_isr_with_dummy_error 43
create_isr_with_dummy_error 44
create_isr_with_dummy_error 45
create_isr_with_dummy_error 46
create_isr_with_dummy_error 47
create_isr_with_dummy_error 48
create_isr_with_dummy_error 49
create_isr_with_dummy_error 50
create_isr_with_dummy_error 51
create_isr_with_dummy_error 52
create_isr_with_dummy_error 53
create_isr_with_dummy_error 54
create_isr_with_dummy_error 55
create_isr_with_dummy_error 56
create_isr_with_dummy_error 57
create_isr_with_dummy_error 58
create_isr_with_dummy_error 59
create_isr_with_dummy_error 60
create_isr_with_dummy_error 61
create_isr_with_dummy_error 62
create_isr_with_dummy_error 63
create_isr_with_dummy_error 64
create_isr_with_dummy_error 65
create_isr_with_dummy_error 66
create_isr_with_dummy_error 67
create_isr_with_dummy_error 68
create_isr_with_dummy_error 69
create_isr_with_dummy_error 70
create_isr_with_dummy_error 71
create_isr_with_dummy_error 72
create_isr_with_dummy_error 73
create_isr_with_dummy_error 74
create_isr_with_dummy_error 75
create_isr_with_dummy_error 76
create_isr_with_dummy_error 77
create_isr_with_dummy_error 78
create_isr_with_dummy_error 79
create_isr_with_dummy_error 80
create_isr_with_dummy_error 81
create_isr_with_dummy_error 82
create_isr_with_dummy_error 83
create_isr_with_dummy_error 84
create_isr_with_dummy_error 85
create_isr_with_dummy_error 86
create_isr_with_dummy_error 87
create_isr_with_dummy_error 88
create_isr_with_dummy_error 89
create_isr_with_dummy_error 90
create_isr_with_dummy_error 91
create_isr_with_dummy_error 92
create_isr_with_dummy_error 93
create_isr_with_dummy_error 94
create_isr_with_dummy_error 95
create_isr_with_dummy_error 96
create_isr_with_dummy_error 97
create_isr_with_dummy_error 98
create_isr_with_dummy_error 99
create_isr_with_dummy_error 100
create_isr_with_dummy_error 101
create_isr_with_dummy_error 102
create_isr_with_dummy_error 103
create_isr_with_dummy_error 104
create_isr_with_dummy_error 105
create_isr_with_dummy_error 106
create_isr_with_dummy_error 107
create_isr_with_dummy_error 108
create_isr_with_dummy_error 109
create_isr_with_dummy_error 110
create_isr_with_dummy_error 111
create_isr_with_dummy_error 112
create_isr_with_dummy_error 113
create_isr_with_dummy_error 114
create_isr_with_dummy_error 115
create_isr_with_dummy_error 116
create_isr_with_dummy_error 117
create_isr_with_dummy_error 118
create_isr_with_dummy_error 119
create_isr_with_dummy_error 120
create_isr_with_dummy_error 121
create_isr_with_dummy_error 122
create_isr_with_dummy_error 123
create_isr_with_dummy_error 124
create_isr_with_dummy_error 125
create_isr_with_dummy_error 126
create_isr_with_dummy_error 127
create_isr_with_dummy_error 128
create_isr_with_dummy_error 129
create_isr_with_dummy_error 130
create_isr_with_dummy_error 131
create_isr_with_dummy_error 132
create_isr_with_dummy_error 133
create_isr_with_dummy_error 134
create_isr_with_dummy_error 135
create_isr_with_dummy_error 136
create_isr_with_dummy_error 137
create_isr_with_dummy_error 138
create_isr_with_dummy_error 139
create_isr_with_dummy_error 140
create_isr_with_dummy_error 141
create_isr_with_dummy_error 142
create_isr_with_dummy_error 143
create_isr_with_dummy_error 144
create_isr_with_dummy_error 145
create_isr_with_dummy_error 146
create_isr_with_dummy_error 147
create_isr_with_dummy_error 148
create_isr_with_dummy_error 149
create_isr_with_dummy_error 150
create_isr_with_dummy_error 151
create_isr_with_dummy_error 152
create_isr_with_dummy_error 153
create_isr_with_dummy_error 154
create_isr_with_dummy_error 155
create_isr_with_dummy_error 156
create_isr_with_dummy_error 157
create_isr_with_dummy_error 158
create_isr_with_dummy_error 159
create_isr_with_dummy_error 160
create_isr_with_dummy_error 161
create_isr_with_dummy_error 162
create_isr_with_dummy_error 163
create_isr_with_dummy_error 164
create_isr_with_dummy_error 165
create_isr_with_dummy_error 166
create_isr_with_dummy_error 167
create_isr_with_dummy_error 168
create_isr_with_dummy_error 169
create_isr_with_dummy_error 170
create_isr_with_dummy_error 171
create_isr_with_dummy_error 172
create_isr_with_dummy_error 173
create_isr_with_dummy_error 174
create_isr_with_dummy_error 175
create_isr_with_dummy_error 176
create_isr_with_dummy_error 177
create_isr_with_dummy_error 178
create_isr_with_dummy_error 179
create_isr_with_dummy_error 180
create_isr_with_dummy_error 181
create_isr_with_dummy_error 182
create_isr_with_dummy_error 183
create_isr_with_dummy_error 184
create_isr_with_dummy_error 185
create_isr_with_dummy_error 186
create_isr_with_dummy_error 187
create_isr_with_dummy_error 188
create_isr_with_dummy_error 189
create_isr_with_dummy_error 190
create_isr_with_dummy_error 191
create_isr_with_dummy_error 192
create_isr_with_dummy_error 193
create_isr_with_dummy_error 194
create_isr_with_dummy_error 195
create_isr_with_dummy_error 196
create_isr_with_dummy_error 197
create_isr_with_dummy_error 198
create_isr_with_dummy_error 199
create_isr_with_dummy_error 200
create_isr_with_dummy_error 201
create_isr_with_dummy_error 202
create_isr_with_dummy_error 203
create_isr_with_dummy_error 204
create_isr_with_dummy_error 205
create_isr_with_dummy_error 206
create_isr_with_dummy_error 207
create_isr_with_dummy_error 208
create_isr_with_dummy_error 209
create_isr_with_dummy_error 210
create_isr_with_dummy_error 211
create_isr_with_dummy_error 212
create_isr_with_dummy_error 213
create_isr_with_dummy_error 214
create_isr_with_dummy_error 215
create_isr_with_dummy_error 216
create_isr_with_dummy_error 217
create_isr_with_dummy_error 218
create_isr_with_dummy_error 219
create_isr_with_dummy_error 220
create_isr_with_dummy_error 221
create_isr_with_dummy_error 222
create_isr_with_dummy_error 223
create_isr_with_dummy_error 224
create_isr_with_dummy_error 225
create_isr_with_dummy_error 226
create_isr_with_dummy_error 227
create_isr_with_dummy_error 228
create_isr_with_dummy_error 229
create_isr_with_dummy_error 230
create_isr_with_dummy_error 231
create_isr_with_dummy_error 232
create_isr_with_dummy_error 233
create_isr_with_dummy_error 234
create_isr_with_dummy_error 235
create_isr_with_dummy_error 236
create_isr_with_dummy_error 237
create_isr_with_dummy_error 238
create_isr_with_dummy_error 239
create_isr_with_dummy_error 240
create_isr_with_dummy_error 241
create_isr_with_dummy_error 242
create_isr_with_dummy_error 243
create_isr_with_dummy_error 244
create_isr_with_dummy_error 245
create_isr_with_dummy_error 246
create_isr_with_dummy_error 247
create_isr_with_dummy_error 248
create_isr_with_dummy_error 249
create_isr_with_dummy_error 250
create_isr_with_dummy_error 251
create_isr_with_dummy_error 252
create_isr_with_dummy_error 253
create_isr_with_dummy_error 254
create_isr_with_dummy_error 255

global common_isr
common_isr:
;	Save the current context
	save_context

;	Call the ISR handler in the interrupt manager
	mov rdi, rsp
	call isr_handler

;	Restore the context
	restore_context

;	Clean error code and interrupt number
	add rsp, 16

	iretq