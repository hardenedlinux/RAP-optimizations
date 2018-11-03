/* Writed by David fuqiang Fan <feqin1023@gmail.com> &
   Shawn C[a.k.a "citypw"] <citypw@gmail.com> members of HardenedLinux.
   The code of this file try to make some optimizationsfor  PaX RAP.
   Supply the API for RAP.

   Licensed under the GPL v2. */

#include "rap.h"
#include "rap-hl-cfi.h"

//
bool hl_will_call_ipa_pta = false;

// gcc internal defined pass name.
extern struct simple_ipa_opt_pass pass_ipa_pta;
/* Try make GCC call ipa-pta pass if optimization level is NOT 0 */
void 
rap_try_call_ipa_pta (void* gcc_data, void* user_data) 
{
  gcc_assert (current_pass);
  /* If already execute pta pass, return immediatelly.  */
  if (hl_will_call_ipa_pta)
    return;

  if (optimize &&
      (0 == errorcount + sorrycount) &&
      (void *)current_pass == (void *)&pass_ipa_pta)
    {
      hl_will_call_ipa_pta = true;
      *(bool*)gcc_data = true;
      /* The variable optimize is defined int GCC */
      *(volatile int*)user_data = optimize;
    }
  
  return;
}

