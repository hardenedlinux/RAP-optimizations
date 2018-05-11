/* The impelmentation code of optimization pass for RAP.
   We supply the API for RAP  */

#include "gcc-common.h"
//#include "system.h"
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
//set fp_set;

/* Contain available funtion */
//set func_set;

/* TODO, move this to rap plugin_init */
//register_callback(plugin_name, PLUGIN_FINISH_DECL, rap_gather_fp, &fp_set);
//register_callback(plugin_name, PLUGIN_FINISH_UNIT, rap_gather_avail_function, 
  //                &func_set);

/* Test for function of file scope */
extern bool is_rap_function_never_escape (tree t) {
        /* We must call this procedure after IPA_PASS */
        /* currect pass.pass.tv_id > TV_CGRAPHOPT */
        /* gcc_checking_assert (TREE_CODE (t) == FUNCTION_DECL); */
        struct cgraph_node *node =  cgraph_get_node(t);

        gcc_checking_assert(node);
        
        /* Function is visible in current compilation unit only
           and its address is never taken. */
        if (node->local && node->local.local)
                return true;

        /* Function has NOT address taken */
        /* Function is NOT visible by other units */
        /* And we need the function is file scope, because we have not whole 
           program optimization*/
        else if (!(node->symbol.address_taken 
                || node->symbol.externally_visible 
                || TREE_PUBLIC(tree)))
                return true;

        return false;
}

#if 0
/*

*/
extern void rap_gather_fp(void *decl, set *s)
{
        /* If current declaration is variable and its type is funtion pointer 
           gather it */
        tree t = (tree)decl;
        /* Only care about pointer to function type */
        if (TREE_CODE(t) == POINTER_TYPE 
            && TREE_CODE(TREE_TYPE(t)) == FUNCTION_TYPE)
                add_to_set(t, s);

        return;
}



extern void rap_gather_avail_function(void *decl, set *s) 
{
        tree t = (tree)decl;
        /* Gather function type what are our potential target */
        if (TREE_CODE(t) == POINTER_TYPE)
                add_to_set(t, s);

        return;
}



/* Check current funtion whether a possible target for the function pointer.
   
   If available return 0 otherwise return 1. */
/*
extern int rap_available_naive(tree p, tree func) 
{


}
*/
#endif
