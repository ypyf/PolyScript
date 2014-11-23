#pragma once

#ifndef TYPES_H_
#define TYPES_H_

typedef unsigned char Byte;

typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;
typedef unsigned long long U64;

typedef signed char    I8;
typedef signed short   I16;
typedef int            I32;
typedef long long      I64; // LLP64

#ifndef TRUE
#define TRUE            1
#endif

#ifndef FALSE
#define FALSE           0
#endif

#define CH_SPACE       ' '

#endif  /* TYPES_H_ */