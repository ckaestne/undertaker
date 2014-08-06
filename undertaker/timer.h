/*
 *   undertaker - timer header for easy access measurements between two points
 *
 * Copyright (C) 2014 Stefan Hengelein <stefan.hengelein@fau.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>
#include <iostream>

#define TIMING

#ifdef TIMING

#define INIT_TIMER(var) auto var = std::chrono::high_resolution_clock::now();
#define START_TIMER(var) var = std::chrono::high_resolution_clock::now();
#define STOP_TIMER(var)                                                                           \
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(                           \
                     std::chrono::high_resolution_clock::now() - var).count() << std::endl;

#define STOP_TIMER_MUS(var)                                                                       \
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(                           \
                     std::chrono::high_resolution_clock::now() - var).count() << std::endl;

#define STOP_TIMER_NS(var)                                                                        \
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(                            \
                     std::chrono::high_resolution_clock::now() - var).count() << std::endl;

#define P_STOP_TIMER(var, name)                                                                   \
    std::cout << "RUNTIME of " << name << ": "                                                    \
              << std::chrono::duration_cast<std::chrono::milliseconds>(                           \
                     std::chrono::high_resolution_clock::now() - var).count() << " ms."           \
              << std::endl;

#else

#define INIT_TIMER(var)
#define START_TIMER(var)
#define STOP_TIMER(var)
#define P_STOP_TIMER(var, name)

#endif

#endif
