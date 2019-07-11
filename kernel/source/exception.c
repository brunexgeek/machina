#include <sys/exception.h>
#include <sys/soc.h>


#define PTR_DIFF(from, to)        ((uint32_t *) &(to) - (uint32_t *) &(from) - 2)


void except_undef_handler();
void except_pref_handler();
void except_data_handler();


int except_initialize()
{
    struct excepttbl *table = (struct excepttbl*) BCMXXXX_EXCEPTTBL;
    table->UndefinedInstruction = 0xEA000000 | PTR_DIFF(table->IRQ, except_undef_handler);
    //table->SupervisorCall = 0xEA000000 | PTR_DIFF(table->IRQ, except_undef_handler);
    table->PrefetchAbort = 0xEA000000 | PTR_DIFF(table->IRQ, except_pref_handler);
    table->DataAbort = 0xEA000000 | PTR_DIFF(table->IRQ, except_data_handler);
}


void except_handler( uint32_t id, void *frame )
{

}