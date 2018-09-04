/* Write by David fuqiang Fan <feqin1023@gmail.com>, member of HardenedLinux.
   Licensed under the GPL v2
   The impelmentation code of optimization pass for RAP.
   Supply the API for RAP  */

#include <stdio.h>
#include <string.h>
#include "rap.h"
#include "tree-pass.h"
//#include "../include/pointer-set.h"
 
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

/* This data structure defined in gcc source. */
//extern struct simple_ipa_opt_pass pass_ipa_pta;

/* Contains the beed called optimization level of GCC */
int gcc_optimize_level = 0;
/* Count how many function we have optimized */
int rap_opt_statistics_data = 0;
/* Contain the statistics data, maybe gcc will called many times, we need output
   data in the same file, for the conventices of scripts process. */
const char *dump_rap_opt_statistics_filename = "rap_opt_statistics_dump";
FILE *dump_rap_opt_statistics_fd;
// Hold all the ROP target functions.
static bitmap sensi_funcs = BITMAP_ALLOC (NULL);
/* Contains the type database which are pointer analysis can not sloved */
static struct pointer_set_t *pointer_types;

/* Try make GCC call ipa-pta pass if optimization level is NOT 0 */
void 
rap_try_call_ipa_pta (void* gcc_data, void* user_data) 
{
  /* Make sure we have reach */
  bool init = false;

  //gcc_assert (current_pass);
  if (current_pass 
      && 
      (! strcmp (((struct opt_pass *)current_pass)->name, "pta")))
    {
      *(bool*)gcc_data = true;
      init = true;
      /* The variable optimize is defined int GCC */
      *(int*)user_data = optimize;
    }
  gcc_assert (init);

  return;
}

/* Tools for type database operates */
static void
insert_type_db (tree t)
{
  tree ty = TREE_TYPE (t);

  if (! pointer_types)
    pointer_types = pointer_set_create ();

  gcc_assert (FUNCTION_POINTER_TYPE_P (t));
  pointer_set_insert (pointer_types, (const void *)ty);

  return;
}


/* If after alias analysis some function pointer may be point anything, we have
   to make the conservation solution, contain and cache the the point to type,
   when emit RAP guard code, make sure all the functions of the compatible type
   NOT igonred and optimized 
   Return value is trivial */
static bool
rap_base_type_db_fliter (const void *db_type, void *fn)
{
  tree f = (tree) fn;
  gcc_assert (TREE_CODE (f) == FUNCTION_DECL);
  if (types_compatible_p ((tree)db_type, TREE_TYPE(f)))
    if (bitmap_clear_bit (sensi_funcs, DECL_UID(f)))
      return true;

  return true;
}

/* The real worker */
static void
rap_gather_function_targets_1 (tree t)
{
  //types_compatible_p()
  //TREE_TYPE()
  //FUNCTION_POINTER_TYPE_P()
  struct ptr_info_def *pi;
  bitmap set;
  pi = SSA_NAME_PTR_INFO (t);
  if (pi)
    {
      if (pi->pt.anything)
        /* we are in trouble, pointer analysis can not give any answer about 
           this pointer point to */
        insert_type_db (t);
    }
  else
    {
      set = pi->pt.vars;
      if (! bitmap_empty_p (set))
        bitmap_ior_into (sensi_funcs, set);
    }

  return;
}

/* Entry point of build the oracle, gather what functions never may be 
   indirected called */
void 
rap_gather_function_targets ()
{
  struct cgraph_node *node;
  struct varpool_node *var;
  tree t;
  struct function *func;
  unsigned int i;

  /* This optimization depend on GCC optimizations */
  if (0 == gcc_optimize_level)
    return;

  /* Gather function pointer infos from global may available variable */
  FOR_EACH_VARIABLE (var)
    {
      if (var->alias)
	continue;
      gcc_assert (t = var->symbol.decl);
      /* We only care about function pointer variables */
      if (! FUNCTION_POINTER_TYPE_P (t))
	continue;
      rap_gather_function_targets_1 (t);
    }
  /* Gather function pointer infos from every function */
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      t = NULL;

      /* Nodes without a body are not interesting.  */
      if (!cgraph_function_with_gimple_body_p (node))
	continue;
      func = DECL_STRUCT_FUNCTION (node->symbol.decl);
      //push_cfun (func);
      /* Function pointers will be SSA_NAME contained in current function, 
        When gcc after pointer analysis we gather all the functions may be 
        pointed by some function pointer and we ignore which function pointer
        can access it. All this gathered function are the sensitive data, need
        RAP add guard instruction. */
      FOR_EACH_VEC_ELT (*func->gimple_df->ssa_names, i, t)
	{
	  if (! (t || FUNCTION_POINTER_TYPE_P (t)))
	    continue;
	  rap_gather_function_targets_1 (t);
        }
    } // FOR_EACH_DEFINED_FUNCTION (node)
  
  /* We have see all of the data about alias and build the type database, It's 
     time for the final fliter */
  if (! pointer_types)
    return;
  /* fial fliter */
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      t = NULL;

      /* Nodes without a body are not interesting.  */
      if (!cgraph_function_with_gimple_body_p (node))
	continue;
      func = DECL_STRUCT_FUNCTION (node->symbol.decl);
      pointer_set_traverse (pointer_types, 
		            rap_base_type_db_fliter,
		            func);
    }

  return;
} // end of rap_gather_function_targets


/* Basic test of function nature */
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

/* Entry point of the oracle, look up current function weather or not beed
   gathered into our target function set. If YES return 1 otherwise return 0 */
int 
is_rap_function_maybe_roped (tree f)
{
  if (! is_rap_function_may_be_aliased (f))
    return 0;
  /* Ask the oracle for help */
  if (0 == gcc_optimize_level)
    /* Function is_rap_function_may_be_aliased() must be failed we arive here, 
       but our oracle dependent the GCC optimizations. */
    return 1;
  else
    return bitmap_bit_p (sensi_funcs, DECL_UID (f));
}

/* Write some statistics for our algorithm */
void
dump_rap_opt_statistics ()
{
  dump_rap_opt_statistics_fd = fopen (dump_rap_opt_statistics_filename, "a");
  fprintf (dump_rap_opt_statistics_fd, "%d\n", rap_opt_statistics_data);

  return;
}

/* Clean and cloese function for optimizations */
void
rap_optimization_clean ()
{
  /* Now we have finish our job, close file and destroy the GCC allocated data*/
  fclose (dump_rap_opt_statistics_fd);
  bitmap_clear (sensi_funcs);
  pointer_set_destroy (pointer_types);

  return;
}

