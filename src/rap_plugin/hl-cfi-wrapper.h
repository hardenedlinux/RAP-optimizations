/* Writed by David fuqiang Fan <feqin1023@gmail.com> &
   Shawn C[a.k.a "citypw"] <citypw@gmail.com> members of HardenedLinux.
   The code of this file try to make some optimizationsfor  PaX RAP.
   Supply the API for RAP.

   Contains function api wrappers conquer different GCC versions.

   Licensed under the GPL v2. */

#if BUILDING_GCC_VERSION >= 7000
#define cfi_hash_set 



