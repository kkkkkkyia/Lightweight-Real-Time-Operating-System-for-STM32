#include "my_arch.h"
#include "port.h"
#include "my_task.h"

my_uint8 ATCH_IsInterruptContext(void)
{
	return 0;
}


void * ARCH_PrepareStack(void* StartOfstack, void * Param)
{
	My_TCBInitParameter* TaskParam = (My_TCBInitParameter *)Param;

	my_uint32 * TopOfStack = (my_uint32 *)((my_uint32)StartOfstack + TaskParam->StackSize - sizeof(my_uint32));
	return pxPortInitialiseStack(TopOfStack,(my_uint32)(TaskParam->TaskEntry),TaskParam->PrivateData);

}
