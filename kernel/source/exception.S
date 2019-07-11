#include <sys/arm.h>


#define EXCP_DIV0             0
#define EXCP_UNDEF_INST       1
#define EXCP_PREF_ABORT       2
#define EXCP_DATA_ABORT       3
#define EXCP_UNKNOWN          4


    .macro    except_handler_stub name, exception, offset
    .global   \name
\name:
    sub    sp, sp, #4               /* correct stack (number of pushs must be even) */
    sub    lr, lr, #\offset         /* lr: correct PC of aborted program */
    stmfd  sp!, {lr}                /* store PC onto stack */
    mrs    lr, spsr                 /* lr can be overwritten now */
    stmfd  sp!, {lr}                /* store saved PSR onto stack */
    stmfd  sp, {r0-r14}^            /* store user registers r0-r14 (unbanked) */
    sub    sp, sp, #4*15            /* correct stack (not done by previous instruction */
    mov    r1, sp                   /* save sp_abt or sp_und */
    cps    #0x12                    /* set IRQ mode to access sp_irq and lr_irq */
    mov    r2, sp
    mov    r3, lr
    cps    #0x11                    /* set FIQ mode to access sp_fiq and lr_fiq */
    mov    r4, sp
    mov    r5, lr
    cps    #0x1F                    /* our abort handler runs in system mode */
    mov    sp, r1                   /* set sp_sys to stack top of abort stack */
    stmfd  sp!, {r2-r5}             /* store lr_fiq, sp_fiq, lr_irq, sp_irq onto stack */
    mov    r1, sp                   /* r1: pointer to register frame */
    mov    r0, #\exception          /* r0: exception identifier */
    b      except_handler           /* jump to ExceptionHandler (never returns) */
    .endm


    except_handler_stub except_undef_handler, EXCP_UNDEF_INST, 4
    except_handler_stub except_pref_handler, EXCP_PREF_ABORT, 4
    except_handler_stub except_data_handler, EXCP_DATA_ABORT, 8