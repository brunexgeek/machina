#ifndef MACHINA_ARM_H
#define MACHINA_ARM_H


#define BEGIN_ASM_FUNCTION(x) \
	.text; .global x; x:

#define ASM_SIZE(x)	.size x, . - x;

#define END_ASM_FUNCTION(x) \
    ASM_SIZE(x)

#endif // MACHINA_ARM_H

