#include "dof.h"
#include "doflineargathersat.h"
#include "dofdiffusion.h"

#pragma warning (disable : 4018)











PtrDOF createLinGatherDOF (PtrFuncRadiusOfCoC func)
{
	return boost::shared_ptr<DOF> (new DOFlinearGatherSAT (func));
}

PtrDOF createDiffusionDOF (PtrFuncRadiusOfCoC func)
{
	return boost::shared_ptr<DOF> (new DOFdiffusion (func));
}