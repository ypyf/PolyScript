#ifndef XASM_H_
#define XASM_H_

struct AssemblyInfo
{
    char* sourceName;

};

void XASM_Assembly(const char* filename, const char* execFileName);

#endif /* XASM_H_ */
