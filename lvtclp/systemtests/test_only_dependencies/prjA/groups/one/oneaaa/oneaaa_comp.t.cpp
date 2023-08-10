#include <oneaaa_comp.h>
#include <oneaaa_othercomp.h> // Should be in the component, but isn't! "Not good"
#include <oneaaa_othercomp2.h> // Is in the component _and_ in the test. "All good"

int main()
{
    // ... Test case ...
    return 0;
}
