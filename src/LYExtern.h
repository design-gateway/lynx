#ifndef EXTERNALS_H
#define EXTERNALS_H

#ifndef LYSTRUCTS_H
#include <LYStructs.h>
#endif /* LYSTRUCTS_H */

void run_external PARAMS((char * c));
char *string_short PARAMS((char * str, int cut_pos));

#ifdef WIN_EX
extern char * quote_pathname PARAMS((char * pathname));
#endif

#endif /* EXTERNALS_H */
