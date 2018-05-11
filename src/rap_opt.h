/* The impelmentation code of optimization pass for RAP.
  
We supply the API for RAP  */

#ifndef RAP_OPT_H
#define RAP_OPT_H


//extern int rap_available_naive(tree p, tree func); 

/* This function is defined in gcc/c-decl.c */
//extern tree lookup_name(tree);
/* */
//#define I_SYMBOL_BINDING(node) \
//  (((struct lang_identifier *) IDENTIFIER_NODE_CHECK(node))->symbol_binding)

//#define RAP_POINTER_TO_FUNCTION(P)

/* This is defined in gcc/c-decl.c but as static linkage, so we reuse it here.
   And the depth of external scope is 0. 
   If scope depth is more than 1 it's local scope */
//#define FILE_SCOPE_DEPTH 1

/* File scope function*/
extern bool is_rap_function_never_escape (tree t);

#endif /* RAP_OPT_H */
