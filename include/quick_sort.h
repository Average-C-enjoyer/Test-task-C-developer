#ifndef QUICK_SORT_H
#define QUICK_SORT_H

#include "stat_data.h"

static inline void swap(StatData *a, StatData *b)
{
    StatData tmp = *a;
    *a = *b;
    *b = tmp;
}

static void insertion_sort(StatData *arr, long left, long right)
{
    for (long i = left + 1; i <= right; i++) {
        StatData key = arr[i];
        long j = i - 1;

        while (j >= left && arr[j].cost > key.cost) {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = key;
    }
}

extern void quick_sort(StatData *arr, long left, long right)
{
    while (left < right) {

        // for small arrays insertion sort is faster
        if (right - left < 16) {
            insertion_sort(arr, left, right);
            return;
        }

        // median-of-three
        long mid = left + (right - left) / 2;

        if (arr[mid].cost < arr[left].cost)
            swap(&arr[mid], &arr[left]);
        if (arr[right].cost < arr[left].cost)
            swap(&arr[right], &arr[left]);
        if (arr[right].cost < arr[mid].cost)
            swap(&arr[right], &arr[mid]);

        float pivot = arr[mid].cost;

        long i = left;
        long j = right;

        while (i <= j) {
            while (arr[i].cost < pivot) i++;
            while (arr[j].cost > pivot) j--;

            if (i <= j) {
                swap(&arr[i], &arr[j]);
                i++;
                j--;
            }
        }

        // tail recursion elimination
        if (j - left < right - i) {
            if (left < j)
                quick_sort(arr, left, j);
            left = i;
        } else {
            if (i < right)
                quick_sort(arr, i, right);
            right = j;
        }
    }
}

#endif // QUICK_SORT_H

