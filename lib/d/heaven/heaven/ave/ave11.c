#include "immortals.h"
#include "immort.h"
#include "path.h"
inherit"/std/room.c";

void init() {
//wtf is this?    shield_it(SHIELDCITY,"/d/ss/daggerford/ladyluck");
   ::init();
}

void setup() {
   set_long("You are on Immortal avenue.  You stand in the City of"+
      " the Immortals.  You stare in awe at the wondrous buildings around"+
      " you.  You feel the magic in the air, and feel at peace here. To"+
      " the west of you lies "+ IM11A + 
      ".\n");
   set_short("Immortal square");
   add_exit("west",IR11A,"door");
   add_exit("east",ROOM+"ave8","road");
   add_exit("south",ROOM+"ave12","road");
   set_light(100);
}      

