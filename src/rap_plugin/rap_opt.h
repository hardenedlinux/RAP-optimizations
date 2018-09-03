/* The impelmentation code of optimization pass for RAP.
   Supply the API for RAP  */

#ifndef RAP_OPT_H
#define RAP_OPT_H

/* Contains the beed called optimization level of GCC */
extern int gcc_optimize_level;
/* Count how many function we have optimized */
extern int rap_opt_statistics;

/* Try make GCC call ipa-pta pass if optimization level is NOT 0 */
void rap_try_call_ipa_pta (void* gcc_data, void* user_data);
void rap_gather_function_targets ();
int is_rap_function_maybe_roped (tree f);
void rap_opt_statistics ();
void rap_optimization_clean ();

#endif /* RAP_OPT_H */
