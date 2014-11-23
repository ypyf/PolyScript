#ifndef __XASM_H__
#define __XASM_H__

struct AssemblyInfo
{
	char* sourceName;

};

void YASM_Assembly(const char* filename, const char* execFileName);

#endif // __XASM_H__