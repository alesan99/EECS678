#ifndef TEST4_H
#define TEST4_H

#include "LinkedList.h"
#include "ListQuestions.h"

int test4() {
    int points = 10;

    // Step 1: Create two empty linked lists
    struct Node* list1 = NULL;
    struct Node* list2 = NULL;

    // Step 2: Merge the two empty lists and check the output
    struct Node* mergedList = mergeLists(list1, list2);
    
    if (mergedList != NULL) {
        printf("Merge Test Failed: Expected NULL, but function returned non-NULL.\n");
        points -= 4;
    } else {
        printf("Merge Test Passed: Both lists were empty, and merge returned NULL as expected.\n");
    }

    // Step 3: Check for a cycle in the merged list (Should be false)
    if (mergedList != NULL && hasCycle(mergedList)) {
        printf("Cycle Detection Test Failed: Incorrectly detected a cycle.\n");
        points -= 4;
    } else if (mergedList == NULL) {
        printf("Cycle Detection Test Passed: No cycle detected, as expected with empty lists.\n");
    } else {
        printf("Cycle Detection Test Failed: Merged list is NULL, but incorrectly detected as having a cycle.\n");
        points -= 4;
    }

    // Clean up memory
    deleteList(&mergedList);

    // Return the final score for this test
    return points;
}

#endif

