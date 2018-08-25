/* The impelmentation code of optimization pass for RAP.
   We supply the API for RAP  */

#include "gcc-common.h"
#include "bitmap.h"

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

//register_callback(plugin_name, PLUGIN_FINISH_DECL, rap_gather_fp, &fp_set);
//register_callback(plugin_name, PLUGIN_FINISH_UNIT, rap_gather_avail_function, 
  //                &func_set);

/* This variable defined in GCC source */
struct opt_pass *current_pass;
struct simple_ipa_opt_pass pass_ipa_pta;
// Hold all the ROP target functions.
bitmap sensi_funcs = BITMAP_ALLOC (NULL);

/* Make sure we will call GCC ipa pta pass */
void rap_make_sure_call_ipa_pta (void* gcc_data, void* user_data) 
{
  /* Make sure we have reach */
  bool init = false;

  //gcc_assert (current_pass);
  if (current_pass && (void*)current_pass == (void*)&pass_ipa_pta)
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

/* Gather what functions never may be indirected called */
void rap_gather_function_targets ()
{
  struct cgraph_node *node;
  struct varpool_node *var;
  struct ptr_info_def *pi = NULL;
  bitmap set = NULL;
  
  /* Gather function pointer infos from global may available variable */
  FOR_EACH_VARIABLE (var)
    {
      if (var->alias)
	      continue;
      gcc_assert(var->symbol.decl);
      set = (SSA_NAME_PTR_INFO(var->symbol.decl))->pt.vars;
      if (! bitmap_empty_p(set))
        bitmap_ior_into(sensi_funcs, set);
    }
  /* Gather function pointer infos from every function */
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      struct function *func;
      tree fp = NULL;
      int i;
      set = NULL;

      /* Nodes without a body are not interesting.  */
      if (!cgraph_function_with_gimple_body_p (node))
	      continue;
      func = DECL_STRUCT_FUNCTION (node->symbol.decl);
      push_cfun (func);
      /* Function pointers will be SSA_NAME contained in current function, 
        When gcc after pointer analysis we gather all the functions may be 
        pointed by some function pointer and we ignore which function pointer
        can access it. All this gathered function are the sensitive data, need
        RAP add guard instruction. */

      for (i = 0, pi = NULL; fp = ssa_name (i); i++)
        {
          if (pi = SSA_NAME_PTR_INFO(fp))
            if (! bitmap_empty_p(set = pi->pt.vars))
              bitmap_ior_into(sensi_funcs, set);
          pi = NULL;
        }
    } // FOR_EACH_DEFINED_FUNCTION (node)

  return;
} // end of rap_gather_function_targets

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


/* Look up current function weather or not beed gathered into our target
   function set. If NOT return 1 otherwise return 0 */
int 
is_rap_function_maybe_roped (tree f)
{
  return ! bitmap_bit_p(DECL_UID(f));
}

