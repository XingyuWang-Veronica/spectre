// #define NUM_POSSIBLE_VALS 256
// #define STEP_SIZE 512

#include "example.h"
#include <stdio.h>

// the probe array used for the attack (its address would be used in a gadget)
int probe_array[NUM_POSSIBLE_VALS * STEP_SIZE] = {0};
// probe_array[43 * STEP_SIZE] = 9;

int gadget(void)
{

	probe_array[42 * STEP_SIZE] = 9;

    // we don't care about this
    int garbage;

    /* The gadget will feature a condition or indirect jump target that can be mispredicted */
    /* I show it here as a conditional, but an indirect jump also works */
    int some_condition = 1;

    /* We assume some secret value is in a register */
    int secret = 42;

    // Step 1: multiply register by step size (might be mul or shift instruction)
    int processed_1 = secret * STEP_SIZE;

    // Step 2: add previous result to probe_array base address (might be add or lea)
    int *processed_2 = probe_array + processed_1;


    /* other code could fall between 2 and 3, so long as processed_2 doesn't change */

    // Step 3: mispredict an indirect or conditional branch
    if (some_condition) {
        // Step 4: processed_2 is loaded at the jump target
        // In the case of an indirect branch (spectre v2), the load can be anywhere in the program,
        // as we can mistrain the branch predictor to jump to arbitray locations.
        // I show it inside the conditional for simplicity.
        garbage = *processed_2;
		printf("garbage is %d\n", garbage);
    }

    return 0;
}

int main(void)
{
    // separate gadget into its own function to make reading assembly a bit easier
    return gadget();
}
