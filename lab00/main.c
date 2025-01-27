#include "test1.h"
#include "test2.h"
#include "test3.h"
#include "test4.h"
#include "test5.h"
#include "test6.h"
#include "test7.h"
#include "test8.h"
#include "test9.h"
#include "test10.h"

int main() {
    int totalScore = 0;

    totalScore += test1();
    totalScore += test2();
    totalScore += test3();
    totalScore += test4();
    totalScore += test5();
    totalScore += test6();
    totalScore += test7();
    totalScore += test8();
    totalScore += test9();
    totalScore += test10();

    // Print the total score
    printf("Total Score: %d/100\n", totalScore);

    return 0;
}

