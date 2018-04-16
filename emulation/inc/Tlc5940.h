/*  Copyright (c) 2009 by Alex Leone <acleone ~AT~ gmail.com>

    This file is part of the Arduino TLC5940 Library.

    The Arduino TLC5940 Library is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    The Arduino TLC5940 Library is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with The Arduino TLC5940 Library.  If not, see
    <http://www.gnu.org/licenses/>. */

#ifndef TLC5940_H
#define TLC5940_H

#define TLC_CHANNEL_TYPE    uint8_t

class Tlc5940
{
  public:
    void init(uint16_t initialValue = 0) {}
    void clear(void) {}
    uint8_t update(void) {return 0;}
    void set(TLC_CHANNEL_TYPE channel, uint16_t value) {}
    uint16_t get(TLC_CHANNEL_TYPE channel) {return 0;}
    void setAll(uint16_t value) {}

};

// for the preinstantiated Tlc variable.
extern Tlc5940 Tlc;

#endif

