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

#include "hashset.h"

hashsetaddr * hashset[SETCAPACITY];
hashsetaddr hashelements[ELEMENTSCAPACITY];

int hashelementcounter = 0;

// Returns false if exists (else true)
inline bool hashadd(uintptr_t address) {
    unsigned int key = address % SETCAPACITY;
    hashsetaddr * hashiter;
    // we are full... sorry
    if (hashelementcounter == ELEMENTSCAPACITY) {
        return true;
    }
    // Check if key already exists
    if (hashset[key] == NULL)
        hashset[key] = & hashelements[hashelementcounter];
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
            if (hashiter->address == address)
                return false;
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
