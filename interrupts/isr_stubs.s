# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    isr_stubs.s                                        :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: thrieg <thrieg@student.42mulhouse.fr>      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/12 01:07:21 by thrieg            #+#    #+#              #
#    Updated: 2026/01/12 01:22:07 by thrieg           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

.text
    .intel_syntax noprefix
    .extern isr_common_stub

.global isr_stub_0
isr_stub_0:
    push 0            # fake err_code
    push 0        # int_no
    jmp isr_common_stub

.global isr_stub_1
isr_stub_1:
    push 0            # fake err_code
    push 1        # int_no
    jmp isr_common_stub

.global isr_stub_2
isr_stub_2:
    push 0            # fake err_code
    push 2        # int_no
    jmp isr_common_stub

.global isr_stub_3
isr_stub_3:
    push 0            # fake err_code
    push 3        # int_no
    jmp isr_common_stub

.global isr_stub_4
isr_stub_4:
    push 0            # fake err_code
    push 4        # int_no
    jmp isr_common_stub

.global isr_stub_5
isr_stub_5:
    push 0            # fake err_code
    push 5        # int_no
    jmp isr_common_stub

.global isr_stub_6
isr_stub_6:
    push 0            # fake err_code
    push 6        # int_no
    jmp isr_common_stub

.global isr_stub_7
isr_stub_7:
    push 0            # fake err_code
    push 7        # int_no
    jmp isr_common_stub

.global isr_stub_8
isr_stub_8:
    push 8        # int_no
    jmp isr_common_stub

.global isr_stub_9
isr_stub_9:
    push 0            # fake err_code
    push 9        # int_no
    jmp isr_common_stub

.global isr_stub_10
isr_stub_10:
    push 10        # int_no
    jmp isr_common_stub

.global isr_stub_11
isr_stub_11:
    push 11        # int_no
    jmp isr_common_stub

.global isr_stub_12
isr_stub_12:
    push 12        # int_no
    jmp isr_common_stub

.global isr_stub_13
isr_stub_13:
    push 13        # int_no
    jmp isr_common_stub

.global isr_stub_14
isr_stub_14:
    push 14        # int_no
    jmp isr_common_stub

.global isr_stub_15
isr_stub_15:
    push 0            # fake err_code
    push 15        # int_no
    jmp isr_common_stub

.global isr_stub_16
isr_stub_16:
    push 0            # fake err_code
    push 16        # int_no
    jmp isr_common_stub

.global isr_stub_17
isr_stub_17:
    push 17        # int_no
    jmp isr_common_stub

.global isr_stub_18
isr_stub_18:
    push 0            # fake err_code
    push 18        # int_no
    jmp isr_common_stub

.global isr_stub_19
isr_stub_19:
    push 0            # fake err_code
    push 19        # int_no
    jmp isr_common_stub

.global isr_stub_20
isr_stub_20:
    push 0            # fake err_code
    push 20        # int_no
    jmp isr_common_stub

.global isr_stub_21
isr_stub_21:
    push 0            # fake err_code
    push 21        # int_no
    jmp isr_common_stub

.global isr_stub_22
isr_stub_22:
    push 0            # fake err_code
    push 22        # int_no
    jmp isr_common_stub

.global isr_stub_23
isr_stub_23:
    push 0            # fake err_code
    push 23        # int_no
    jmp isr_common_stub

.global isr_stub_24
isr_stub_24:
    push 0            # fake err_code
    push 24        # int_no
    jmp isr_common_stub

.global isr_stub_25
isr_stub_25:
    push 0            # fake err_code
    push 25        # int_no
    jmp isr_common_stub

.global isr_stub_26
isr_stub_26:
    push 0            # fake err_code
    push 26        # int_no
    jmp isr_common_stub

.global isr_stub_27
isr_stub_27:
    push 0            # fake err_code
    push 27        # int_no
    jmp isr_common_stub

.global isr_stub_28
isr_stub_28:
    push 0            # fake err_code
    push 28        # int_no
    jmp isr_common_stub

.global isr_stub_29
isr_stub_29:
    push 0            # fake err_code
    push 29        # int_no
    jmp isr_common_stub

.global isr_stub_30
isr_stub_30:
    push 0            # fake err_code
    push 30        # int_no
    jmp isr_common_stub

.global isr_stub_31
isr_stub_31:
    push 0            # fake err_code
    push 31        # int_no
    jmp isr_common_stub

.global isr_stub_32
isr_stub_32:
    push 0            # fake err_code
    push 32        # int_no
    jmp isr_common_stub

.global isr_stub_33
isr_stub_33:
    push 0            # fake err_code
    push 33        # int_no
    jmp isr_common_stub

.global isr_stub_34
isr_stub_34:
    push 0            # fake err_code
    push 34        # int_no
    jmp isr_common_stub

.global isr_stub_35
isr_stub_35:
    push 0            # fake err_code
    push 35        # int_no
    jmp isr_common_stub

.global isr_stub_36
isr_stub_36:
    push 0            # fake err_code
    push 36        # int_no
    jmp isr_common_stub

.global isr_stub_37
isr_stub_37:
    push 0            # fake err_code
    push 37        # int_no
    jmp isr_common_stub

.global isr_stub_38
isr_stub_38:
    push 0            # fake err_code
    push 38        # int_no
    jmp isr_common_stub

.global isr_stub_39
isr_stub_39:
    push 0            # fake err_code
    push 39        # int_no
    jmp isr_common_stub

.global isr_stub_40
isr_stub_40:
    push 0            # fake err_code
    push 40        # int_no
    jmp isr_common_stub

.global isr_stub_41
isr_stub_41:
    push 0            # fake err_code
    push 41        # int_no
    jmp isr_common_stub

.global isr_stub_42
isr_stub_42:
    push 0            # fake err_code
    push 42        # int_no
    jmp isr_common_stub

.global isr_stub_43
isr_stub_43:
    push 0            # fake err_code
    push 43        # int_no
    jmp isr_common_stub

.global isr_stub_44
isr_stub_44:
    push 0            # fake err_code
    push 44        # int_no
    jmp isr_common_stub

.global isr_stub_45
isr_stub_45:
    push 0            # fake err_code
    push 45        # int_no
    jmp isr_common_stub

.global isr_stub_46
isr_stub_46:
    push 0            # fake err_code
    push 46        # int_no
    jmp isr_common_stub

.global isr_stub_47
isr_stub_47:
    push 0            # fake err_code
    push 47        # int_no
    jmp isr_common_stub

.global isr_stub_48
isr_stub_48:
    push 0            # fake err_code
    push 48        # int_no
    jmp isr_common_stub

.global isr_stub_49
isr_stub_49:
    push 0            # fake err_code
    push 49        # int_no
    jmp isr_common_stub

.global isr_stub_50
isr_stub_50:
    push 0            # fake err_code
    push 50        # int_no
    jmp isr_common_stub

.global isr_stub_51
isr_stub_51:
    push 0            # fake err_code
    push 51        # int_no
    jmp isr_common_stub

.global isr_stub_52
isr_stub_52:
    push 0            # fake err_code
    push 52        # int_no
    jmp isr_common_stub

.global isr_stub_53
isr_stub_53:
    push 0            # fake err_code
    push 53        # int_no
    jmp isr_common_stub

.global isr_stub_54
isr_stub_54:
    push 0            # fake err_code
    push 54        # int_no
    jmp isr_common_stub

.global isr_stub_55
isr_stub_55:
    push 0            # fake err_code
    push 55        # int_no
    jmp isr_common_stub

.global isr_stub_56
isr_stub_56:
    push 0            # fake err_code
    push 56        # int_no
    jmp isr_common_stub

.global isr_stub_57
isr_stub_57:
    push 0            # fake err_code
    push 57        # int_no
    jmp isr_common_stub

.global isr_stub_58
isr_stub_58:
    push 0            # fake err_code
    push 58        # int_no
    jmp isr_common_stub

.global isr_stub_59
isr_stub_59:
    push 0            # fake err_code
    push 59        # int_no
    jmp isr_common_stub

.global isr_stub_60
isr_stub_60:
    push 0            # fake err_code
    push 60        # int_no
    jmp isr_common_stub

.global isr_stub_61
isr_stub_61:
    push 0            # fake err_code
    push 61        # int_no
    jmp isr_common_stub

.global isr_stub_62
isr_stub_62:
    push 0            # fake err_code
    push 62        # int_no
    jmp isr_common_stub

.global isr_stub_63
isr_stub_63:
    push 0            # fake err_code
    push 63        # int_no
    jmp isr_common_stub

.global isr_stub_64
isr_stub_64:
    push 0            # fake err_code
    push 64        # int_no
    jmp isr_common_stub

.global isr_stub_65
isr_stub_65:
    push 0            # fake err_code
    push 65        # int_no
    jmp isr_common_stub

.global isr_stub_66
isr_stub_66:
    push 0            # fake err_code
    push 66        # int_no
    jmp isr_common_stub

.global isr_stub_67
isr_stub_67:
    push 0            # fake err_code
    push 67        # int_no
    jmp isr_common_stub

.global isr_stub_68
isr_stub_68:
    push 0            # fake err_code
    push 68        # int_no
    jmp isr_common_stub

.global isr_stub_69
isr_stub_69:
    push 0            # fake err_code
    push 69        # int_no
    jmp isr_common_stub

.global isr_stub_70
isr_stub_70:
    push 0            # fake err_code
    push 70        # int_no
    jmp isr_common_stub

.global isr_stub_71
isr_stub_71:
    push 0            # fake err_code
    push 71        # int_no
    jmp isr_common_stub

.global isr_stub_72
isr_stub_72:
    push 0            # fake err_code
    push 72        # int_no
    jmp isr_common_stub

.global isr_stub_73
isr_stub_73:
    push 0            # fake err_code
    push 73        # int_no
    jmp isr_common_stub

.global isr_stub_74
isr_stub_74:
    push 0            # fake err_code
    push 74        # int_no
    jmp isr_common_stub

.global isr_stub_75
isr_stub_75:
    push 0            # fake err_code
    push 75        # int_no
    jmp isr_common_stub

.global isr_stub_76
isr_stub_76:
    push 0            # fake err_code
    push 76        # int_no
    jmp isr_common_stub

.global isr_stub_77
isr_stub_77:
    push 0            # fake err_code
    push 77        # int_no
    jmp isr_common_stub

.global isr_stub_78
isr_stub_78:
    push 0            # fake err_code
    push 78        # int_no
    jmp isr_common_stub

.global isr_stub_79
isr_stub_79:
    push 0            # fake err_code
    push 79        # int_no
    jmp isr_common_stub

.global isr_stub_80
isr_stub_80:
    push 0            # fake err_code
    push 80        # int_no
    jmp isr_common_stub

.global isr_stub_81
isr_stub_81:
    push 0            # fake err_code
    push 81        # int_no
    jmp isr_common_stub

.global isr_stub_82
isr_stub_82:
    push 0            # fake err_code
    push 82        # int_no
    jmp isr_common_stub

.global isr_stub_83
isr_stub_83:
    push 0            # fake err_code
    push 83        # int_no
    jmp isr_common_stub

.global isr_stub_84
isr_stub_84:
    push 0            # fake err_code
    push 84        # int_no
    jmp isr_common_stub

.global isr_stub_85
isr_stub_85:
    push 0            # fake err_code
    push 85        # int_no
    jmp isr_common_stub

.global isr_stub_86
isr_stub_86:
    push 0            # fake err_code
    push 86        # int_no
    jmp isr_common_stub

.global isr_stub_87
isr_stub_87:
    push 0            # fake err_code
    push 87        # int_no
    jmp isr_common_stub

.global isr_stub_88
isr_stub_88:
    push 0            # fake err_code
    push 88        # int_no
    jmp isr_common_stub

.global isr_stub_89
isr_stub_89:
    push 0            # fake err_code
    push 89        # int_no
    jmp isr_common_stub

.global isr_stub_90
isr_stub_90:
    push 0            # fake err_code
    push 90        # int_no
    jmp isr_common_stub

.global isr_stub_91
isr_stub_91:
    push 0            # fake err_code
    push 91        # int_no
    jmp isr_common_stub

.global isr_stub_92
isr_stub_92:
    push 0            # fake err_code
    push 92        # int_no
    jmp isr_common_stub

.global isr_stub_93
isr_stub_93:
    push 0            # fake err_code
    push 93        # int_no
    jmp isr_common_stub

.global isr_stub_94
isr_stub_94:
    push 0            # fake err_code
    push 94        # int_no
    jmp isr_common_stub

.global isr_stub_95
isr_stub_95:
    push 0            # fake err_code
    push 95        # int_no
    jmp isr_common_stub

.global isr_stub_96
isr_stub_96:
    push 0            # fake err_code
    push 96        # int_no
    jmp isr_common_stub

.global isr_stub_97
isr_stub_97:
    push 0            # fake err_code
    push 97        # int_no
    jmp isr_common_stub

.global isr_stub_98
isr_stub_98:
    push 0            # fake err_code
    push 98        # int_no
    jmp isr_common_stub

.global isr_stub_99
isr_stub_99:
    push 0            # fake err_code
    push 99        # int_no
    jmp isr_common_stub

.global isr_stub_100
isr_stub_100:
    push 0            # fake err_code
    push 100        # int_no
    jmp isr_common_stub

.global isr_stub_101
isr_stub_101:
    push 0            # fake err_code
    push 101        # int_no
    jmp isr_common_stub

.global isr_stub_102
isr_stub_102:
    push 0            # fake err_code
    push 102        # int_no
    jmp isr_common_stub

.global isr_stub_103
isr_stub_103:
    push 0            # fake err_code
    push 103        # int_no
    jmp isr_common_stub

.global isr_stub_104
isr_stub_104:
    push 0            # fake err_code
    push 104        # int_no
    jmp isr_common_stub

.global isr_stub_105
isr_stub_105:
    push 0            # fake err_code
    push 105        # int_no
    jmp isr_common_stub

.global isr_stub_106
isr_stub_106:
    push 0            # fake err_code
    push 106        # int_no
    jmp isr_common_stub

.global isr_stub_107
isr_stub_107:
    push 0            # fake err_code
    push 107        # int_no
    jmp isr_common_stub

.global isr_stub_108
isr_stub_108:
    push 0            # fake err_code
    push 108        # int_no
    jmp isr_common_stub

.global isr_stub_109
isr_stub_109:
    push 0            # fake err_code
    push 109        # int_no
    jmp isr_common_stub

.global isr_stub_110
isr_stub_110:
    push 0            # fake err_code
    push 110        # int_no
    jmp isr_common_stub

.global isr_stub_111
isr_stub_111:
    push 0            # fake err_code
    push 111        # int_no
    jmp isr_common_stub

.global isr_stub_112
isr_stub_112:
    push 0            # fake err_code
    push 112        # int_no
    jmp isr_common_stub

.global isr_stub_113
isr_stub_113:
    push 0            # fake err_code
    push 113        # int_no
    jmp isr_common_stub

.global isr_stub_114
isr_stub_114:
    push 0            # fake err_code
    push 114        # int_no
    jmp isr_common_stub

.global isr_stub_115
isr_stub_115:
    push 0            # fake err_code
    push 115        # int_no
    jmp isr_common_stub

.global isr_stub_116
isr_stub_116:
    push 0            # fake err_code
    push 116        # int_no
    jmp isr_common_stub

.global isr_stub_117
isr_stub_117:
    push 0            # fake err_code
    push 117        # int_no
    jmp isr_common_stub

.global isr_stub_118
isr_stub_118:
    push 0            # fake err_code
    push 118        # int_no
    jmp isr_common_stub

.global isr_stub_119
isr_stub_119:
    push 0            # fake err_code
    push 119        # int_no
    jmp isr_common_stub

.global isr_stub_120
isr_stub_120:
    push 0            # fake err_code
    push 120        # int_no
    jmp isr_common_stub

.global isr_stub_121
isr_stub_121:
    push 0            # fake err_code
    push 121        # int_no
    jmp isr_common_stub

.global isr_stub_122
isr_stub_122:
    push 0            # fake err_code
    push 122        # int_no
    jmp isr_common_stub

.global isr_stub_123
isr_stub_123:
    push 0            # fake err_code
    push 123        # int_no
    jmp isr_common_stub

.global isr_stub_124
isr_stub_124:
    push 0            # fake err_code
    push 124        # int_no
    jmp isr_common_stub

.global isr_stub_125
isr_stub_125:
    push 0            # fake err_code
    push 125        # int_no
    jmp isr_common_stub

.global isr_stub_126
isr_stub_126:
    push 0            # fake err_code
    push 126        # int_no
    jmp isr_common_stub

.global isr_stub_127
isr_stub_127:
    push 0            # fake err_code
    push 127        # int_no
    jmp isr_common_stub

.global isr_stub_128
isr_stub_128:
    push 0            # fake err_code
    push 128        # int_no
    jmp isr_common_stub

.global isr_stub_129
isr_stub_129:
    push 0            # fake err_code
    push 129        # int_no
    jmp isr_common_stub

.global isr_stub_130
isr_stub_130:
    push 0            # fake err_code
    push 130        # int_no
    jmp isr_common_stub

.global isr_stub_131
isr_stub_131:
    push 0            # fake err_code
    push 131        # int_no
    jmp isr_common_stub

.global isr_stub_132
isr_stub_132:
    push 0            # fake err_code
    push 132        # int_no
    jmp isr_common_stub

.global isr_stub_133
isr_stub_133:
    push 0            # fake err_code
    push 133        # int_no
    jmp isr_common_stub

.global isr_stub_134
isr_stub_134:
    push 0            # fake err_code
    push 134        # int_no
    jmp isr_common_stub

.global isr_stub_135
isr_stub_135:
    push 0            # fake err_code
    push 135        # int_no
    jmp isr_common_stub

.global isr_stub_136
isr_stub_136:
    push 0            # fake err_code
    push 136        # int_no
    jmp isr_common_stub

.global isr_stub_137
isr_stub_137:
    push 0            # fake err_code
    push 137        # int_no
    jmp isr_common_stub

.global isr_stub_138
isr_stub_138:
    push 0            # fake err_code
    push 138        # int_no
    jmp isr_common_stub

.global isr_stub_139
isr_stub_139:
    push 0            # fake err_code
    push 139        # int_no
    jmp isr_common_stub

.global isr_stub_140
isr_stub_140:
    push 0            # fake err_code
    push 140        # int_no
    jmp isr_common_stub

.global isr_stub_141
isr_stub_141:
    push 0            # fake err_code
    push 141        # int_no
    jmp isr_common_stub

.global isr_stub_142
isr_stub_142:
    push 0            # fake err_code
    push 142        # int_no
    jmp isr_common_stub

.global isr_stub_143
isr_stub_143:
    push 0            # fake err_code
    push 143        # int_no
    jmp isr_common_stub

.global isr_stub_144
isr_stub_144:
    push 0            # fake err_code
    push 144        # int_no
    jmp isr_common_stub

.global isr_stub_145
isr_stub_145:
    push 0            # fake err_code
    push 145        # int_no
    jmp isr_common_stub

.global isr_stub_146
isr_stub_146:
    push 0            # fake err_code
    push 146        # int_no
    jmp isr_common_stub

.global isr_stub_147
isr_stub_147:
    push 0            # fake err_code
    push 147        # int_no
    jmp isr_common_stub

.global isr_stub_148
isr_stub_148:
    push 0            # fake err_code
    push 148        # int_no
    jmp isr_common_stub

.global isr_stub_149
isr_stub_149:
    push 0            # fake err_code
    push 149        # int_no
    jmp isr_common_stub

.global isr_stub_150
isr_stub_150:
    push 0            # fake err_code
    push 150        # int_no
    jmp isr_common_stub

.global isr_stub_151
isr_stub_151:
    push 0            # fake err_code
    push 151        # int_no
    jmp isr_common_stub

.global isr_stub_152
isr_stub_152:
    push 0            # fake err_code
    push 152        # int_no
    jmp isr_common_stub

.global isr_stub_153
isr_stub_153:
    push 0            # fake err_code
    push 153        # int_no
    jmp isr_common_stub

.global isr_stub_154
isr_stub_154:
    push 0            # fake err_code
    push 154        # int_no
    jmp isr_common_stub

.global isr_stub_155
isr_stub_155:
    push 0            # fake err_code
    push 155        # int_no
    jmp isr_common_stub

.global isr_stub_156
isr_stub_156:
    push 0            # fake err_code
    push 156        # int_no
    jmp isr_common_stub

.global isr_stub_157
isr_stub_157:
    push 0            # fake err_code
    push 157        # int_no
    jmp isr_common_stub

.global isr_stub_158
isr_stub_158:
    push 0            # fake err_code
    push 158        # int_no
    jmp isr_common_stub

.global isr_stub_159
isr_stub_159:
    push 0            # fake err_code
    push 159        # int_no
    jmp isr_common_stub

.global isr_stub_160
isr_stub_160:
    push 0            # fake err_code
    push 160        # int_no
    jmp isr_common_stub

.global isr_stub_161
isr_stub_161:
    push 0            # fake err_code
    push 161        # int_no
    jmp isr_common_stub

.global isr_stub_162
isr_stub_162:
    push 0            # fake err_code
    push 162        # int_no
    jmp isr_common_stub

.global isr_stub_163
isr_stub_163:
    push 0            # fake err_code
    push 163        # int_no
    jmp isr_common_stub

.global isr_stub_164
isr_stub_164:
    push 0            # fake err_code
    push 164        # int_no
    jmp isr_common_stub

.global isr_stub_165
isr_stub_165:
    push 0            # fake err_code
    push 165        # int_no
    jmp isr_common_stub

.global isr_stub_166
isr_stub_166:
    push 0            # fake err_code
    push 166        # int_no
    jmp isr_common_stub

.global isr_stub_167
isr_stub_167:
    push 0            # fake err_code
    push 167        # int_no
    jmp isr_common_stub

.global isr_stub_168
isr_stub_168:
    push 0            # fake err_code
    push 168        # int_no
    jmp isr_common_stub

.global isr_stub_169
isr_stub_169:
    push 0            # fake err_code
    push 169        # int_no
    jmp isr_common_stub

.global isr_stub_170
isr_stub_170:
    push 0            # fake err_code
    push 170        # int_no
    jmp isr_common_stub

.global isr_stub_171
isr_stub_171:
    push 0            # fake err_code
    push 171        # int_no
    jmp isr_common_stub

.global isr_stub_172
isr_stub_172:
    push 0            # fake err_code
    push 172        # int_no
    jmp isr_common_stub

.global isr_stub_173
isr_stub_173:
    push 0            # fake err_code
    push 173        # int_no
    jmp isr_common_stub

.global isr_stub_174
isr_stub_174:
    push 0            # fake err_code
    push 174        # int_no
    jmp isr_common_stub

.global isr_stub_175
isr_stub_175:
    push 0            # fake err_code
    push 175        # int_no
    jmp isr_common_stub

.global isr_stub_176
isr_stub_176:
    push 0            # fake err_code
    push 176        # int_no
    jmp isr_common_stub

.global isr_stub_177
isr_stub_177:
    push 0            # fake err_code
    push 177        # int_no
    jmp isr_common_stub

.global isr_stub_178
isr_stub_178:
    push 0            # fake err_code
    push 178        # int_no
    jmp isr_common_stub

.global isr_stub_179
isr_stub_179:
    push 0            # fake err_code
    push 179        # int_no
    jmp isr_common_stub

.global isr_stub_180
isr_stub_180:
    push 0            # fake err_code
    push 180        # int_no
    jmp isr_common_stub

.global isr_stub_181
isr_stub_181:
    push 0            # fake err_code
    push 181        # int_no
    jmp isr_common_stub

.global isr_stub_182
isr_stub_182:
    push 0            # fake err_code
    push 182        # int_no
    jmp isr_common_stub

.global isr_stub_183
isr_stub_183:
    push 0            # fake err_code
    push 183        # int_no
    jmp isr_common_stub

.global isr_stub_184
isr_stub_184:
    push 0            # fake err_code
    push 184        # int_no
    jmp isr_common_stub

.global isr_stub_185
isr_stub_185:
    push 0            # fake err_code
    push 185        # int_no
    jmp isr_common_stub

.global isr_stub_186
isr_stub_186:
    push 0            # fake err_code
    push 186        # int_no
    jmp isr_common_stub

.global isr_stub_187
isr_stub_187:
    push 0            # fake err_code
    push 187        # int_no
    jmp isr_common_stub

.global isr_stub_188
isr_stub_188:
    push 0            # fake err_code
    push 188        # int_no
    jmp isr_common_stub

.global isr_stub_189
isr_stub_189:
    push 0            # fake err_code
    push 189        # int_no
    jmp isr_common_stub

.global isr_stub_190
isr_stub_190:
    push 0            # fake err_code
    push 190        # int_no
    jmp isr_common_stub

.global isr_stub_191
isr_stub_191:
    push 0            # fake err_code
    push 191        # int_no
    jmp isr_common_stub

.global isr_stub_192
isr_stub_192:
    push 0            # fake err_code
    push 192        # int_no
    jmp isr_common_stub

.global isr_stub_193
isr_stub_193:
    push 0            # fake err_code
    push 193        # int_no
    jmp isr_common_stub

.global isr_stub_194
isr_stub_194:
    push 0            # fake err_code
    push 194        # int_no
    jmp isr_common_stub

.global isr_stub_195
isr_stub_195:
    push 0            # fake err_code
    push 195        # int_no
    jmp isr_common_stub

.global isr_stub_196
isr_stub_196:
    push 0            # fake err_code
    push 196        # int_no
    jmp isr_common_stub

.global isr_stub_197
isr_stub_197:
    push 0            # fake err_code
    push 197        # int_no
    jmp isr_common_stub

.global isr_stub_198
isr_stub_198:
    push 0            # fake err_code
    push 198        # int_no
    jmp isr_common_stub

.global isr_stub_199
isr_stub_199:
    push 0            # fake err_code
    push 199        # int_no
    jmp isr_common_stub

.global isr_stub_200
isr_stub_200:
    push 0            # fake err_code
    push 200        # int_no
    jmp isr_common_stub

.global isr_stub_201
isr_stub_201:
    push 0            # fake err_code
    push 201        # int_no
    jmp isr_common_stub

.global isr_stub_202
isr_stub_202:
    push 0            # fake err_code
    push 202        # int_no
    jmp isr_common_stub

.global isr_stub_203
isr_stub_203:
    push 0            # fake err_code
    push 203        # int_no
    jmp isr_common_stub

.global isr_stub_204
isr_stub_204:
    push 0            # fake err_code
    push 204        # int_no
    jmp isr_common_stub

.global isr_stub_205
isr_stub_205:
    push 0            # fake err_code
    push 205        # int_no
    jmp isr_common_stub

.global isr_stub_206
isr_stub_206:
    push 0            # fake err_code
    push 206        # int_no
    jmp isr_common_stub

.global isr_stub_207
isr_stub_207:
    push 0            # fake err_code
    push 207        # int_no
    jmp isr_common_stub

.global isr_stub_208
isr_stub_208:
    push 0            # fake err_code
    push 208        # int_no
    jmp isr_common_stub

.global isr_stub_209
isr_stub_209:
    push 0            # fake err_code
    push 209        # int_no
    jmp isr_common_stub

.global isr_stub_210
isr_stub_210:
    push 0            # fake err_code
    push 210        # int_no
    jmp isr_common_stub

.global isr_stub_211
isr_stub_211:
    push 0            # fake err_code
    push 211        # int_no
    jmp isr_common_stub

.global isr_stub_212
isr_stub_212:
    push 0            # fake err_code
    push 212        # int_no
    jmp isr_common_stub

.global isr_stub_213
isr_stub_213:
    push 0            # fake err_code
    push 213        # int_no
    jmp isr_common_stub

.global isr_stub_214
isr_stub_214:
    push 0            # fake err_code
    push 214        # int_no
    jmp isr_common_stub

.global isr_stub_215
isr_stub_215:
    push 0            # fake err_code
    push 215        # int_no
    jmp isr_common_stub

.global isr_stub_216
isr_stub_216:
    push 0            # fake err_code
    push 216        # int_no
    jmp isr_common_stub

.global isr_stub_217
isr_stub_217:
    push 0            # fake err_code
    push 217        # int_no
    jmp isr_common_stub

.global isr_stub_218
isr_stub_218:
    push 0            # fake err_code
    push 218        # int_no
    jmp isr_common_stub

.global isr_stub_219
isr_stub_219:
    push 0            # fake err_code
    push 219        # int_no
    jmp isr_common_stub

.global isr_stub_220
isr_stub_220:
    push 0            # fake err_code
    push 220        # int_no
    jmp isr_common_stub

.global isr_stub_221
isr_stub_221:
    push 0            # fake err_code
    push 221        # int_no
    jmp isr_common_stub

.global isr_stub_222
isr_stub_222:
    push 0            # fake err_code
    push 222        # int_no
    jmp isr_common_stub

.global isr_stub_223
isr_stub_223:
    push 0            # fake err_code
    push 223        # int_no
    jmp isr_common_stub

.global isr_stub_224
isr_stub_224:
    push 0            # fake err_code
    push 224        # int_no
    jmp isr_common_stub

.global isr_stub_225
isr_stub_225:
    push 0            # fake err_code
    push 225        # int_no
    jmp isr_common_stub

.global isr_stub_226
isr_stub_226:
    push 0            # fake err_code
    push 226        # int_no
    jmp isr_common_stub

.global isr_stub_227
isr_stub_227:
    push 0            # fake err_code
    push 227        # int_no
    jmp isr_common_stub

.global isr_stub_228
isr_stub_228:
    push 0            # fake err_code
    push 228        # int_no
    jmp isr_common_stub

.global isr_stub_229
isr_stub_229:
    push 0            # fake err_code
    push 229        # int_no
    jmp isr_common_stub

.global isr_stub_230
isr_stub_230:
    push 0            # fake err_code
    push 230        # int_no
    jmp isr_common_stub

.global isr_stub_231
isr_stub_231:
    push 0            # fake err_code
    push 231        # int_no
    jmp isr_common_stub

.global isr_stub_232
isr_stub_232:
    push 0            # fake err_code
    push 232        # int_no
    jmp isr_common_stub

.global isr_stub_233
isr_stub_233:
    push 0            # fake err_code
    push 233        # int_no
    jmp isr_common_stub

.global isr_stub_234
isr_stub_234:
    push 0            # fake err_code
    push 234        # int_no
    jmp isr_common_stub

.global isr_stub_235
isr_stub_235:
    push 0            # fake err_code
    push 235        # int_no
    jmp isr_common_stub

.global isr_stub_236
isr_stub_236:
    push 0            # fake err_code
    push 236        # int_no
    jmp isr_common_stub

.global isr_stub_237
isr_stub_237:
    push 0            # fake err_code
    push 237        # int_no
    jmp isr_common_stub

.global isr_stub_238
isr_stub_238:
    push 0            # fake err_code
    push 238        # int_no
    jmp isr_common_stub

.global isr_stub_239
isr_stub_239:
    push 0            # fake err_code
    push 239        # int_no
    jmp isr_common_stub

.global isr_stub_240
isr_stub_240:
    push 0            # fake err_code
    push 240        # int_no
    jmp isr_common_stub

.global isr_stub_241
isr_stub_241:
    push 0            # fake err_code
    push 241        # int_no
    jmp isr_common_stub

.global isr_stub_242
isr_stub_242:
    push 0            # fake err_code
    push 242        # int_no
    jmp isr_common_stub

.global isr_stub_243
isr_stub_243:
    push 0            # fake err_code
    push 243        # int_no
    jmp isr_common_stub

.global isr_stub_244
isr_stub_244:
    push 0            # fake err_code
    push 244        # int_no
    jmp isr_common_stub

.global isr_stub_245
isr_stub_245:
    push 0            # fake err_code
    push 245        # int_no
    jmp isr_common_stub

.global isr_stub_246
isr_stub_246:
    push 0            # fake err_code
    push 246        # int_no
    jmp isr_common_stub

.global isr_stub_247
isr_stub_247:
    push 0            # fake err_code
    push 247        # int_no
    jmp isr_common_stub

.global isr_stub_248
isr_stub_248:
    push 0            # fake err_code
    push 248        # int_no
    jmp isr_common_stub

.global isr_stub_249
isr_stub_249:
    push 0            # fake err_code
    push 249        # int_no
    jmp isr_common_stub

.global isr_stub_250
isr_stub_250:
    push 0            # fake err_code
    push 250        # int_no
    jmp isr_common_stub

.global isr_stub_251
isr_stub_251:
    push 0            # fake err_code
    push 251        # int_no
    jmp isr_common_stub

.global isr_stub_252
isr_stub_252:
    push 0            # fake err_code
    push 252        # int_no
    jmp isr_common_stub

.global isr_stub_253
isr_stub_253:
    push 0            # fake err_code
    push 253        # int_no
    jmp isr_common_stub

.global isr_stub_254
isr_stub_254:
    push 0            # fake err_code
    push 254        # int_no
    jmp isr_common_stub

.global isr_stub_255
isr_stub_255:
    push 0            # fake err_code
    push 255        # int_no
    jmp isr_common_stub

