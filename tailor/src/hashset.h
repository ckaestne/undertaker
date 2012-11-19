// Copyright (C) 2012 Bernhard Heinloth <bernhard@heinloth.net>
// Copyright (C) 2012 Valentin Rothberg <valentinrothberg@gmail.com>
// Copyright (C) 2012 Andreas Ruprecht  <rupran@einserver.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef HASHSET_H
#define HASHSET_H

#include <inttypes.h>
#include <unistd.h>
#include <stdbool.h>

// Capacity for hashing set size - the bigger the faster. And Primes are great.
#define SETCAPACITY 31337
// Capacity for elements - should be big enough to hold all common addresses
#define ELEMENTSCAPACITY 250000

/* Structure to hold address information and provide a pointer to the next
 * structure inside the buckets.
 */
typedef struct hashsetaddr {
    uintptr_t address;
    struct hashsetaddr * next;
} hashsetaddr;

// Mapping of address to buckets
hashsetaddr * hashset[SETCAPACITY];
// Actual location of all buckets
hashsetaddr hashelements[ELEMENTSCAPACITY];

// How many places are filled
int hashelementcounter = 0;

/* For our purposes we only need to add stuff, but never delete.
 * We check, if the hashmap is full, or the key already exists.
 * If it did not exists, we add the new address to the end of a bucket.
 */
inline bool hashadd(uintptr_t address) {
    unsigned int key = address % SETCAPACITY;
    hashsetaddr * hashiter;
    // we are full... sorry
    if (hashelementcounter == ELEMENTSCAPACITY) {
        return true;
    }
    // Check if key already exists
    if (hashset[key] == NULL) {
        hashset[key] = & hashelements[hashelementcounter];
    }
    // Check if it is at base
    else if (hashset[key]->address == address)
        return false;
    // else go thru buckets
    else {
        hashiter = hashset[key];
        while (hashiter->next != NULL) {
            // go to the next bucket
            hashiter = hashiter->next;
            // check if it is the needed one
            if (hashiter->address == address) {
                return false;
            }
        }
        // Paste a new one
        hashiter->next = & hashelements[hashelementcounter];
    }
    // filling the new hashset
    hashelements[hashelementcounter].address = address;
    hashelements[hashelementcounter].next = NULL;
    hashelementcounter++;
    return true;
}

#endif
