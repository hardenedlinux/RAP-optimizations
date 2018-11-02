/* Writed by David fuqiang Fan <feqin1023@gmail.com> &
   Shawn C[a.k.a "citypw"] <citypw@gmail.com> members of HardenedLinux.
   The code of this file try to make some optimizationsfor  PaX RAP.
   Supply the API for RAP.

   And we also call function wich compute function type hash from PaX RAP.
   Code architecture inspired by RAP of PaX Team <pageexec@freemail.hu>.

   Licensed under the GPL v2. */

#include <stdio.h>

#ifndef RAP_OPT_H
#define RAP_OPT_H

/* Contains the beed called optimization level of GCC */
extern volatile int cfi_gcc_optimize_level;
/* Count how many function we have optimized */
extern int rap_opt_statistics_data;

/* Try make GCC call ipa-pta pass if optimization level is NOT 0 */
void rap_try_call_ipa_pta (void* gcc_data, void* user_data);
//void rap_gather_function_targets ();
int is_rap_function_maybe_roped (tree f);
void rap_optimization_clean ();

#endif /* RAP_OPT_H */
