#ifndef TEST10_H
#define TEST10_H

#include "LinkedList.h"
#include "ListQuestions.h"

int test10() {
    int points = 10;

    // Step 1: Create two linked lists, one in reverse order relative to the other
    struct Node* list1 = NULL;
    struct Node* list2 = NULL;
    list1 = insertAtEnd(list1, 10);
    list1 = insertAtEnd(list1, 20);
    list1 = insertAtEnd(list1, 30);
    list2 = insertAtEnd(list2, 1);
    list2 = insertAtEnd(list2, 2);
    list2 = insertAtEnd(list2, 3);

    // Step 2: Merge the two lists and check the output
    struct Node* mergedList = mergeLists(list1, list2);
    
    if (mergedList == NULL) {
        printf("Merge Test Failed: Function returned NULL.\n");
        points -= 4;
    } else {
        printf("Merged List: ");
        printList(mergedList);  // Expected output: "1 -> 2 -> 3 -> 10 -> 20 -> 30 -> NULL"

        // Checking if the merge was correct
        struct Node* expected = NULL;
        expected = insertAtEnd(expected, 1);
        expected = insertAtEnd(expected, 2);
        expected = insertAtEnd(expected, 3);
        expected = insertAtEnd(expected, 10);
        expected = insertAtEnd(expected, 20);
        expected = insertAtEnd(expected, 30);

        struct Node* temp1 = mergedList;
        struct Node* temp2 = expected;
        int mergeCorrect = 1;

        while (temp1 != NULL && temp2 != NULL) {
            if (temp1->data != temp2->data) {
                mergeCorrect = 0;
                break;
            }
            temp1 = temp1->next;
            temp2 = temp2->next;
        }

        if (temp1 != NULL || temp2 != NULL) {
            mergeCorrect = 0;
        }

        if (!mergeCorrect) {
            printf("Merge Test Failed: Merged list is incorrect.\n");
            points -= 4;
        } else {
            printf("Merge Test Passed.\n");
        }

        // Clean up memory for the expected list
        deleteList(&expected);
    }

    // Step 3: Check for a cycle in the merged list (Should be false)
    if (mergedList != NULL && hasCycle(mergedList)) {
        printf("Cycle Detection Test Failed: Incorrectly detected a cycle.\n");
        points -= 4;
    } else if (mergedList == NULL) {
        printf("Cycle Detection Test Failed: Merged list is NULL.\n");
        points -= 4;
    } else {
        printf("Cycle Detection Test Passed.\n");
    }

    // Clean up memory
    deleteList(&mergedList);

    // Return the final score for this test
    return points;
}

#endif

