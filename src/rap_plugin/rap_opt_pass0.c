/* The impelmentation code of optimization pass for RAP.
   We supply the API for RAP  */

#include "gcc-common.h"
#include "bitmap.h"
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


/* This variable defined in GCC source */
struct opt_pass *current_pass;
struct simple_ipa_opt_pass pass_ipa_pta;

/* Make sure we will call GCC ipa pta pass */
void rap_make_sure_call_ipa_pta (void* gcc_data, void* user_data) 
{
  /* Make sure we have reach */
  bool init = false;

  //gcc_assert (current_pass);
  if (current_pass && (void*)current_pass == (void*)pass_ipa_pta)
  {
    *(bool*)gcc_data = true;
    init = true;
  }
  gcc_assert (init);

  return;
}


/* Test for function of file scope */
bool is_rap_function_never_escape (tree t) {
        /* We must call this procedure after IPA_PASS */
        /* currect pass.pass.tv_id > TV_CGRAPHOPT */
        /* gcc_checking_assert (TREE_CODE (t) == FUNCTION_DECL); */
#if 0
        struct cgraph_node *node =  cgraph_get_node(t);

        gcc_checking_assert(node);
        
        /* Function is visible in current compilation unit only
           and its address is never taken. */
        if (/*node->local && */ node->local.local)
                return true;

        /* Function has NOT address taken */
        /* Function is NOT visible by other units */
        /* And we need the function is file scope, because we have not whole 
           program optimization*/
        else if (!(node->symbol.address_taken 
                || node->symbol.externally_visible 
                || TREE_PUBLIC(t)))
                return true;
#endif
        return false;
}

/* Basic test */  

void rap_gather_function_targets ()
{
  struct cgraph_node *node;
  struct varpool_node *var;
  int from;

  /* Gather function pointer infos from every variable */
  FOR_EACH_VARIABLE (var)
    {
      if (var->alias)
	continue;

    }
  
  /* Gather function pointer infos from every function */
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      struct function *func;
      basic_block bb;

      /* Nodes without a body are not interesting.  */
      if (!cgraph_function_with_gimple_body_p (node))
	continue;

      func = DECL_STRUCT_FUNCTION (node->symbol.decl);
      push_cfun (func);

      /* Gather function pointer infos from the function body.  */
      FOR_EACH_BB_FN (bb, func)
	{
	  gimple_stmt_iterator gsi;

	  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	    {
	      gimple stmt = gsi_stmt (gsi);

	      find_func_aliases (stmt);
	    }
	}

      pop_cfun ();
    }

  /* Assign the points-to sets to the SSA names in the unit.  */
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      tree ptr;
      struct function *fn;
      unsigned i;
      basic_block bb;

      /* Nodes without a body are not interesting.  */
      if (!cgraph_function_with_gimple_body_p (node))
	continue;

      fn = DECL_STRUCT_FUNCTION (node->symbol.decl);

      /* Compute the points-to sets for pointer SSA_NAMEs.  */
      FOR_EACH_VEC_ELT (*fn->gimple_df->ssa_names, i, ptr)
	{
	  if (ptr
	      && POINTER_TYPE_P (TREE_TYPE (ptr)))
	    //find_what_p_points_to (ptr);
	}

      FOR_EACH_BB_FN (bb, fn)
	{
	  gimple_stmt_iterator gsi;

	  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	    {
	      gimple stmt = gsi_stmt (gsi);
	      struct pt_solution *pt;
	      tree decl;

	      if (!is_gimple_call (stmt))
		continue;

	      /* Handle direct calls to external functions.  */
	      decl = gimple_call_fndecl (stmt);

	      /* Handle indirect calls.  */
	      if (!decl
		  && (fi = get_fi_for_callee (stmt)))
		{

		}
	    }
	}
    }
  //
  bitmap ignore_funcs = BITMAP_ALLOC (NULL);
  //
  bitmap_bit_p (ignore_funcs, DECL_UID (symbol));
  //
  BITMAP_FREE(ignore_funcs);
}
// int call_flags = gimple_call_flags (stmt);
static inline bool 
is_rap_function_may_be_aliased (tree f)
{
  return (TREE_CODE (f) != CONST_DECL
	  && !((TREE_STATIC (f) || TREE_PUBLIC (f) || DECL_EXTERNAL (f))
	       && TREE_READONLY (f)
	       && !TYPE_NEEDS_CONSTRUCTING (TREE_TYPE (f)))
	  && (TREE_PUBLIC (f)
	      || DECL_EXTERNAL (f)
	      || TREE_ADDRESSABLE (f)));
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
