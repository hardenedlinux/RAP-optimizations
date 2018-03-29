/* The impelmentation code of optimization pass for RAP.
   We supply the API for RAP  */

#include "gcc-common.h"

//#include "set.h"


/* There are many optimization methrod can do for RAP.
   From simple to complex and aside with the gcc internal work stage.

   Maybe the most efficent methrod is add gcc intenal library and require to merge.
   Because of the lack of analysis for function pointer.

   Wrok with gcc plugin we have more isolate and also indepened with gcc version
   methrod, but need more replicate data 

   1, If a function pointer variable has not external linkage and live at code 
      generation, it's legal function set must be only just the current file 
      scope avail function and the proper type.
   
   2, Global alias anylysis for the avail function set. */

/* Contain function pointer variable */
set fp_set;

/* Contain available funtion */
set func_set;

/* TODO, move this to rap plugin_init */
register_callback(plugin_name, PLUGIN_FINISH_DECL, rap_gather_fp, &fp_set);
register_callback(plugin_name, PLUGIN_FINISH_UNIT, rap_gather_avail_function, 
                  &func_set);

/*

*/
extern void rap_gather_fp(void *decl, set *s)
{
        /* If current declaration is variable and its type is funtion pointer 
           gather it */
        if (decl)
                add_to_set(s, decl);
}



extern void rap_gather_avail_function(void *decl, set *s) 
{

}


/* Check current funtion whether a possible target for the function pointer.
   
   If available return 0 otherwise return 1. */
extern int rap_available_naive(tree p, tree func) 
{


}

