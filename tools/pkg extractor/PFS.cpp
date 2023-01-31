#include "PFS.h"

PFS::PFS()
{

}


PFS::~PFS()
{

}

bool PFS::pfsOuterReadHeader(U08* psfOuterStart)
{
	psfOuterheader = (PFS_HDR&)psfOuterStart[0];
	return true;
}