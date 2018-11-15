/* Writed by David fuqiang Fan <feqin1023@gmail.com> &
   Shawn C[a.k.a "citypw"] <citypw@gmail.com> members of HardenedLinux.
   The code of this file try to make some optimizations for PaX RAP.
   Supply the API for RAP.

   And we also call function wich compute function type hash from PaX RAP.
   Code architecture inspired by RAP of PaX Team <pageexec@freemail.hu>.

   Licensed under the GPL v2. */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "gcc-common.h"
#include "rap.h"
#include "tree-pass.h"
#include "diagnostic.h"
#include "bitmap.h"
#include "gimple-pretty-print.h"


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


/* Contains the beed called optimization level of GCC */
int cfi_gcc_optimize_level;
/* Count how many function we have optimized */
int rap_opt_statistics_data;
/* Contain the statistics data, maybe gcc will called many times, we need output
   data in the same file, for the conventices of scripts process. */
//const char *dump_rap_opt_statistics_filename = "rap_opt_statistics_dump";
//FILE *dump_rap_opt_statistics_fd;

/* Contain all the sensitive functions which maybe the targets of ROP.
   so all of these functons should compute and output the function hash. */
static bitmap sensi_funcs;
/* Contains the type database which are pointer analysis can not sloved */
static struct pointer_set_t *sensi_func_types;
//
static bool will_call_ipa_pta;
/* For compatiable with the original RAP */
typedef int rap_hash_value_type;
/* Used for disable dom info, because dom info is function based, 
   when cfun changed set this falg.  */
//static bool is_cfi_chaned_cfun;
//static bool is_cfi_need_clean_dom_info;

// gcc internal defined pass name.
extern struct ipa_opt_pass_d pass_ipa_inline;


/* Test GCC will call some passes which is benefit. */

void 
rap_check_will_call_passes (void* gcc_data, void* user_data) 
{
  //gcc_assert (current_pass);
  if (current_pass 
      && 
     (void *)current_pass == (void *)&pass_ipa_inline)
      /*(! strcmp ((current_pass)->name, "inline")))*/
    {
      if (*(bool*)gcc_data)
        if (dump_file && (dump_flags & TDF_DETAILS))
	  fprintf(dump_file, "hl-cfi -  NOT call pass 'inline'\n");
    }

  return;
}

// gcc internal defined pass name.
extern struct simple_ipa_opt_pass pass_ipa_pta;
extern struct gcc_options global_options;


/* Try make GCC call ipa-pta pass if optimization level is NOT 0 */

void 
rap_try_call_ipa_pta (void* gcc_data, void* user_data) 
{
  gcc_assert (current_pass);
  /* If already execute pta pass, return immediatelly.  */
  if (will_call_ipa_pta)
    return;
  
  /* !!!NOTICE, There is a EXTRA item definition: x_flag_emit_templates 
     in Ubuntu official package gcc-plugin-dev ../4.8/plugin/include/options.h.
     But when we build the gcc the code download from gnu.gcc.org haven't
     this item, so when you access the item after x_flag_dump_rtl_in_asm in 
     the struct global_options, you are fucked.  
     I really thought this is a gcc bug, when i dig into the g++ parser
     after two days. I found this above.  */
  if (optimize &&
      (0 == errorcount + sorrycount) &&
      (void *)current_pass == (void *)&pass_ipa_pta)
    {
      will_call_ipa_pta = true;
      *(bool*)gcc_data = true;
      /* The variable optimize is defined int GCC */
      *(volatile int*)user_data = optimize;
    }
  
  return;
}


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


/* Tools for type database operates */

static void
insert_type_db (tree t)
{
  tree ty = TREE_TYPE (t);

  if (! sensi_func_types)
    sensi_func_types = pointer_set_create ();

  gcc_assert (FUNCTION_POINTER_TYPE_P (TREE_TYPE (t)));
  gcc_assert (TREE_CODE (TREE_TYPE (ty)) == FUNCTION_TYPE);

  pointer_set_insert (sensi_func_types, (const void *)ty);

  return;
}


/* If after alias analysis some function pointer may be point anything, we have
   to make the conservation solution, contain and cache the the point to type,
   when emit RAP guard code, make sure all the functions of the compatible type
   NOT igonred and optimized.
   If return false function pointer_set_staverse() of outside will stop. */ 

static bool
rap_base_type_db_fliter (const void *type, void *fn)
{
  tree f = (tree) fn;
  gcc_assert (TREE_CODE (f) == FUNCTION_DECL);

  if (types_compatible_p ((tree)type, TREE_TYPE(f)))
    if (bitmap_set_bit (sensi_funcs, DECL_UID(f)))
      return false;

  return true;
}


/* The real worker */

static void
rap_gather_function_targets_1 (tree t)
{
  struct ptr_info_def *pi;
  bitmap set;
  pi = SSA_NAME_PTR_INFO (t);

  gcc_assert (FUNCTION_POINTER_TYPE_P (TREE_TYPE(t)));

  if (NULL == pi || pi->pt.anything)
    {
      /* we are in trouble, pointer analysis can not give any answer about 
         this pointer point to, so we record the all the type of the
     	 function pointer. */	    
      insert_type_db (t);
      return;
    }
  else
    {
      // elseif part.
      set = pi->pt.vars;
      if (! bitmap_empty_p (set))
	/* sensi_funcs |= set */
        bitmap_ior_into (sensi_funcs, set);
    }

  return;
}


/* Entry point of build the oracle which can answer which function is maybe 
   ROPed.

   Technically we have two sets support this:
   - sensi_func_types : which for the worse case of ipa-pta cann't 
                        compute the pointers point to set. we contain the 
			pointers types.
   - sensi_funcs : the core database of oracle. contains all the functions
                   which is sensitive.

   And we also code of have three part for this:
   1, Global variables in varpool whose type is pointer to function. 
      we pull it's point to set into sensi_funcs or the worse it's type 
      (conservative, presume it can point to any function with the 
      compatiable type).
   2, Local variables in cfun->gimple_df.ssa_names. like as above.
   3, Final fliter for all the functions, whose type is compatiables with
      any item of the sensi_func_types.  */

void 
rap_gather_function_targets ()
{
  struct cgraph_node *node;
  struct varpool_node *var;
  tree t = NULL;
  struct function *func;
  unsigned int i;

  /* This optimization depend on GCC optimizations */
  if (0 == cfi_gcc_optimize_level)
    return;
  // 
  gcc_assert (will_call_ipa_pta);
  gcc_assert (NULL == sensi_funcs);

  bitmap sensi_funcs = BITMAP_ALLOC (NULL);

  /* 1, Gather function pointer infos from globals in varpool. */
  FOR_EACH_VARIABLE (var)
    {
      if (var->alias)
	continue;
      gcc_assert (t = var->symbol.decl);
      /* We only care about function pointer variables */
      if (! FUNCTION_POINTER_TYPE_P (TREE_TYPE(t)))
	continue;
      rap_gather_function_targets_1 (t);
    }
  /* 2, Gather function pointer infos from locals of every functions. */
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      /* Nodes without a body are not interesting.  */
      if (!cgraph_function_with_gimple_body_p (node))
	continue;

      func = DECL_STRUCT_FUNCTION (node->symbol.decl);
      
      /* Function pointers will be SSA_NAME contained in current function, 
        When gcc after pointer analysis we gather all the functions may be 
        pointed by some function pointer and we ignore which function pointer
        can access it. All this gathered function are the sensitive data, need
        RAP add guard instruction. */
      FOR_EACH_VEC_ELT (*func->gimple_df->ssa_names, i, t)
	{
	  if (t && FUNCTION_POINTER_TYPE_P (TREE_TYPE (t)))
	    rap_gather_function_targets_1 (t);
        }
    } // FOR_EACH_DEFINED_FUNCTION (node)
  
  /* 3, We have see all of the data about alias and build the type database, 
     It's time for the final oracle. */
  if (! sensi_func_types)
    return;

  /* final fliter */
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      /* Nodes without a body are not interesting.  */
      if (!cgraph_function_with_gimple_body_p (node))
	continue;

      t = node->symbol.decl;

      /* Never been the target. local functions, etc..  */
      if (is_rap_function_may_be_aliased (t))
	continue;

#ifdef HL_CFI_CHECKING
      gcc_assert(! bitmap_bit_p (sensi_funcs, DECL_UID (t));
#endif

      pointer_set_traverse (sensi_func_types, 
		            rap_base_type_db_fliter,
		            t);
    }

  return;
} // end of rap_gather_function_targets


/* The wraper of hl_gather pass.  */

static unsigned int
hl_gather_execute ()
{
  rap_gather_function_targets ();
  
  return 0;
}


/* Will we need the gather?  */

static bool
hl_gather_gate ()
{
  if (will_call_ipa_pta && require_call_hl_gather)
    return true;

  return false;
}


/* Genetate the pass structure */
#define PASS_NAME hl_gather
//#define PROPERTIES_REQUIRED PROP_gimple_any
//#define PROPERTIES_PROVIDED PROP_gimple_lcf
#define TODO_FLAGS_FINISH \
	  TODO_verify_ssa | TODO_verify_stmts | TODO_dump_func | \
	  TODO_remove_unused_locals | TODO_update_ssa | TODO_cleanup_cfg | \
	  TODO_rebuild_cgraph_edges | TODO_verify_flow
#include "gcc-generate-simple_ipa-pass.h"
#undef PASS_NAME


/* Entry point of the oracle, look up current function weather or not beed
   gathered into our target function set. If YES return 1 otherwise return 0 */

int 
is_rap_function_maybe_roped (tree f)
{
  if (! is_rap_function_may_be_aliased (f))
    return 0;
  /* Ask the oracle for help */
  if (0 == cfi_gcc_optimize_level)
    /* Function is_rap_function_may_be_aliased() must be failed we arive here, 
       but our oracle dependent the GCC optimizations. */
    return 1;
  else
    return bitmap_bit_p (sensi_funcs, DECL_UID (f));
}

#if 0
/* Write some statistics for our algorithm */

void
dump_rap_opt_statistics ()
{
  dump_rap_opt_statistics_fd = fopen (dump_rap_opt_statistics_filename, "a");
  fprintf (dump_rap_opt_statistics_fd, "%d\n", rap_opt_statistics_data);

  return;
}
#endif

/* Clean and cloese function for optimizations.  */

void
rap_optimization_clean ()
{
  /* Now we have finish our job, close file and destroy the GCC allocated 
     data. */
  //fclose (dump_rap_opt_statistics_fd);
  bitmap_clear (sensi_funcs);
  pointer_set_destroy (sensi_func_types);

  return;
}


/* Type definition for hash value maps. */
struct cfi_function_hash_maps_t
{
  struct pointer_map_t *map;
  /* Implementate a circle list. */
  unsigned total;
#if 0
#define HL_CFI_MAP_CACHE_SIZE 3
  // FIFO cache.
  struct cfi_function_hash_pair_t
  {
    tree type;
    rap_hash_value_type val;
  } cfi_cache [HL_CFI_MAP_CACHE_SIZE];
#endif
};

/* Constains the fucntion type and hash value maps. */
static struct cfi_function_hash_maps_t cfi_db;

/* Temp type, used for access cfi_db. */
struct cfi_key_value_t
{
 tree type;
 rap_hash_value_type val;
};


/* Hook fed to pointer_map_traverse, type compatiablity test. 
   If find return TRUE, otherwise FALSE. */

bool
pointer_map_access (const void *key, void **val, void *type)
{
  struct cfi_key_value_t *contain_addr;

  contain_addr = (struct cfi_key_value_t *)type;
  if (types_compatible_p ((tree)key, contain_addr->type))
  {
    contain_addr->val = (rap_hash_value_type)(ptrdiff_t)*val;
    /* For the hook compatablity, 
       we want pointer_map_traverse() STOP, need return false.  */
    //return true;
    return false;
  }

  /* Wierd. hook compatiablity.  */
  return true;
}


/* Search the [function type : hash value] table, if not have compatiable 
   type match, create one and insert into the table. */

static rap_hash_value_type
find_or_create_cfi_hash_val (tree type)
{
  int i;
  rap_hash_value_type val = 0;
  void **val_address;

  gcc_assert (TREE_CODE (type) == FUNCTION_TYPE);
  //struct pointer_map_t *cfi_function_hash_maps;
  if (! cfi_db.map)
    cfi_db.map = pointer_map_create ();
  
  /* Search */
  struct cfi_key_value_t contain = {type, val};
  if (0 == cfi_db.total)
    goto create;
  pointer_map_traverse (cfi_db.map, pointer_map_access, (void *)&contain);
  if (contain.val)
    return contain.val;

  /* Fall through. update db. */
create:
  val = (rap_hash_value_type)
	((rap_hash_function_type (type, imprecise_rap_hash_flags)).hash);
  val_address = pointer_map_insert (cfi_db.map, (void *)type);
  /* Store the rap_hash_value_type hash key as a pointer value. */
  val_address[0] = (void *)(ptrdiff_t)val;
  ++cfi_db.total;
  return val;
}

#define BUILD_SOURCE_HASH_TREE 1
#define BUILD_TARGET_HASH_TREE 2


/* Build cfi hash tree, target or source depend on the argument.
   ??? should we reuse the tree node. */
// set_dom_info_availability (enum cdi_direction dir, enum dom_state new_state)
//free_dominance_info (enum cdi_direction dir)

static tree
build_cfi_hash_tree (gimple cs, int direct, tree *target_off_type_p)
{
  //tree hash_tree, var;
  rap_hash_value_type val;
  tree decl, func_type;

  gcc_assert(is_gimple_call (cs));
  
  decl = gimple_call_fn (cs);
  func_type = TREE_TYPE (TREE_TYPE (decl));
  // safe guard 
  gcc_assert (TREE_CODE (decl) == SSA_NAME);
  gcc_assert (TREE_CODE (func_type) == FUNCTION_TYPE);
  
  // create source hash tree.
  if (direct == BUILD_SOURCE_HASH_TREE)
    {
      val = find_or_create_cfi_hash_val (func_type);
      return build_int_cst(integer_type_node, val);
    }
  else
    {
      tree off_tree;
      int target_offset;

      gcc_assert (direct == BUILD_TARGET_HASH_TREE);
      gcc_assert (target_off_type_p);
      
      //target = create_tmp_var (integer_type_node, "hl_target");
      //target = make_ssa_name (var, NULL);

      /* This code fragment of compute target function hash offset comes
	 from Pax RAP. */
      /* We need the forward offset. */
      if (UNITS_PER_WORD == 8)
        {
	  target_offset = - 2 * sizeof(rap_hash_value_type);
	  *target_off_type_p = long_integer_type_node;
	}
      else if (UNITS_PER_WORD == 4)
        {
	  target_offset = - sizeof(rap_hash_value_type);
	  *target_off_type_p = integer_type_node;
        }
      else
	gcc_unreachable ();

      /* Build the tree for : ((rap_hash_value_type *)target_function - 1) 
         This code is referenced from gcc source: gimplify_modify_expr_rhs() */

      // integer_ptr_type_node
      // func is the function pointer, ADDR_EXPR, pointer(function)
      gcc_assert (FUNCTION_POINTER_TYPE_P ( TREE_TYPE (decl)));
      // type is the result type of cast.
      off_tree = build_int_cst (*target_off_type_p, (HOST_WIDE_INT)target_offset);
      return fold_build2 (MEM_REF, *target_off_type_p, decl,
		   // This is a pointer type tree reprensent the offset.
		   build_int_cst_wide (integer_ptr_type_node,
				       TREE_INT_CST_LOW (off_tree),
				       TREE_INT_CST_HIGH (off_tree)));

    }

  gcc_unreachable ();
}


/* Probability of the catch branch is taken.  */
#define ERR_PROB 0.001


/* Help function called when the fe-cfi violate catched. */

static basic_block
cfi_catch_and_trap_bb (gimple cs, basic_block after)
{
  tree trap;
  gimple g;
  gimple_seq seq;
  basic_block bb;
#if 0
  tree report;
  tree param;
  gimple_seq seq;
  basic_block bb;
  gimple_stmt_iterator gsi;

  /* Build the report & trap tree. */
  const char *str = "[!] HardenedLinux cfi violate catched.";
  int len = strlen (str);
  param = build_string_literal (len, str);
  
  /* gimple sequence for bb.  */
  seq = g = gimple_build_call (report, loc);
  /* ssa concerns.  */
  update_modified_stmt (g);
  bb = create_basic_block (seq, NULL, after);
  gimple_set_block (g, bb);

  /* Initialize iterator.  */
  //gsi = gsi_start (seq);
#endif  
  trap = builtin_decl_explicit (BUILT_IN_TRAP);
  seq = g = gimple_build_call (trap, 0);
  /* Set the limits on seq.  */
  g->gsbase.prev = g;
  g->gsbase.next = NULL;
  bb = create_basic_block (seq, NULL, after);
  /* For update ssa.  */
  gcc_assert (cfun->gimple_df && gimple_ssa_operands (cfun)->ops_active);
  // Need mark update???
  update_stmt_if_modified (g);
  //update_modified_stmt (g);
  //gsi_insert_after (&gsi, g, GSI_SAME_STMT);
  gimple_set_bb (g, bb);
  gimple_set_location (g, gimple_location (cs));
  gimple_set_block (g, gimple_block (cs));
  
  return bb;
}


/* Insert branch and create two blcok contain original function call and our
   catch code. And also need complete the control flow graph.
     +-------
     stmt1;
     call fptr;
     +-------
     change to =>
     +-------
     stmt1;
     lhs_1 = t_;
     ne_expr (lhs_1, s_);
     // FALLTHRU
     <bb ???> # true
     cfi_catch();
     <bb ???> # false
     call fptr;
     +--------

   The value of argument s_ is a const integer tree, 
   t_ is a MEM-REF, may need insert temp varibale as lhs_1 get the value. */

static void
insert_cond_and_build_ssa_cfg (gimple_stmt_iterator *gp, 
		               tree s_, 
			       tree t_, 
			       tree t_t)
{
  gimple cs, g;
  gimple_stmt_iterator gsi;
  gimple assign   // used for dumps.
  gimple cond;    // test gimple we insert.
  gimple call;    // call label gimple we insert.
  basic_block old_bb;
  basic_block catch_bb;
  edge edge_false;
  edge edge_true;
  tree lhs;       // temp variable suit for direct ssa name.
  tree lhs_1;

  gsi = *gp;
  cs = gsi_stmt (gsi);
  gcc_assert (is_gimple_call (cs));
  /* Possible???  */
  gcc_assert (! stmt_ends_bb_p (cs));

#if 0
  /* First of all, disable the dom info, for effecicent and simplity */
  if (is_cfi_need_clean_dom_info && ! is_cfi_chaned_cfun)
    {
      free_dominance_info (CDI_DOMINATORS);
      free_dominance_info (CDI_POST_DOMINATORS);
      /* Set disavailable is NOT enough, 
	 will catch assert of function calculate_dominance_info.  */
      //set_dom_info_availability (CDI_DOMINATORS, DOM_NONE);
      //set_dom_info_availability (CDI_POST_DOMINATORS, DOM_NONE);
      is_cfi_need_clean_dom_info = false;
    }
#endif

  /* Insert gimples. */
  /* lhs_1 = t_ */
  lhs = create_tmp_var (t_t, "hl_cfi_hash");
  gcc_assert (is_gimple_reg (lhs));
  assign = g = gimple_build_assign (lhs, t_);
  lhs_1 = make_ssa_name (lhs, g);
  gimple_assign_set_lhs (g, lhs_1);
  // Complete the ssa define statement.
  //SSA_NAME_DEF_STMT (lhs_1) = g;
  gimple_set_block (g, gimple_block (cs));
  gsi_insert_before (&gsi, g, GSI_SAME_STMT);
  // if (lhs_1 != s_) goto cfi_catch else goto call
  cond = g = gimple_build_cond (NE_EXPR, lhs_1, s_, NULL, NULL);
  gimple_set_block (g, gimple_block (cs));
  gsi_insert_before (&gsi, g, GSI_SAME_STMT);
  
  call = cs;
  // current statement should be original call.
  gcc_assert (is_gimple_call (gsi_stmt (gsi)));
  
  // guard test.
  GIMPLE_CHECK (cond, GIMPLE_COND);
  GIMPLE_CHECK (call, GIMPLE_CALL);
  gcc_assert (gimple_has_body_p (cfun->decl));

  /* We can sure we have this code fragment(write as gimple pointers):
     # old code
     assign
     cond
     <new bb> #true
     catch
     <old bb> #false
     call
     # old code  */
  /* Make the blocks & edges. */
  /* Get the original bb, Thers is only one. 
     For now the basic block is clean.  */
  old_bb = gimple_bb (cs);
  edge_false = split_block (old_bb, cs);
  gcc_assert (edge_false->flags == EDGE_FALLTHRU);
  edge_false->flags &= ~EDGE_FALLTHRU;
  edge_false->flags |= EDGE_FALSE_VALUE;
  GIMPLE_CHECK (edge_false->dest->il.gimple.seq, GIMPLE_CALL);

  /* Create block after the block contain original call. 
     We can have a toplogical for the blocks created and old. */
  // EDGE_TRUE_VALUE
  catch_bb = cfi_catch_and_trap_bb (cs, edge_false->dest);
  /* catch_bb must dominated by old the bb contains the indirect call 
     what we insert cfi guard.  */
  if (current_loops != NULL)
    {
      add_bb_to_loop (catch_bb, old_bb->loop_father);
      if (old_bb->loop_father->latch == old_bb)
        old_bb->loop_father->latch = catch_bb;
    }
  edge_true = make_single_succ_edge (old_bb, catch_bb, EDGE_TRUE_VALUE);
  edge_true->probability = REG_BR_PROB_BASE * ERR_PROB;
  edge_true->count = 
      apply_probability (old_bb->count, edge_true->probability);
  // 
  edge_false->probability = inverse_probability (edge_true->probability);
  edge_true->count = old_bb->count - edge_true->count;
  /* As we introduced new control-flow we need to insert phi nodes
     for the new blocks.  */
  //mark_virtual_operands_for_renaming (cfun);
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Found protected indirect call: ");
      print_gimple_stmt (dump_file, cs, 0, TDF_SLIM);
      fprintf (dump_file, "Indirect call gadget will become: ");
      print_gimple_stmt (dump_file, assign, 0, TDF_SLIM);
      print_gimple_stmt (dump_file, cond, 0, TDF_SLIM);
      print_gimple_stmt (dump_file, catch_bb->il.gimple.seq, 0, TDF_SLIM);
      print_gimple_stmt (dump_file, cs, 0, TDF_SLIM);
      fprintf (dump_file, "\n");
    }

  return;
}


/* Build the check statement: 
   if ((int *)(cs->target_function - sizeof(rap_hash_value_type)) != hash) 
     catch () */

static void
build_cfi (gimple_stmt_iterator *gp)
{
  gimple cs;
  tree th;  // hash get behind the function definitions.
  tree sh;  // hash get before indirect calls.
  tree target_off_type = NULL;
  tree decl;

  cs = gsi_stmt (*gp);
  gcc_assert (is_gimple_call (cs));
  decl = gimple_call_fn (cs);
  /* We must be indirect call */
  gcc_assert (TREE_CODE (decl) == SSA_NAME);
  gcc_assert (TREE_TYPE (TREE_TYPE (decl)) == cs->gimple_call.u.fntype);
  
  /* build source hash tree */
  sh = build_cfi_hash_tree (cs, BUILD_SOURCE_HASH_TREE, NULL);
  /* build target hash tree */
  th = build_cfi_hash_tree (cs, BUILD_TARGET_HASH_TREE, &target_off_type);

  /* Build the condition expression and insert into the code block, because
     the conditional import new branch, so we also need update the blocks */
  insert_cond_and_build_ssa_cfg (gp, sh, th, target_off_type);

  return;
}


/// cgraph_local_node_p
/* This new pass will be added after the GCC pass ipa "pta". */

static unsigned int
hl_cfi_execute ()
{
  struct cgraph_node *node;
  bool is_status_changed = false;
  
  /* If we are in lto mode, execute this pass in the ltrans. */
  if (flag_lto && ! flag_ltrans)
    return 0;
  
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      struct function *func;
      basic_block bb;
      
      /* Without body not our intent. */
      if (!cgraph_function_with_gimple_body_p (node))
	continue;

      func = DECL_STRUCT_FUNCTION (node->symbol.decl);
      push_cfun (func);
      /* If insert blocks is inside a loop.  */
      loop_optimizer_init (LOOPS_NORMAL | LOOPS_HAVE_RECORDED_EXITS);
  
      FOR_EACH_BB_FN (bb, func)
	{
	  gimple_stmt_iterator gsi;
	  //is_cfi_chaned_cfun = false;

	  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	    {
	      //tree decl;
	      gimple cs;
	      //tree hash;
	      cs = gsi_stmt (gsi);
	      /* We are in forward cfi only cares about function call */
	      if (! is_gimple_call (cs))
		continue;
	      /* Indirect calls */
	      if (NULL == gimple_call_fndecl (cs))
	        {
		  /* Must have will been inserted guard code. */
		  is_status_changed |= true;
                  /* First of all, disable the dom info, for effecicent and simplity */
                  free_dominance_info (CDI_DOMINATORS);
                  free_dominance_info (CDI_POST_DOMINATORS);
		  build_cfi (&gsi);
	        }
	    }
	}
      
      loop_optimizer_finalize ();

      /* As we introduced new control-flow we need to insert phi nodes
         for the new blocks.  */
      if (is_status_changed)
        mark_virtual_operands_for_renaming (cfun);

      pop_cfun ();
    }

  if (is_status_changed)
    return TODO_update_ssa;

  return 0;
}


static bool
hl_cfi_gate ()
{
  return require_call_hl_cfi;
}


/* Genetate the pass structure */
#define PASS_NAME hl_cfi
//#define PROPERTIES_REQUIRED PROP_gimple_any
//#define PROPERTIES_PROVIDED PROP_gimple_lcf
#define TODO_FLAGS_FINISH \
	  TODO_verify_ssa | TODO_verify_stmts | TODO_verify_flow | \
	  TODO_dump_func | TODO_remove_unused_locals | TODO_cleanup_cfg | \
	  TODO_rebuild_cgraph_edges
#include "gcc-generate-simple_ipa-pass.h"
#undef PASS_NAME

